/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef DEMOS_YANGHUI_CLUSTER_INCLUDE_YANGHUI_DEMO_CALCULATOR_H_
#define DEMOS_YANGHUI_CLUSTER_INCLUDE_YANGHUI_DEMO_CALCULATOR_H_
#include <condition_variable>

#include <caf/all.hpp>

#include "../../../message_priority_actor/include/message_priority_actor.h"
#include "../yanghui_config.h"
#include "./count_cluster.h"

calculator::behavior_type calculator_fun(calculator::pointer self);
calculator::behavior_type sleep_calculator_fun(calculator::pointer self,
                                               std::atomic_int& deal_msg_count,
                                               int sleep_micro);

#endif  // DEMOS_YANGHUI_CLUSTER_INCLUDE_YANGHUI_DEMO_CALCULATOR_H_
