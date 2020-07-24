/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include "./include/yanghui_server.h"

#include <limits>

// yanghui_compare_job_actor::behavior_type yanghui_compare_job_actor_fun(
//    yanghui_compare_job_actor::pointer self) {
//  return {[=](const std::vector<std::vector<int>>& yanghui_data) {
//    int n = yanghui_data.size();
//    int* temp_states = reinterpret_cast<int*>(malloc(sizeof(int) * n));
//    int* states = reinterpret_cast<int*>(malloc(sizeof(int) * n));
//
//    states[0] = 1;
//    states[0] = yanghui_data[0][0];
//    int i, j, k, min_sum = std::numeric_limits<int>::max();
//    for (i = 1; i < n; i++) {
//      for (j = 0; j < i + 1; j++) {
//        if (j == 0) {
//          temp_states[0] = states[0] + yanghui_data[i][j];
//        } else if (j == i) {
//          temp_states[j] = states[j - 1] + yanghui_data[i][j];
//        } else {
//          temp_states[j] =
//              std::min(states[j - 1], states[j]) + yanghui_data[i][j];
//        }
//      }
//
//      for (k = 0; k < i + 1; k++) {
//        states[k] = temp_states[k];
//      }
//    }
//
//    for (j = 0; j < n; j++) {
//      if (states[j] < min_sum) min_sum = states[j];
//    }
//
//    free(temp_states);
//    free(states);
//    return min_sum;
//  }};
//}

// void printRet(int return_value) {
//  printf("call actor return value: %d\n", return_value);
//  // std::cout << "call actor return value:" << return_value << std::endl;
//}

void ErrorHandler(const caf::error& err) {
  std::cout << "call actor get error:" << caf::to_string(err) << std::endl;
}

caf::behavior yanghui_standard_job_actor_fun(
    caf::stateful_actor<yanghui_job_state>* self, ActorGuard* actor_guard) {
  return {[&](const YanghuiData& data) {
            std::cout << "start standard job counting." << std::endl;
            auto yanghui_data = data.data;
            std::cout << "yanghui_standard_job_actor_fun 0" << std::endl;
            //            caf::strong_actor_ptr message_sender =
            //            self->current_sender();
            self->state.message_sender = self->current_sender();
            std::cout << "yanghui_standard_job_actor_fun 1" << std::endl;
            actor_guard->SendAndReceive(
                [&](int result) { self->send(self, result); }, ErrorHandler,
                yanghui_data);
            std::cout << "yanghui_standard_job_actor_fun 2" << std::endl;
          },
          [=](int result) {
            anon_send(caf::actor_cast<caf::actor>(self->state.message_sender),
                      true, result);
          }};
}

caf::behavior yanghui_priority_job_actor_fun(
    caf::stateful_actor<yanghui_job_state>* self, WorkerPool* worker_pool,
    caf::actor dispatcher) {
  return {[&](const YanghuiData& data) {
            std::cout << "start yanghui calculation with priority."
                      << std::endl;
            auto yanghui_data = data.data;
            while (true) {
              std::cout << "waiting for worker" << std::endl;
              if (!worker_pool->IsEmpty()) {
                break;
              }
              std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            std::cout << "yanghui_priority_job_actor_fun 0" << std::endl;
            self->state.message_sender = self->current_sender();
            std::cout << "yanghui_priority_job_actor_fun 1" << std::endl;
            anon_send(dispatcher, yanghui_data);
            std::cout << "yanghui_priority_job_actor_fun 2" << std::endl;
          },
          [=](std::vector<std::pair<bool, int>> result) {
            auto result_pair_1 = result[0];
            auto result_pair_2 = result[1];

            if (result.size() != 2 || !result_pair_1.first ||
                (result_pair_1.first != result_pair_2.first)) {
              anon_send(caf::actor_cast<caf::actor>(self->state.message_sender),
                        false, 0);
            }

            anon_send(caf::actor_cast<caf::actor>(self->state.message_sender),
                      true, result_pair_1.second);
          }};
}

caf::behavior yanghui_load_balance_job_actor_fun(
    caf::stateful_actor<yanghui_job_state>* self,
    caf::actor yanghui_load_balance_count_path,
    caf::actor yanghui_load_balance_get_min) {
  return {[&](const YanghuiData& data) {
            std::cout << "start load balance job counting." << std::endl;
            auto yanghui_data = data.data;
            self->state.message_sender = self->current_sender();
            caf::anon_send(yanghui_load_balance_count_path, yanghui_data);
          },
          [=](const std::vector<int>& count_path_result) {
            caf::anon_send(yanghui_load_balance_get_min, count_path_result);
          },
          [=](int final_result) {
            caf::anon_send(
                caf::actor_cast<caf::actor>(self->state.message_sender), true,
                final_result);
          }};
}

caf::behavior yanghui_router_pool_job_actor_fun(
    caf::stateful_actor<yanghui_job_state>* self, ActorGuard* pool_guard) {
  return {[&](const YanghuiData& data) {
            std::cout << "start router pool job counting." << std::endl;
            auto yanghui_data = data.data;
            //            caf::strong_actor_ptr message_sender =
            //            self->current_sender();
            self->state.message_sender = self->current_sender();
            pool_guard->SendAndReceive(
                [&](int result) { self->send(self, result); }, ErrorHandler,
                yanghui_data);
          },
          [=](int result) {
            anon_send(caf::actor_cast<caf::actor>(self->state.message_sender),
                      true, result);
          }};
}