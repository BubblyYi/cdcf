/*
 * Copyright (c) 2019 ThoughtWorks Inc.
 */
#include <cmath>
#include <vector>

#include "../include/gossip.h"
#include "../include/membership.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "include/membership_message.h"
#include "include/mock_gossip.h"

using namespace testing;

// Member
bool CompareMembers(const std::vector<membership::Member> &lhs,
                    const std::vector<membership::Member> &rhs) {
  if (lhs.size() != rhs.size()) {
    return false;
  }

  for (int i = 0; i < lhs.size(); ++i) {
    if (lhs[i] != rhs[i]) {
      return false;
    }
  }

  return true;
}

TEST(Membership, CreateOneMember) {
  membership::Member member("node1", "127.0.0.1", 27777);

  EXPECT_EQ(member.GetNodeName(), "node1");
  EXPECT_EQ(member.GetIpAddress(), "127.0.0.1");
  EXPECT_EQ(member.GetPort(), 27777);
}

TEST(Membership, MemberComparisonEqual) {
  membership::Member member1("node1", "127.0.0.1", 27777);
  membership::Member member2("node1", "127.0.0.1", 27777);

  EXPECT_EQ(member1, member2);
}

TEST(Membership, MemberComparisonNotEqual) {
  membership::Member member1("node1", "127.0.0.1", 27777);
  membership::Member member2("node1", "127.0.0.1", 27788);

  EXPECT_NE(member1, member2);
}

// Config
TEST(Membership, ConfigWithHost) {
  membership::Config config;
  config.AddHostMember("node1", "127.0.0.1", 27777);

  membership::Member member("node1", "127.0.0.1", 27777);

  EXPECT_EQ(config.GetHostMember(), member);
}

TEST(Membership, ConfigWithSeedMember) {
  membership::Config config;
  config.AddOneSeedMember("node1", "127.0.0.1", 27777);
  std::vector<membership::Member> seed_members = config.GetSeedMembers();

  membership::Member seed_member1("node1", "127.0.0.1", 27777);
  std::vector<membership::Member> seed_members_compare;
  seed_members_compare.push_back(seed_member1);

  EXPECT_TRUE(CompareMembers(seed_members, seed_members_compare));
}

TEST(Membership, ConfigWithTransport) {
  MockTransport transport;
  membership::Config config;

  config.AddTransport(&transport);
  EXPECT_EQ(config.GetTransport(), &transport);
}

// Message
TEST(Message, CreateUpMessage) {
  membership::Message message1;
  membership::Member member{"node2", "127.0.0.1", 28888};
  message1.InitAsUpMessage(member, 1);
  std::string serialized_msg = message1.SerializeToString();

  membership::Message message2;
  message2.DeserializeFromString(serialized_msg);
  EXPECT_TRUE(message2.IsUpMessage());
  EXPECT_EQ(message2.GetMember(), member);
  EXPECT_EQ(message2.GetIncarnation(), 1);
}

TEST(Message, MessageParseFromArray) {
  membership::Message message1;
  membership::Member member{"node2", "127.0.0.1", 28888};
  message1.InitAsUpMessage(member, 1);
  std::string serialized_msg = message1.SerializeToString();

  membership::Message message2;
  message2.DeserializeFromArray(serialized_msg.data(), serialized_msg.size());
  EXPECT_TRUE(message2.IsUpMessage());
  EXPECT_EQ(message2.GetMember(), member);
  EXPECT_EQ(message2.GetIncarnation(), 100);
}

// Membership
TEST(Membership, CreateHostMember) {
  membership::Membership my_membership;
  membership::Member member("node1", "127.0.0.1", 27777);

  membership::Config config;
  config.AddHostMember("node1", "127.0.0.1", 27777);

  my_membership.Init(config);

  std::vector<membership::Member> members = my_membership.GetMembers();
  EXPECT_EQ(members.size(), 1);
  EXPECT_EQ(members[0], member);
}

