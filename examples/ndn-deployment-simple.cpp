/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

// ndn-simple.cpp

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/ndnSIM-module.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NdnDeploymentSimple");

/**
 * This scenario simulates a very simple network topology:
 *
 *
 *      +-------------+     10Mbps      +---------------+     1.5Mbps    +--------------+      10Mbps
 *      | consumer(0) | <------------>  | ndn router(2) | <------------> | ip router(4) |  <----------> producer(5)
 *      +-------------+ 10ms    +---------------+     100ms       +--------------+      10ms
 *
 *ip client(3)                                                                                  ip server(6)
 *
 *
 * For every received interest, producer replies with a data packet, containing
 * 1024 bytes of virtual payload.
 *
 * To run scenario and see what is happening, use the following command:
 *
 *     NS_LOG=ndn.Consumer:ndn.Producer ./waf --run=ndn-simple
 */

NodeContainer n0n2;
NodeContainer n1n2;
NodeContainer n2n3;
NodeContainer n3n4;
NodeContainer n3n5;

int 
main(int argc, char* argv[])
{
  // setting default parameters for PointToPoint links and channels
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("1Mbps"));
  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("10ms"));
  Config::SetDefault("ns3::QueueBase::MaxSize", StringValue("20p"));

   std::string aredLinkDataRate = "1.5Mbps";
   std::string aredLinkDelay = "20ms";

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.Parse(argc, argv);

  // Creating nodes
  NS_LOG_INFO ("Create nodes");
  NodeContainer nodes;
  nodes.Create(6);
  Names::Add ( "N0", nodes.Get (0));
  Names::Add ( "N1", nodes.Get (1));
  Names::Add ( "N2", nodes.Get (2));
  Names::Add ( "N3", nodes.Get (3));
  Names::Add ( "N4", nodes.Get (4));
  Names::Add ( "N5", nodes.Get (5));
  n0n2 = NodeContainer (nodes.Get (0), nodes.Get (2));
  n1n2 = NodeContainer (nodes.Get (1), nodes.Get (2));
  n2n3 = NodeContainer (nodes.Get (2), nodes.Get (3));
  n3n4 = NodeContainer (nodes.Get (3), nodes.Get (4));
  n3n5 = NodeContainer (nodes.Get (3), nodes.Get (5));

  // Connecting nodes using two links
  NS_LOG_INFO ("Create channels");
  PointToPointHelper p2p;
 
  NetDeviceContainer devn0n2;
  NetDeviceContainer devn1n2;
  NetDeviceContainer devn2n3;
  NetDeviceContainer devn3n4;
  NetDeviceContainer devn3n5;

  devn0n2 = p2p.Install(n0n2); 
  devn1n2 = p2p.Install(n1n2); 
  devn2n3 = p2p.Install(n2n3); 
  devn3n4 = p2p.Install(n3n4); 
  devn3n5 = p2p.Install(n3n5); 
  
  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.InstallAll();

  NS_LOG_INFO ("Set RED params");
  uint32_t meanPktSize = 1000;
  Config::SetDefault ("ns3::RedQueueDisc::MaxSize", StringValue ("1000p"));
  Config::SetDefault ("ns3::RedQueueDisc::MeanPktSize", UintegerValue (meanPktSize));
  Config::SetDefault ("ns3::RedQueueDisc::Wait", BooleanValue (true));
  Config::SetDefault ("ns3::RedQueueDisc::Gentle", BooleanValue (true));
  Config::SetDefault ("ns3::RedQueueDisc::QW", DoubleValue (0.002));
  Config::SetDefault ("ns3::RedQueueDisc::MinTh", DoubleValue (5));
  Config::SetDefault ("ns3::RedQueueDisc::MaxTh", DoubleValue (15));

  Config::SetDefault ("ns3::RedQueueDisc::ARED", BooleanValue (true));
  Config::SetDefault ("ns3::RedQueueDisc::UseHardDrop", BooleanValue (false));
  Config::SetDefault ("ns3::RedQueueDisc::LInterm", DoubleValue (10));
  Config::SetDefault ("ns3::TcpSocketBase::EcnMode", StringValue ("ClassicEcn"));
  Config::SetDefault ("ns3::RedQueueDisc::UseEcn", BooleanValue (true));
 
  QueueDiscContainer queueDiscs;
  // install AQM
  TrafficControlHelper tchPfifo;
  uint16_t handle = tchPfifo.SetRootQueueDisc ("ns3::PfifoFastQueueDisc");
  tchPfifo.AddInternalQueues (handle, 3, "ns3::DropTailQueue", "MaxSize", StringValue ("1000p"));
 
  TrafficControlHelper tchRed;
  tchRed.SetRootQueueDisc ("ns3::RedQueueDisc", "LinkBandwidth", StringValue (aredLinkDataRate),
                           "LinkDelay", StringValue (aredLinkDelay));

  p2p.SetQueue ("ns3::DropTailQueue");
  p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("0ms"));
  devn0n2 = p2p.Install (n0n2);
  NS_LOG_INFO ("install tchPfifo n0n2");
  tchPfifo.Install (devn0n2);
  NS_LOG_INFO ("after");
 
  p2p.SetQueue ("ns3::DropTailQueue");
  p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("1ms"));
  devn1n2 = p2p.Install (n1n2);
  tchPfifo.Install (devn1n2);
 
  p2p.SetQueue ("ns3::DropTailQueue");
  p2p.SetDeviceAttribute ("DataRate", StringValue (aredLinkDataRate));
  p2p.SetChannelAttribute ("Delay", StringValue ("100ms"));
  devn2n3 = p2p.Install (n2n3);
  // only backbone link has ARED queue disc
  queueDiscs = tchRed.Install (devn2n3);
 
  p2p.SetQueue ("ns3::DropTailQueue");
  p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
  devn3n4 = p2p.Install (n3n4);
  tchPfifo.Install (devn3n4);
 
  p2p.SetQueue ("ns3::DropTailQueue");
  p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("3ms"));
  devn3n5 = p2p.Install (n3n5);
  tchPfifo.Install (devn3n5);

  // ndn-stack-helper update
  ndnHelper.Update(nodes);

  // Choosing forwarding strategy
  ndn::StrategyChoiceHelper::InstallAll("/prefix", "/localhost/nfd/strategy/multicast");

  // Installing applications
  // Consumer
  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  // Consumer will request /prefix/0, /prefix/1, ...
  consumerHelper.SetPrefix("/prefix");
  consumerHelper.SetAttribute("Frequency", StringValue("10")); // 10 interests a second
  auto apps = consumerHelper.Install(nodes.Get(0));                // first node
  apps.Stop(Seconds(10.0)); // stop the consumer app at 10 seconds mark

  // Producer
  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  // Producer will reply to all requests starting with /prefix
  producerHelper.SetPrefix("/prefix");
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  producerHelper.Install(nodes.Get(2)); // last node

  Simulator::Stop(Seconds(20.0));

  Simulator::Run();
  
  QueueDisc::Stats st = queueDiscs.Get (0)->GetStats ();
  if (true) {
     std::cout << "*** ARED stats from Node 2 queue ***" << std::endl;
     std::cout << st << std::endl;
   }

  Simulator::Destroy();

  return 0;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}
