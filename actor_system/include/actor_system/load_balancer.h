/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */
#ifndef ACTOR_SYSTEM_INCLUDE_ACTOR_SYSTEM_LOAD_BALANCER_H_
#define ACTOR_SYSTEM_INCLUDE_ACTOR_SYSTEM_LOAD_BALANCER_H_

#include <limits>
#include <unordered_map>

#include <caf/all.hpp>
#include <caf/default_attachable.hpp>

namespace cdcf {
class load_balancer : public caf::monitorable_actor {
 public:
  static caf::actor make(caf::execution_unit *host) {
    auto &sys = host->system();
    caf::actor_config config{host};
    auto res = caf::make_actor<load_balancer, caf::actor>(
        sys.next_actor_id(), sys.node(), &sys, config);
    auto ptr = static_cast<load_balancer *>(
        caf::actor_cast<caf::abstract_actor *>(res));
    return res;
  }

 public:
  explicit load_balancer(caf::actor_config &config)
      : caf::monitorable_actor(config),
        planned_reason_(caf::exit_reason::normal) {}

  void enqueue(caf::mailbox_element_ptr what,
               caf::execution_unit *host) override {
    caf::upgrade_lock<caf::detail::shared_spinlock> guard{workers_mtx_};
    if (Filter(guard, what, host)) {
      return;
    }
    if (what->mid.is_response()) {
      return Reply(what, host, guard);
    } else {
      return Relay(what, host, guard);
    }
  }

  void on_destroy() override {
    CAF_PUSH_AID_FROM_PTR(this);
    if (!getf(is_cleaned_up_flag)) {
      cleanup(caf::exit_reason::unreachable, nullptr);
      monitorable_actor::on_destroy();
      unregister_from_system();
    }
  }

 protected:
  void on_cleanup(const caf::error &reason) override {
    CAF_PUSH_AID_FROM_PTR(this);
    CAF_IGNORE_UNUSED(reason);
    CAF_LOG_TERMINATE_EVENT(this, reason);
  }

 private:
  struct Ticket {
    static Ticket ReplyTo(const caf::mailbox_element_ptr &what) {
      bool required_reply = what->mid != what->mid.response_id();
      return required_reply ? Ticket{what->mid.response_id(),
                                     caf::actor_cast<caf::actor>(what->sender)}
                            : Ticket{};
    }

    caf::message_id response_id;
    caf::actor response_to;

    void Reply(caf::strong_actor_ptr sender, const caf::message &message,
               caf::execution_unit *host) {
      if (!response_to) {
        return;
      }
      response_to->enqueue(sender, response_id, message, host);
    }
  };

  void Reply(caf::mailbox_element_ptr &what, caf::execution_unit *host,
             caf::upgrade_lock<caf::detail::shared_spinlock> &guard) {
    auto from = caf::actor_cast<caf::actor>(what->sender);
    --loads_[from];
    auto ticket = ExtractTicket(what);
    guard.unlock();
    ticket.Reply(ctrl(), what->move_content_to_message(), host);
  }

  std::pair<caf::message_id, Ticket> PlaceTicket(
      const caf::mailbox_element_ptr &what) {
    auto new_message_id = AllocateRequestID(what);
    auto ticket = Ticket::ReplyTo(what);
    tickets_.emplace(new_message_id.response_id(), ticket);
    return std::make_pair(new_message_id, ticket);
  }

  Ticket ExtractTicket(const caf::mailbox_element_ptr &what) {
    auto it = tickets_.find(what->mid);
    if (it == tickets_.end()) {
      return {};
    }
    /* worker get remove from load balancer, should remove the ticket */
    if (std::find(workers_.begin(), workers_.end(), what->sender) ==
        workers_.end()) {
      tickets_.erase(it);
      return {};
    }
    auto result = it->second;
    tickets_.erase(it);
    return result;
  }

  caf::message_id AllocateRequestID(const caf::mailbox_element_ptr &what) {
    auto priority = static_cast<caf::message_priority>(what->mid.category());
    auto result = ++last_request_id_;
    return priority == caf::message_priority::normal
               ? result
               : result.with_high_priority();
  }

  void Relay(caf::mailbox_element_ptr &what, caf::execution_unit *host,
             caf::upgrade_lock<caf::detail::shared_spinlock> &guard) {
    auto worker = Policy(home_system(), workers_, what, host);
    ++loads_[worker];
    auto [new_id, _] = PlaceTicket(what);
    guard.unlock();
    // replace sender with load_balancer
    worker->enqueue(ctrl(), new_id, what->move_content_to_message(), host);
    return;
  }

  size_t GetWorkerLoad(const caf::actor &actor) {
    auto it = loads_.find(actor);
    return it == loads_.end() ? 0 : it->second;
  }

  caf::actor Policy(caf::actor_system &system,
                    const std::vector<caf::actor> &actors,
                    caf::mailbox_element_ptr &mail, caf::execution_unit *host) {
    CAF_ASSERT(!actors.empty());
    /* Implement a round robin with std::rotated, note that its complexity is
     * O(n). */
    std::vector<caf::actor> candidates(actors.size());
    rotated_ = (rotated_ + 1) % actors.size();
    auto pivot = actors.begin() + rotated_;
    std::rotate_copy(actors.begin(), pivot, actors.end(), candidates.begin());
    auto it = std::min_element(candidates.begin(), candidates.end(),
                               [this](auto &a, auto &b) {
                                 return GetWorkerLoad(a) < GetWorkerLoad(b);
                               });
    return *it;
  }