TEST(Membership, CreateHostMemberWithEmptyConfig) {
  membership::Membership my_membership;
  membership::Member member("node1", "127.0.0.1", 27777);

  membership::Config config;

  EXPECT_EQ(my_membership.Init(config),
            membership::MEMBERSHIP_INIT_HOSTMEMBER_EMPTY);
}

// TEST(Membership, CreateHostMemberWithTransport) {
//  membership::Membership my_membership;
//
//  membership::Config config;
//  config.AddHostMember("node1", "127.0.0.1", 27777);
//  MockTransport transport;
//  config.AddTransport(&transport);
//
//  my_membership.Init(config);
//}

int InitBasicMembership(membership::Membership &new_membership,
                        gossip::Transportable &transport) {
  membership::Config config;
  config.AddHostMember("node1", "127.0.0.1", 27777);
  config.AddTransport(&transport);

  return new_membership.Init(config);
}

void SimulateReceivingUpMessage(const membership::Member& member,
                                MockTransport& transport) {
  gossip::Address address{member.GetIpAddress(), member.GetPort()};

  membership::Message message;
  message.InitAsUpMessage(member, 1);
  std::string serialized_msg = message.SerializeToString();
  gossip::Payload payload(serialized_msg);

  transport.CallGossipHandler(address, payload);
}

void SimulateReceivingDownMessage(const membership::Member& member,
                                  MockTransport& transport) {
  gossip::Address address{member.GetIpAddress(), member.GetPort()};

  membership::Message message;
  message.InitAsDownMessage(member, 1);
  std::string serialized_msg = message.SerializeToString();
  gossip::Payload payload(serialized_msg);

  transport.CallGossipHandler(address, payload);
}

TEST(Membership, NewUpMessageReceived) {
  membership::Membership my_membership;
  MockTransport transport;
  InitBasicMembership(my_membership, transport);

  membership::Member member{"node2", "127.0.0.1", 28888};
  SimulateReceivingUpMessage(member, transport);

  std::vector<membership::Member> members{{"node1", "127.0.0.1", 27777},
                                          {"node2", "127.0.0.1", 28888}};

  EXPECT_TRUE(CompareMembers(my_membership.GetMembers(), members));
}

TEST(Membership, DuplicateUpMessageReceived) {
  membership::Membership my_membership;
  MockTransport transport;
  InitBasicMembership(my_membership, transport);

  membership::Member member{"node2", "127.0.0.1", 28888};
  EXPECT_CALL(transport, Gossip(_, _, _)).Times(AnyNumber());
  SimulateReceivingUpMessage(member, transport);
  SimulateReceivingUpMessage(member, transport);
  SimulateReceivingUpMessage(member, transport);

  std::vector<membership::Member> members{{"node1", "127.0.0.1", 27777},
                                          {"node2", "127.0.0.1", 28888}};

  EXPECT_TRUE(CompareMembers(my_membership.GetMembers(), members));
}

TEST(Membership, NewDownMessageReceived) {
  membership::Membership my_membership;
  MockTransport transport;
  InitBasicMembership(my_membership, transport);

  membership::Member member{"node2", "127.0.0.1", 28888};
  EXPECT_CALL(transport, Gossip(_, _, _)).Times(AnyNumber());
  SimulateReceivingUpMessage(member, transport);

  std::vector<membership::Member> members{{"node1", "127.0.0.1", 27777},
                                          {"node2", "127.0.0.1", 28888}};

  EXPECT_TRUE(CompareMembers(my_membership.GetMembers(), members));

  membership::Member memberDowned{"node2", "127.0.0.1", 27777};
  SimulateReceivingDownMessage(memberDowned, transport);
  SimulateReceivingDownMessage(memberDowned, transport);

  std::vector<membership::Member> membersAfterDown{
      {"node1", "127.0.0.1", 28888}};
  EXPECT_TRUE(CompareMembers(my_membership.GetMembers(), membersAfterDown));
}

