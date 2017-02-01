/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/socket.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("FirstScriptExample");

void ReceiveReqPacket(Ptr<Socket> socket) {
    Address from;
    Ptr<Packet> packet= socket->RecvFrom(from);
    Ipv4Address ipv4From = InetSocketAddress::ConvertFrom(from).GetIpv4();

    std::cout << "Received request from: "<< ipv4From << std::endl;
    std::cout << "Sending the reply..." << std::endl;
    socket->Connect(InetSocketAddress(ipv4From,5000));
    
    socket->Send(Create<Packet> (60));

}

void SendPkt(Ptr<Socket> socket) {
    Ptr<Packet> pkt = Create<Packet> (40);
    socket->Send(pkt);
}

void RecRepPkt(Ptr<Socket> socket) {
  Address from;
  Ptr<Packet> packet= socket->RecvFrom(from);
  Ipv4Address ipv4From = InetSocketAddress::ConvertFrom(from).GetIpv4();

  std::cout << "Received reply from: "<< ipv4From << std::endl;
}
int
main (int argc, char *argv[])
{
  Time::SetResolution (Time::NS);

  NodeContainer nodes;
  nodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");

  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 80);
  InetSocketAddress port = InetSocketAddress (Ipv4Address::GetAny (), 5000);
  Ptr<Socket> sink = Socket::CreateSocket (nodes.Get (0), tid);
  sink->Bind(local);
  sink->SetRecvCallback(MakeCallback(&ReceiveReqPacket));

  Ptr<Socket> client = Socket::CreateSocket (nodes.Get (1), tid);

  Ptr<Packet> packet = Create<Packet> (64);
  // InetSocketAddress remote = InetSocketAddress (interfaces.GetAddress (0, 0), 80);
  InetSocketAddress broadcast = InetSocketAddress (Ipv4Address("255.255.255.255"), 80);
  client->SetAllowBroadcast(true);
  client->Connect (broadcast);
  client->Bind(port);
  client->SetRecvCallback(MakeCallback(&RecRepPkt));

  pointToPoint.EnablePcap("sockets",devices);

  // AsciiTraceHelper ascii;
  // pointToPoint.EnablePcapAll(ascii.CreateFileStream("output"));

  Simulator::Schedule(Seconds(5.0),SendPkt,client);

  Simulator::Stop(Seconds(10.0));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
