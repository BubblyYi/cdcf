/*
 * Copyright (c) 2019-2020 ThoughtWorks Inc.
 */
#include "src/gossip.h"

#include <iostream>
// anti clang-format sort
#include <asio.hpp>

#include "src/gossip/message.h"

namespace gossip {

using asio::ip::tcp, asio::ip::udp;

class Transport : public Transportable {
 public:
  Transport(const Address &udp, const Address &tcp) try
      : udpSocket_(ioContext_, udp::endpoint(udp::v4(), udp.port)),
        stopReceiving_(false),
        acceptor_(ioContext_, tcp::endpoint(tcp::v4(), tcp.port)) {
    StartReceiveGossip();
    StartAccept();
    ioThread_ = std::thread(&Transport::IORoutine, this);
  } catch (const std::exception &e) {
    throw PortOccupied(e);
  }

  virtual ~Transport() {
    stopReceiving_ = true;
    ioContext_.stop();
    if (ioThread_.joinable()) {
      ioThread_.join();
    }
  }

 public:
  virtual ErrorCode Gossip(const std::vector<Address> &nodes,
                           const Payload &payload) {
    udp::resolver resolver(ioContext_);
    for (const auto &node : nodes) {
      auto endpoints = resolver.resolve(node.host, std::to_string(node.port));
      const auto &endpoint = *endpoints.begin();
      auto sent = udpSocket_.send_to(asio::buffer(payload.data), endpoint);
      assert(sent == payload.data.size() && "all bytes should be sent");
    }
    return ErrorCode::kOK;
  }

  virtual void RegisterGossipHandler(GossipHandler handler) {
    gossipHandler_ = handler;
  }

  virtual ErrorCode Push(const Address &node, const void *data, size_t size,
                         DidPushHandler didPush) {
    tcp::resolver resolver(ioContext_);
    auto endpoints = resolver.resolve(node.host, std::to_string(node.port));
    tcp::socket socket(ioContext_);
    asio::connect(socket, endpoints);
    auto pointer = reinterpret_cast<const uint8_t *>(data);
    auto buffer = Message(Message::Type::kPush, pointer, size).Encode();
    if (didPush) {
      socket.async_write_some(asio::buffer(buffer),
                              [didPush](const std::error_code &error, size_t) {
                                didPush(ErrorCode::kOK);
                              });
      return ErrorCode::kOK;
    } else {
      auto sent = socket.write_some(asio::buffer(buffer));
      return sent == buffer.size() ? ErrorCode::kOK : ErrorCode::kUnknown;
    }
  }

  virtual void RegisterPushHandler(PushHandler handler) {
    pushHandler_ = handler;
  }

  virtual PullResult Pull(const Address &node, const void *data, size_t size,
                          DidPullHandler didPull) try {
    PullResult result{ErrorCode::kUnknown, {}};
    tcp::resolver resolver(ioContext_);
    auto endpoints = resolver.resolve(node.host, std::to_string(node.port));
    auto socket = std::make_shared<tcp::socket>(ioContext_);
    asio::connect(*socket, endpoints);
    auto pointer = reinterpret_cast<const uint8_t *>(data);
    auto out = Message(Message::Type::kPull, pointer, size).Encode();
    if (didPull) {
      socket->async_write_some(
          asio::buffer(out),
          [didPull, result, socket](const std::error_code &err, size_t bytes) {
            if (err) {
              didPull(result);
              return;
            }
            Message message;
            auto buffer = std::vector<uint8_t>(1024, 0);
            while (true) {
              auto bytes_transferred =
                  socket->read_some(asio::buffer(buffer, 1024));
              if (bytes_transferred == 0) {
                didPull(result);
                return;
              }
              for (size_t decoded = 0; decoded < bytes_transferred;) {
                decoded += message.Decode(&buffer[decoded],
                                          bytes_transferred - decoded);
                if (message.IsSatisfied()) {
                  didPull(PullResult{ErrorCode::kOK, message.Data()});
                  return;
                }
              }
            }
          });
    } else {
      auto sent = socket->write_some(asio::buffer(out));
      if (sent != out.size()) {
        return result;
      }
      Message message;
      auto buffer = std::vector<uint8_t>(1024, 0);
      while (true) {
        auto bytes_transferred = socket->read_some(asio::buffer(buffer, 1024));
        if (bytes_transferred == 0) {
          return result;
        }
        for (size_t decoded = 0; decoded < bytes_transferred;) {
          decoded +=
              message.Decode(&buffer[decoded], bytes_transferred - decoded);
          if (message.IsSatisfied()) {
            return PullResult{ErrorCode::kOK, message.Data()};
          }
        }
      }
    }
    return result;
  } catch (const std::exception &) {
    return PullResult{ErrorCode::kUnknown, {}};
  }