TEST(Membership, UpMessageRetransmitWithThreeMember) {
  // start a member
  membership::Membership node;
  membership::Config config;
  MockTransport transport;
  config.AddHostMember("node1", "127.0.0.1", 27777);
  config.AddTransport(&transport);
  config.AddRetransmitMultiplier(3);
  node.Init(config);

  int retransmit_limit_one_member =
      config.GetRetransmitMultiplier() * ceil(log(1));
  int retransmit_limit_two_member =
      config.GetRetransmitMultiplier() * ceil(log(2));
  int retransmit_limit_three_member =
      config.GetRetransmitMultiplier() * ceil(log(3));

  EXPECT_CALL(transport, Gossip(_, _, _))
      .Times(retransmit_limit_one_member + retransmit_limit_two_member +
             retransmit_limit_three_member);

  SimulateReceivingUpMessage({"node2", "127.0.0.1", 28888}, transport);
  EXPECT_TRUE(CompareMembers(
      node.GetMembers(),
      {{"node1", "127.0.0.1", 27777}, {"node2", "127.0.0.1", 28888}}));

  SimulateReceivingUpMessage({"node3", "127.0.0.1", 29999}, transport);
  EXPECT_TRUE(
      CompareMembers(node.GetMembers(), {{"node1", "127.0.0.1", 27777},
                                         {"node2", "127.0.0.1", 28888},
                                         {"node3", "127.0.0.1", 29999}}));

  SimulateReceivingUpMessage({"node4", "127.0.0.1", 26666}, transport);
  EXPECT_TRUE(
      CompareMembers(node.GetMembers(), {{"node4", "127.0.0.1", 26666},
                                         {"node1", "127.0.0.1", 27777},
                                         {"node2", "127.0.0.1", 28888},
                                         {"node3", "127.0.0.1", 29999}}));
}

// TEST(Membership, Compare) {
//
//  std::string str1 = "127.0.0.1";
//  std::string str2 = "127.0.0.2";
//
//  EXPECT_TRUE(str1 < str2);
//
//  std::string str3 = "128.0.0.1";
//  std::string str4 = "127.0.0.2";
//
//  EXPECT_TRUE(str3 > str4);
//}

// TEST(Membership, JoinWithSingleNodeCluster) {
//
//  // start a member
//  membership::Membership node;
//  membership::Config config;
//  MockTransport transport;
//  config.AddHostMember("node_a", "127.0.0.1", 27777);
//  config.AddTransport(&transport);
//  config.AddOneSeedMember("node_b", "127.0.0.1", 28888);
//
//  membership::Message message;
//  message.InitAsUpMessage({"node_a", "127.0.0.1", 27777}, 1);
//  std::string serialized_msg = message.SerializeToString();
//  gossip::Payload payload(serialized_msg);
//
//  // should send an up message to seed member
//  EXPECT_CALL(transport,
//  Gossip(UnorderedElementsAre(gossip::Address{"127.0.0.1", 28888}), payload,
//  _)).Times(AtLeast(1)); node.Init(config);
//}

// TEST(Membership, MessageDissemination) {
//
//  // start a member a
//  membership::Membership node_a;
//  membership::Config config_a;
//  MockTransport transport_a;
//  config_a.AddHostMember("node_a", "127.0.0.1", 27777);
//  config_a.AddTransport(&transport_a);
//  node_a.Init(config_a);
//  // start another member b
//  membership::Membership node_b;
//  membership::Config config_b;
//  MockTransport transport_b;
//  config_b.AddHostMember("node_b", "127.0.0.1", 28888);
//  config_b.AddTransport(&transport_b);
//  node_a.Init(config_b);
//  // send an up message to a
//  membership::Member memberDowned{"node_c", "127.0.0.1", 29999};
//  SimulateReceivingDownMessage(memberDowned, transport_a);
//  // detect up being processed on both a & b
//}
