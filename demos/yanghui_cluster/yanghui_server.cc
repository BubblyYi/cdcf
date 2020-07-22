/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#include "./include/yanghui_server.h"

yanghui_job_actor::behavior_type yanghui_priority_job_actor_fun(
    yanghui_job_actor::pointer self, WorkerPool& worker_pool,
    caf::actor& dispatcher) {
  return {[&](const std::vector<std::vector<int>>& yanghui_data) {
            std::cout << "start yanghui calculation with priority."
                      << std::endl;
            while (true) {
              std::cout << "waiting for worker" << std::endl;
              if (!worker_pool.IsEmpty()) {
                break;
              }
              std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            //    caf::scoped_actor self{system};

            anon_send(dispatcher, yanghui_data);
            //            self->receive(
            //                [=](std::vector<std::pair<bool, int>> result) {
            //                  for (const auto& pair : result) {
            //                    if (pair.first) {
            //                      std::cout
            //                          << "high priority with final result: "
            //                          << pair.second
            //                          << std::endl;
            //                    } else {
            //                      std::cout << "normal priority with final
            //                      result: "
            //                                << pair.second << std::endl;
            //                    }
            //                  }
            //                },
            //                [&](caf::error error) {
            //                  std::cout << "Receive yanghui priority result
            //                  error: "
            //                            << system.render(error) << std::endl;
            //                });
          },
          [=](std::vector<std::pair<bool, int>> result) -> bool {
            auto result_pair_1 = result[0];
            auto result_pair_2 = result[1];

            if (result_pair_1.first != true) {
              // return false
            }
            // TODO: add compare function and return
            return false;
          }};
}

// void printRet(int return_value) {
//  printf("call actor return value: %d\n", return_value);
//  // std::cout << "call actor return value:" << return_value << std::endl;
//}

void dealSendErr(const caf::error& err) {
  std::cout << "call actor get error:" << caf::to_string(err) << std::endl;
}

yanghui_standard_job_actor::behavior_type yanghui_standard_job_actor_fun(
    yanghui_standard_job_actor::pointer self, ActorGuard& actor_guard) {
  return {[&](const std::vector<std::vector<int>>& yanghui_data) {
            caf::aout(self) << "start count." << std::endl;
            //      actor_guard.SendAndReceive(printRet, dealSendErr,
            //      yanghui_data);
            actor_guard.SendAndReceive(
                [&](int return_value) { self->send(self, return_value); },
                dealSendErr, yanghui_data);
          },
          [=](int return_value) {
            // TODO: add compare function and return
            return false;
          }};
}