  virtual void RegisterPullHandler(PullHandler handler) {
    pullHandler_ = handler;
  }

 private:
  void StartReceiveGossip() {
    auto buffer =
        std::make_shared<std::vector<uint8_t>>(Payload::kMaxPayloadSize);
    auto remote = std::make_shared<udp::endpoint>();
    udpSocket_.async_receive_from(
        asio::buffer(*buffer), *remote,
        [this, buffer, remote](const asio::error_code &error,
                               std::size_t bytes_transferred) {
          if (error) {
            return;
          }
          if (gossipHandler_) {
            const Address address{remote->address().to_string(),
                                  remote->port()};
            gossipHandler_(address, Payload(buffer->data(), bytes_transferred));
          }
          StartReceiveGossip();
        });
  }

  void IORoutine() {
    try {
      ioContext_.run();
    } catch (...) {
    }
  }

  class Connection : public std::enable_shared_from_this<Connection> {
   public:
    typedef std::function<void(tcp::socket *socket, const Address &,
                               const Message &)>
        ReceiveHandler;

    Connection(tcp::socket &&socket, ReceiveHandler onReceive)
        : socket_(std::move(socket)), onReceive_(onReceive) {}

    tcp::socket &Socket() { return socket_; }

    void Start() {
      auto buffer = std::make_shared<std::vector<uint8_t>>(1024, 0);
      auto that = shared_from_this();

      socket_.async_receive(
          asio::buffer(*buffer),
          [this, that, buffer](const std::error_code &error,
                               size_t bytes_transferred) {
            if (error) {
              return;
            }

            if (bytes_transferred != 0) {
              for (size_t decoded = 0; decoded < bytes_transferred;) {
                decoded += message_.Decode(&(*buffer)[decoded],
                                           bytes_transferred - decoded);
                if (message_.IsSatisfied()) {
                  auto remote = socket_.remote_endpoint();
                  const Address address{remote.address().to_string(),
                                        remote.port()};
                  onReceive_(&socket_, address, message_);
                  message_.Reset();
                }
              }
            }
            Start();
          });
    }

   private:
    tcp::socket socket_;
    ReceiveHandler onReceive_;
    Message message_;
  };  // namespace gossip

  void StartAccept() {
    acceptor_.async_accept(
        [this](const std::error_code &error, tcp::socket socket) {
          if (!error) {
            std::make_shared<Connection>(
                std::move(socket),
                [this](tcp::socket *socket, const Address &address,
                       const Message &message) {
                  const auto &buffer = message.Data();
                  if (message.Type() == Message::Type::kPush) {
                    pushHandler_(address, buffer.data(), buffer.size());
                  } else if (message.Type() == Message::Type::kPull) {
                    OnPull(socket, address, buffer);
                  }
                })
                ->Start();
          }
          StartAccept();
        });
  }

 private:
  void OnPull(tcp::socket *socket, const Address &address,
              const std::vector<uint8_t> &request) {
    auto response = pullHandler_(address, request.data(), request.size());
    Message respondMessage(Message::Type::kPullResponse, response.data(),
                           response.size());
    auto out = respondMessage.Encode();
    socket->write_some(asio::buffer(out));
  }

  asio::io_context ioContext_;
  udp::socket udpSocket_;
  bool stopReceiving_;
  std::thread ioThread_;
  GossipHandler gossipHandler_;
  tcp::acceptor acceptor_;
  PushHandler pushHandler_;
  PullHandler pullHandler_;
};  // namespace gossip

std::unique_ptr<Transportable> CreateTransport(const Address &udp,
                                               const Address &tcp) {
  return std::make_unique<Transport>(udp, tcp);
}

}  // namespace gossip
