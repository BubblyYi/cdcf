/*
 * Copyright (c) 2020 ThoughtWorks Inc.
 */

#ifndef CDCF_YANGHUI_SERVER_H
#define CDCF_YANGHUI_SERVER_H

#include <caf/all.hpp>

#include "../../actor_fault_tolerance/include/actor_guard.h"
#include "./yanghui_with_priority.h"

const uint16_t yanghui_job_port1 = 55011;
const uint16_t yanghui_job_port2 = 55012;
const uint16_t yanghui_job_port3 = 55013;
const uint16_t yanghui_job_port4 = 55014;

struct yanghui_job_state {
  caf::strong_actor_ptr message_sender;
};

/**
 *  yanghui_standard_job_actor
 */
using yanghui_standard_job_actor =
    caf::typed_actor<caf::reacts_to<std::vector<std::vector<int>>>,
                     caf::reacts_to<caf::strong_actor_ptr, int>>;

yanghui_standard_job_actor::behavior_type yanghui_standard_job_actor_fun(
    yanghui_standard_job_actor::pointer self, ActorGuard* actor_guard);

/**
 *  yanghui_priority_job_actor
 */

using yanghui_priority_job_actor =
    caf::typed_actor<caf::reacts_to<std::vector<std::vector<int>>>,
                     caf::reacts_to<std::vector<std::pair<bool, int>>>>;

yanghui_priority_job_actor::behavior_type yanghui_priority_job_actor_fun(
    yanghui_priority_job_actor::stateful_pointer<yanghui_job_state> self,
    WorkerPool* worker_pool, caf::actor dispatcher);

/**
 *  yanghui_load_balance_job_actor
 */

using yanghui_load_balance_job_actor =
    caf::typed_actor<caf::reacts_to<std::vector<std::vector<int>>>,
                     caf::reacts_to<std::vector<int>>, caf::reacts_to<int>>;

yanghui_load_balance_job_actor::behavior_type
yanghui_load_balance_job_actor_fun(
    yanghui_load_balance_job_actor::stateful_pointer<yanghui_job_state> self,
    caf::actor yanghui_load_balance_count_path,
    caf::actor yanghui_load_balance_get_min);
/**
 *  yanghui_router_pool_job_actor
 */
using yanghui_router_pool_job_actor =
    caf::typed_actor<caf::reacts_to<std::vector<std::vector<int>>>,
                     caf::reacts_to<caf::strong_actor_ptr, int>>;

yanghui_router_pool_job_actor::behavior_type yanghui_router_pool_job_actor_fun(
    yanghui_router_pool_job_actor::pointer self, ActorGuard* pool_guard);

/**
 *  yanghui_compare_job_actor
 */

using yanghui_compare_job_actor =
    caf::typed_actor<caf::replies_to<std::vector<std::vector<int>>>::with<int>>;

yanghui_compare_job_actor::behavior_type yanghui_compare_job_actor_fun(
    yanghui_compare_job_actor::pointer self);

#endif  // CDCF_YANGHUI_SERVER_H