/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*

  Mateus-n00b, Janeiro 2017

  Este script realiza a troca de pacotes entre veiculos, simulando
  o envio das subtarefas a serem executadas nos veiculos substitutos.

  Objetivos concluidos:
      Realizacao da descoberta de surrogates.
      Distancia entre o cliente e os surrogates.

  TODO:
      Algoritmo de escolha de surrogate.
      Enviar dados via socket.

  Version 2.0

  License GPLv3

 */

#include "ns3/vector.h"
#include "ns3/string.h"
#include "ns3/socket.h"
#include "ns3/double.h"
#include "ns3/config.h"
#include "ns3/log.h"
#include "ns3/command-line.h"
#include "ns3/mobility-model.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/position-allocator.h"
#include "ns3/mobility-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/ns2-mobility-helper.h"
#include "ns3/netanim-module.h"

#include <iostream>

#include "ns3/ocb-wifi-mac.h"
#include "ns3/wifi-80211p-helper.h"
#include "ns3/wave-mac-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Projeto-Artigo");
/*

                    GLOBAL VARS

*/

NodeContainer surrogates;
NodeContainer clients;
Ipv4InterfaceContainer ips;
int array[10];

/*

                    FUNCTIONS

*/

// Function to receive "hello" packets
void ReceiveReqPacket(Ptr<Socket> socket) {
    Address from;
    Ptr<Packet> packet= socket->RecvFrom(from);
    Ipv4Address ipv4From = InetSocketAddress::ConvertFrom(from).GetIpv4();

    std::cout << "[+] Received request from: "<< ipv4From << std::endl;
    std::cout << "[!] Sending the reply..." << std::endl;
    socket->Connect(InetSocketAddress(ipv4From,5000));

    socket->Send(Create<Packet> (60));
}

void SendPkt(Ptr<Socket> socket) {
    Ptr<Packet> pkt = Create<Packet> (40);
    socket->Send(pkt);
}

// Get distance from a node1 to a node2
double GetDistance(Ptr<const MobilityModel> model1,Ptr<const MobilityModel> model2){
    double distance = model1->GetDistanceFrom (model2);
    // std::cout << "Distance: "<< distance << std::endl;
    return distance;
}

// Receive the reply packets
void RecRepPkt(Ptr<Socket> socket) {
  Address from;
  Ptr<Packet> packet= socket->RecvFrom(from);
  Ipv4Address ipv4From = InetSocketAddress::ConvertFrom(from).GetIpv4();

  std::cout << "[+] Received reply from: "<< ipv4From << std::endl;

  // Retornando a distancia do cliente ate os surrogates
  for (size_t i = 0; i < surrogates.GetN(); i++) {

      if (ipv4From == ips.GetAddress(array[i])){
      Ptr<MobilityModel> model1 = clients.Get(0)->GetObject<MobilityModel>();
      Ptr<MobilityModel> model2 = surrogates.Get(i)->GetObject<MobilityModel>();

      double distance = GetDistance(model1,model2);
      std::cout << "[!] Distance from client to server "<< ips.GetAddress(array[i]) << " > "
      << distance
      << std::endl;
    }
  }

}

static void
CourseChange (std::ostream *os, std::string foo, Ptr<const MobilityModel> mobility)
{
  Vector pos = mobility->GetPosition (); // Get position
  Vector vel = mobility->GetVelocity (); // Get velocity

  // Prints position and velocities
  *os << Simulator::Now () << " POS: x=" << pos.x << ", y=" << pos.y
      << ", z=" << pos.z << "; VEL:" << vel.x << ", y=" << vel.y
      << ", z=" << vel.z << std::endl;
}

/*
          MAIN FUNCTION
*/

