/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef DEMOS_YANGHUI_CLUSTER_YANGHUI_CONFIG_H_
#define DEMOS_YANGHUI_CLUSTER_YANGHUI_CONFIG_H_
#include <actor_system.h>

#include <string>
#include <vector>

struct NumberCompareData {
  std::vector<int> numbers;
  int index;
};

template <class Inspector>
typename Inspector::result_type inspect(Inspector& f,
                                        const NumberCompareData& x) {
  return f(caf::meta::type_name("NumberCompareData"), x.numbers, x.index);
}

using calculator =
    caf::typed_actor<caf::replies_to<int, int>::with<int>,
                     caf::replies_to<NumberCompareData>::with<int>>;

calculator::behavior_type calculator_fun(calculator::pointer self) {
  return {[=](int a, int b) -> int {
            caf::aout(self) << "received add task. input a:" << a << " b:" << b
                            << std::endl;

            int result = a + b;
            caf::aout(self) << "return: " << result << std::endl;
            return result;
          },
          [=](NumberCompareData& data) -> int {
            if (data.numbers.empty()) {
              caf::aout(self) << "get empty compare" << std::endl;
              return 999;
            }

            int result = data.numbers[0];

            caf::aout(self) << "received compare task, input: ";

            for (int number : data.numbers) {
              caf::aout(self) << number << " ";
              if (number < result) {
                result = number;
              }
            }

            caf::aout(self) << std::endl;
            caf::aout(self) << "return: " << result << std::endl;

            return result;
          }};
}

calculator::behavior_type sleep_calculator_fun(calculator::pointer self,
                                               std::atomic_int& deal_msg_count,
                                               int sleep_micro) {
  return {
      [self, &deal_msg_count, sleep_micro](int a, int b) -> int {
        ++deal_msg_count;
        caf::aout(self) << "slow calculator received add task. input a:" << a
                        << " b:" << b
                        << ", ************* calculator sleep microseconds:"
                        << sleep_micro << " msg count:" << deal_msg_count
                        << std::endl;
        if (sleep_micro) {
          std::this_thread::sleep_for(std::chrono::microseconds(sleep_micro));
        }

        int result = a + b;
        caf::aout(self) << "return: " << result << std::endl;
        return result;
      },
      [self, &deal_msg_count, sleep_micro](NumberCompareData& data) -> int {
        ++deal_msg_count;
        if (data.numbers.empty()) {
          caf::aout(self) << "get empty compare" << std::endl;
          return 999;
        }

        int result = data.numbers[0];

        caf::aout(self) << "received compare task, input: ";

        for (int number : data.numbers) {
          caf::aout(self) << number << " ";
          if (number < result) {
            result = number;
          }
        }

        caf::aout(self) << "************* calculator sleep microseconds:"
                        << sleep_micro << " msg count:" << deal_msg_count
                        << std::endl;

        if (sleep_micro) {
          std::this_thread::sleep_for(std::chrono::microseconds(sleep_micro));
        }

        caf::aout(self) << "return: " << result << std::endl;

        return result;
      }};
}

class typed_calculator : public calculator::base {
  // public:
  //  explicit typed_calculator(caf::actor_config& cfg) : calculator::base(cfg)
  //  {}
  //
  //  behavior_type make_behavior() override { return sleep_calculator_fun(this,
  //  deal_msg_count, 0); }
  //
  //  std::atomic_int deal_msg_count = 0;
 public:
  explicit typed_calculator(caf::actor_config& cfg) : calculator::base(cfg) {}

  behavior_type make_behavior() override {
    return sleep_calculator_fun(this, deal_msg_count, 0);
  }

  std::atomic_int deal_msg_count = 0;
};

class typed_slow_calculator : public calculator::base {
 public:
  explicit typed_slow_calculator(caf::actor_config& cfg)
      : calculator::base(cfg) {}

  behavior_type make_behavior() override {
    return sleep_calculator_fun(this, deal_msg_count, 800);
  }

  std::atomic_int deal_msg_count = 0;
};

class config : public actor_system::Config {
 public:
  uint16_t root_port = 0;
  std::string root_host = "localhost";
  uint16_t worker_port = 0;
  uint16_t node_keeper_port = 0;
  bool root = false;
  bool balance_mode = false;

  config() {
    add_actor_type("calculator", calculator_fun);
    opt_group{custom_options_, "global"}
        .add(root_port, "root_port", "set root port")
        .add(root_host, "root_host", "set root node")
        .add(worker_port, "worker_port, w", "set worker port")
        .add(root, "root, r", "set current node be root")
        .add(node_keeper_port, "node_port", "set node keeper port")
        .add(balance_mode, "balance_mode, b",
             "set current cluster mode, only work when root node");
    add_message_type<NumberCompareData>("NumberCompareData");
  }

  const std::string kResultGroupName = "result";
  const std::string kCompareGroupName = "compare";
};

#endif  // DEMOS_YANGHUI_CLUSTER_YANGHUI_CONFIG_H_
