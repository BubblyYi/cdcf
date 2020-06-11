// Generated by the gRPC C++ plugin.
// If you make any local change, they will be lost.
// source: node_keeper.proto

#include <grpcpp/impl/codegen/async_stream.h>
#include <grpcpp/impl/codegen/async_unary_call.h>
#include <grpcpp/impl/codegen/channel_interface.h>
#include <grpcpp/impl/codegen/client_callback.h>
#include <grpcpp/impl/codegen/client_unary_call.h>
#include <grpcpp/impl/codegen/method_handler.h>
#include <grpcpp/impl/codegen/rpc_service_method.h>
#include <grpcpp/impl/codegen/server_callback.h>
#include <grpcpp/impl/codegen/service_type.h>
#include <grpcpp/impl/codegen/sync_stream.h>

#include <functional>

#include "node_keeper.grpc.pb.h"
#include "node_keeper.pb.h"

static const char* NodeKeeper_method_names[] = {
    "/NodeKeeper/GetMembers",
    "/NodeKeeper/Subscribe",
};

std::unique_ptr<NodeKeeper::Stub> NodeKeeper::NewStub(
    const std::shared_ptr< ::grpc::ChannelInterface>& channel,
    const ::grpc::StubOptions& options) {
  (void)options;
  std::unique_ptr<NodeKeeper::Stub> stub(new NodeKeeper::Stub(channel));
  return stub;
}

NodeKeeper::Stub::Stub(
    const std::shared_ptr< ::grpc::ChannelInterface>& channel)
    : channel_(channel),
      rpcmethod_GetMembers_(NodeKeeper_method_names[0],
                            ::grpc::internal::RpcMethod::NORMAL_RPC, channel),
      rpcmethod_Subscribe_(NodeKeeper_method_names[1],
                           ::grpc::internal::RpcMethod::SERVER_STREAMING,
                           channel) {}

::grpc::Status NodeKeeper::Stub::GetMembers(
    ::grpc::ClientContext* context, const ::google::protobuf::Empty& request,
    ::GetMembersReply* response) {
  return ::grpc::internal::BlockingUnaryCall(
      channel_.get(), rpcmethod_GetMembers_, context, request, response);
}

void NodeKeeper::Stub::experimental_async::GetMembers(
    ::grpc::ClientContext* context, const ::google::protobuf::Empty* request,
    ::GetMembersReply* response, std::function<void(::grpc::Status)> f) {
  ::grpc_impl::internal::CallbackUnaryCall(
      stub_->channel_.get(), stub_->rpcmethod_GetMembers_, context, request,
      response, std::move(f));
}

void NodeKeeper::Stub::experimental_async::GetMembers(
    ::grpc::ClientContext* context, const ::grpc::ByteBuffer* request,
    ::GetMembersReply* response, std::function<void(::grpc::Status)> f) {
  ::grpc_impl::internal::CallbackUnaryCall(
      stub_->channel_.get(), stub_->rpcmethod_GetMembers_, context, request,
      response, std::move(f));
}

void NodeKeeper::Stub::experimental_async::GetMembers(
    ::grpc::ClientContext* context, const ::google::protobuf::Empty* request,
    ::GetMembersReply* response,
    ::grpc::experimental::ClientUnaryReactor* reactor) {
  ::grpc_impl::internal::ClientCallbackUnaryFactory::Create(
      stub_->channel_.get(), stub_->rpcmethod_GetMembers_, context, request,
      response, reactor);
}

void NodeKeeper::Stub::experimental_async::GetMembers(
    ::grpc::ClientContext* context, const ::grpc::ByteBuffer* request,
    ::GetMembersReply* response,
    ::grpc::experimental::ClientUnaryReactor* reactor) {
  ::grpc_impl::internal::ClientCallbackUnaryFactory::Create(
      stub_->channel_.get(), stub_->rpcmethod_GetMembers_, context, request,
      response, reactor);
}

::grpc::ClientAsyncResponseReader< ::GetMembersReply>*
NodeKeeper::Stub::AsyncGetMembersRaw(::grpc::ClientContext* context,
                                     const ::google::protobuf::Empty& request,
                                     ::grpc::CompletionQueue* cq) {
  return ::grpc_impl::internal::ClientAsyncResponseReaderFactory<
      ::GetMembersReply>::Create(channel_.get(), cq, rpcmethod_GetMembers_,
                                 context, request, true);
}

::grpc::ClientAsyncResponseReader< ::GetMembersReply>*
NodeKeeper::Stub::PrepareAsyncGetMembersRaw(
    ::grpc::ClientContext* context, const ::google::protobuf::Empty& request,
    ::grpc::CompletionQueue* cq) {
  return ::grpc_impl::internal::ClientAsyncResponseReaderFactory<
      ::GetMembersReply>::Create(channel_.get(), cq, rpcmethod_GetMembers_,
                                 context, request, false);
}

::grpc::ClientReader< ::Event>* NodeKeeper::Stub::SubscribeRaw(
    ::grpc::ClientContext* context, const ::SubscribeRequest& request) {
  return ::grpc_impl::internal::ClientReaderFactory< ::Event>::Create(
      channel_.get(), rpcmethod_Subscribe_, context, request);
}

void NodeKeeper::Stub::experimental_async::Subscribe(
    ::grpc::ClientContext* context, ::SubscribeRequest* request,
    ::grpc::experimental::ClientReadReactor< ::Event>* reactor) {
  ::grpc_impl::internal::ClientCallbackReaderFactory< ::Event>::Create(
      stub_->channel_.get(), stub_->rpcmethod_Subscribe_, context, request,
      reactor);
}

::grpc::ClientAsyncReader< ::Event>* NodeKeeper::Stub::AsyncSubscribeRaw(
    ::grpc::ClientContext* context, const ::SubscribeRequest& request,
    ::grpc::CompletionQueue* cq, void* tag) {
  return ::grpc_impl::internal::ClientAsyncReaderFactory< ::Event>::Create(
      channel_.get(), cq, rpcmethod_Subscribe_, context, request, true, tag);
}

::grpc::ClientAsyncReader< ::Event>* NodeKeeper::Stub::PrepareAsyncSubscribeRaw(
    ::grpc::ClientContext* context, const ::SubscribeRequest& request,
    ::grpc::CompletionQueue* cq) {
  return ::grpc_impl::internal::ClientAsyncReaderFactory< ::Event>::Create(
      channel_.get(), cq, rpcmethod_Subscribe_, context, request, false,
      nullptr);
}

NodeKeeper::Service::Service() {
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      NodeKeeper_method_names[0], ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler<
          NodeKeeper::Service, ::google::protobuf::Empty, ::GetMembersReply>(
          std::mem_fn(&NodeKeeper::Service::GetMembers), this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      NodeKeeper_method_names[1], ::grpc::internal::RpcMethod::SERVER_STREAMING,
      new ::grpc::internal::ServerStreamingHandler<NodeKeeper::Service,
                                                   ::SubscribeRequest, ::Event>(
          std::mem_fn(&NodeKeeper::Service::Subscribe), this)));
}

NodeKeeper::Service::~Service() {}

::grpc::Status NodeKeeper::Service::GetMembers(
    ::grpc::ServerContext* context, const ::google::protobuf::Empty* request,
    ::GetMembersReply* response) {
  (void)context;
  (void)request;
  (void)response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status NodeKeeper::Service::Subscribe(
    ::grpc::ServerContext* context, const ::SubscribeRequest* request,
    ::grpc::ServerWriter< ::Event>* writer) {
  (void)context;
  (void)request;
  (void)writer;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}