int main (int argc, char *argv[])
{
  std::string phyMode ("OfdmRate6MbpsBW10MHz");
  uint32_t numberOfNodes = 25;
  double interval = 1.0; // seconds

  srand(time(NULL));
  // uint32_t random_number = rand() % numberOfNodes;
  // uint32_t Qserver = rand() % numberOfNodes;

  std::string traceFile;
  std::string logFile;

  // Enable logging from the ns2 helper
  // LogComponentEnable ("Ns2MobilityHelper",LOG_LEVEL_DEBUG);

  // Parse command line attribute
  CommandLine cmd;
  cmd.AddValue ("traceFile", "Ns2 trace de mobilidade", traceFile);
  cmd.AddValue ("logFile", "arquivo de log de mobilidade", logFile);
  cmd.Parse (argc,argv);

  // Check command line arguments
  if (traceFile.empty () || logFile.empty ())
    {
      std::cout << "Usage of " << argv[0] << " :\n\n"
      "./waf --run \"vanets_tcp"
      " --traceFile=src/mobility/examples/default.ns_movements"
      " --logFile=ns2-mob.log\" \n\n"
      "NOTE: ns2-traces-file could be an absolute or relative path. You could use the file default.ns_movements\n"
      "      included in the same directory of this example file.\n\n"
      "NOTE 2: Number of nodes present in the trace file must match with the command line argument and must\n"
      "        be a positive number. Note that you must know it before to be able to load it.\n\n"
      "NOTE 3: Duration must be a positive number. Note that you must know it before to be able to load it.\n\n";

      return 0;
    }

  // Habilitando a mobilidade do ns2 que utiliza os traces gerados pelo SUMO.
  MobilityHelper mobility;
  Ns2MobilityHelper ns2 = Ns2MobilityHelper (traceFile);

  std::ofstream os;
  os.open (logFile.c_str ());
  Time interPacketInterval = Seconds (interval);

  NodeContainer c;
  // c.Create(25);
  c.Create(numberOfNodes);

  ns2.Install ();
  Config::Connect ("/NodeList/*/$ns3::MobilityModel/CourseChange",
                   MakeBoundCallback (&CourseChange, &os));

  // The below set of helpers will help us to put together the wifi NICs we want
  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();

  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::NakagamiPropagationLossModel");
  Ptr<YansWifiChannel> channel = wifiChannel.Create ();
  wifiPhy.SetChannel (channel);

  // ns-3 supports generate a pcap trace
  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11);
  wifiPhy.Set("TxGain",DoubleValue(4.5));
  wifiPhy.Set("RxGain",DoubleValue(4.5));
  wifiPhy.Set("EnergyDetectionThreshold",DoubleValue(-200.0));

  NqosWaveMacHelper wifi80211pMac = NqosWaveMacHelper::Default ();
  Wifi80211pHelper wifi80211p = Wifi80211pHelper::Default ();

  wifi80211p.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                      "DataMode",StringValue (phyMode),
                                      "ControlMode",StringValue (phyMode));
  NetDeviceContainer devices = wifi80211p.Install (wifiPhy, wifi80211pMac, c);
  // Instalando a mobilidade nos nodes
  mobility.Install (c);

  // Instalando a pilha de protocolos
  InternetStackHelper internet;
  internet.Install (c);

  std::vector<NetDeviceContainer> deviceAdjacencyList (numberOfNodes-1);
  std::vector<NodeContainer> nodeAdjacencyList (numberOfNodes-1);

  // servico DHCP
  Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
   ips = ipv4.Assign (devices);


  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  // Crio socket cliente
  uint32_t the_client = rand() % numberOfNodes + 0;
  std::cout << "[!] The client ==> "<< ips.GetAddress(the_client) << std::endl;
  Ptr<Socket> client = Socket::CreateSocket (c.Get (the_client), tid);
  clients.Add(c.Get(the_client));

  // Usado para receber as respostas dos servers
  InetSocketAddress port = InetSocketAddress (Ipv4Address::GetAny (), 5000);

  // Usado para enviar os beacons
  InetSocketAddress broadcast = InetSocketAddress (Ipv4Address("255.255.255.255"), 80);
  client->SetAllowBroadcast(true);
  client->Connect (broadcast);
  client->Bind(port);
  client->SetRecvCallback(MakeCallback(&RecRepPkt));

  // Instalando as applicacoes
  InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 80);

  // Escolhendo os surrogates
  for (size_t x = 0; x < 4; x++) {
      uint32_t s = rand() % numberOfNodes + 0;
      while (s == the_client) {
            s = rand() % numberOfNodes + 0;
      }

      std::cout << "[!] Server online ==> "<< ips.GetAddress(s) << std::endl;

      surrogates.Add(c.Get(s));
      array[x] = s;

      Ptr<Socket> sink = Socket::CreateSocket (c.Get (s), tid);
      sink->Bind(local);
      sink->SetRecvCallback(MakeCallback(&ReceiveReqPacket));
  }

  // std::cout << "[!] Number: "<< ips.GetN() << std::endl;
  std::cout << "[!] Number of surrogates: "<< surrogates.GetN() << std::endl;

  // Animacao gerada para analise
  AnimationInterface anim ("vanets-animation.xml");

  // Habilitando trace
  AsciiTraceHelper ascii;
  wifiPhy.EnableAsciiAll (ascii.CreateFileStream ("resultados.tr"));

  wifiPhy.EnablePcap ("wave-simple-80211p", devices);

  // Agendo a execucao da acao
  uint32_t t = rand() % 60 + 10;
  std::cout << "\t\t[!] Execution started at "<< t << " seconds" << std::endl;
  Simulator::Schedule(Seconds(t),SendPkt,client);

  Simulator::Stop (Seconds (300.0));
  Simulator::Run ();
  Simulator::Destroy ();

  os.close (); // close log file
  return 0;
}