  bool Filter(caf::upgrade_lock<caf::detail::shared_spinlock> &guard,
              const caf::mailbox_element_ptr &what, caf::execution_unit *eu) {
    const auto &sender = what->sender;
    const auto &mid = what->mid;
    const auto &content = what->content();
    CAF_LOG_TRACE(CAF_ARG(mid) << CAF_ARG(content));
    if (content.match_elements<caf::exit_msg>()) {
      Exit(guard, eu, content.get_as<caf::exit_msg>(0));
      return true;
    }
    if (content.match_elements<caf::down_msg>()) {
      Down(guard, eu, content.get_as<caf::down_msg>(0));
      return true;
    }
    if (content.match_elements<caf::sys_atom, caf::put_atom, caf::actor>()) {
      AddWorker(guard, content.get_as<caf::actor>(2));
      return true;
    }
    if (content.match_elements<caf::sys_atom, caf::delete_atom, caf::actor>()) {
      DeleteWorker(guard, content.get_as<caf::actor>(2));
      return true;
    }
    if (content.match_elements<caf::sys_atom, caf::delete_atom>()) {
      ClearWorker(guard);
      return true;
    }
    if (content.match_elements<caf::sys_atom, caf::get_atom>()) {
      auto workers = workers_;
      guard.unlock();
      ListWorkers(sender, mid.response_id(), eu, workers);
      return true;
    }
    if (mid.is_request() && sender != nullptr && workers_.empty()) {
      guard.unlock();
      Ignore(sender, mid.response_id(), eu);
      return true;
    }
    return false;
  }

  void Ignore(const caf::strong_actor_ptr &sender, caf::message_id mid,
              caf::execution_unit *eu) const {
    // Tell client we have ignored this request message by sending and empty
    // message back.
    sender->enqueue(nullptr, mid, caf::message{}, eu);
  }

  void ListWorkers(const caf::strong_actor_ptr &sender, caf::message_id mid,
                   caf::execution_unit *eu,
                   const std::vector<caf::actor> &cpy) const {
    sender->enqueue(nullptr, mid, make_message(std::move(cpy)), eu);
  }

  void ClearWorker(caf::upgrade_lock<caf::detail::shared_spinlock> &guard) {
    caf::upgrade_to_unique_lock<caf::detail::shared_spinlock> unique_guard{
        guard};
    for (auto &worker : workers_) {
      caf::default_attachable::observe_token tk{
          address(), caf::default_attachable::monitor};
      worker->detach(tk);
    }
    workers_.clear();
  }

  void DeleteWorker(caf::upgrade_lock<caf::detail::shared_spinlock> &guard,
                    const caf::actor &worker) {
    caf::upgrade_to_unique_lock<caf::detail::shared_spinlock> unique_guard{
        guard};
    auto last = workers_.end();
    auto i = std::find(workers_.begin(), last, worker);
    if (i != last) {
      caf::default_attachable::observe_token tk{
          address(), caf::default_attachable::monitor};
      worker->detach(tk);
      workers_.erase(i);
    }
  }

  void AddWorker(caf::upgrade_lock<caf::detail::shared_spinlock> &guard,
                 const caf::actor &worker) {
    worker->attach(
        caf::default_attachable::make_monitor(worker.address(), address()));
    caf::upgrade_to_unique_lock<caf::detail::shared_spinlock> unique_guard{
        guard};
    workers_.push_back(worker);
  }

  void Down(caf::upgrade_lock<caf::detail::shared_spinlock> &guard,
            caf::execution_unit *eu,
            const caf::down_msg &dm) {  // remove failed worker from pool
    caf::upgrade_to_unique_lock<caf::detail::shared_spinlock> unique_guard{
        guard};
    auto last = workers_.end();
    auto i = std::find(workers_.begin(), workers_.end(), dm.source);
    CAF_LOG_DEBUG_IF(i == last, "received down message for an unknown worker");
    if (i != last) workers_.erase(i);
    if (workers_.empty()) {
      planned_reason_ = caf::exit_reason::out_of_workers;
      unique_guard.unlock();
      // we can safely run our cleanup code here without holding
      // workers_mtx_ because abstract_actor has its own lock
      if (cleanup(planned_reason_, eu)) {
        unregister_from_system();
      }
    }
  }

  void Exit(caf::upgrade_lock<caf::detail::shared_spinlock> &guard,
            caf::execution_unit *eu, const caf::exit_msg &exit_message) {
    // acquire second mutex as well
    std::vector<caf::actor> workers;
    auto reason = exit_message.reason;
    if (cleanup(std::move(reason), eu)) {
      // send exit messages *always* to all workers and clear vector
      // afterwards but first swap workers_ out of the critical section
      caf::upgrade_to_unique_lock<caf::detail::shared_spinlock> unique_guard{
          guard};
      workers_.swap(workers);
      unique_guard.unlock();
      const auto message = make_message(exit_message);
      for (const auto &worker : workers) {
        anon_send(worker, message);
      }
      unregister_from_system();
    }
  }

  using Loads = std::unordered_map<caf::actor, size_t>;
  Loads loads_;
  using Tickets = std::unordered_map<caf::message_id, Ticket>;
  Tickets tickets_;
  caf::message_id last_request_id_ = {caf::make_message_id()};
  size_t rotated_{std::numeric_limits<size_t>::max()};
  caf::detail::shared_spinlock workers_mtx_;
  std::vector<caf::actor> workers_;
  caf::exit_reason planned_reason_;
};

}  // namespace cdcf
#endif  // ACTOR_SYSTEM_INCLUDE_ACTOR_SYSTEM_LOAD_BALANCER_H_
