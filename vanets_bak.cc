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
#include "ns3/flow-monitor-module.h"

#include <iostream>

#include "ns3/ocb-wifi-mac.h"
#include "ns3/wifi-80211p-helper.h"
#include "ns3/wave-mac-helper.h"

/*
            Important VARS
*/

// Alter this option to set the acceptable distance to perform the
// offloading
#define GOOD_DISTANCE 100.0
// number of Nodes in the simulation
#define numberOfNodes 25
// Packets lenght
#define packetSize     1040
// Number of surrogates in the simulation
#define numberOfSurrogates 4

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Projeto-Artigo");
/*

                    GLOBAL VARS

*/

NodeContainer c;
NodeContainer surrogates;
NodeContainer clients;
NodeContainer providers;
Ipv4InterfaceContainer ips;
int array[10];
int arrayb[10];
/*

                    FUNCTIONS

*/

void serverHandler(Ptr<Socket> socket) {
  Ptr<Packet> pk = socket->Recv(2024,0);
  std::cout << "[SERVER] Received task\n";
  std::cout << "[SERVER] Sending the reply..." << std::endl;

  socket->Send(Create<Packet> (packetSize),0);
}

void clientHandler(Ptr<Socket> socket) {
  socket->Recv(1024,0);
  std::cout << "[CLIENT] Received results!\n";
}

void serverSide() {
     Address from;

     TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
     InetSocketAddress listen = InetSocketAddress (Ipv4Address::GetAny (), 90);

     for (size_t i = 0; i < surrogates.GetN(); i++) {
        Ptr<Socket> server_side = Socket::CreateSocket(surrogates.Get(i),tid);
        server_side->Bind(listen);
        server_side->Listen();
        server_side->SetRecvCallback(MakeCallback(&serverHandler));

     }
}

void clientSide(uint32_t index) {
    // Address from;
    TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
    std::cout << "\t\t[CLIENT] Starting the client!" << std::endl;
    std::cout << "[SYSTEM] Number of providers: "<< providers.GetN() << std::endl;

    if (providers.GetN()>0){
                std::cout << "Assert: TRUE" << std::endl; // For my purposes
                InetSocketAddress end = InetSocketAddress (ips.GetAddress(index), 90);
                Ptr<Socket> client_side = Socket::CreateSocket(clients.Get(0),tid);
                client_side->Connect(end);
                std::cout << "[CLIENT] Sending tasks..." << std::endl;
                client_side->Send(Create<Packet> (packetSize),0);
                client_side->SetRecvCallback(MakeCallback(&clientHandler));
                // checkArr[i] = arrayb[i];
    }else{
          std::cout << "[CLIENT] No good option was found!" << std::endl;
    }
}

// Function to receive "hello" packets
void listenForRequests(Ptr<Socket> socket) {
    Address from;

    Ptr<Packet> packet= socket->RecvFrom(from);
    // Show the content
    uint8_t *buff = new uint8_t[packet->GetSize()];
    packet->CopyData(buff,packet->GetSize());

    Ipv4Address ipv4From = InetSocketAddress::ConvertFrom(from).GetIpv4();

    std::string data = std::string((char*)buff);

    std::cout << "[SERVER] Message received: "<< data << std::endl;

    std::cout << "[SERVER] Received request from: "<< ipv4From << std::endl;
    std::cout << "[SERVER] Sending the reply..." << std::endl;
    socket->Connect(InetSocketAddress(ipv4From,5000));

    socket->Send(Create<Packet> (60));
    Simulator::Schedule(Simulator::Now(),serverSide);
}

void SendPkt(Ptr<Socket> socket) {
    std::stringstream msgx;
    msgx << "Hello Mateus";
    Ptr<Packet> pkt = Create<Packet>((uint8_t*) msgx.str().c_str(), packetSize);
    socket->Send(pkt);

}

// Returns the speed of a node
double GetSpeed(Ptr<const MobilityModel> model1){
      return model1->GetVelocity().x;
}

// Get distance from a node1 to a node2
double GetDistance(Ptr<const MobilityModel> model1,Ptr<const MobilityModel> model2){
        return model1->GetDistanceFrom (model2);
}

bool sameDirection(Ptr<const MobilityModel> model1,Ptr<const MobilityModel> model2){
     // Moving to the north
     if ((model1->GetPosition().x > 0 && model1->GetPosition().y == 0)
         && (model2->GetPosition().x > 0 && model2->GetPosition().y == 0)) {
          return true;
     }
    //  Moving to the south
     else if ((model1->GetPosition().x < 0 && model1->GetPosition().y == 0)
         && (model2->GetPosition().x < 0 && model2->GetPosition().y == 0)) {
         return true;
     }
    //  Moving to the west
     else if ((model1->GetPosition().x == 0 && model1->GetPosition().y > 0)
         && (model2->GetPosition().x == 0 && model2->GetPosition().y > 0)) {
         return true;
     }
     //  Moving to the east
     else if ((model1->GetPosition().x == 0 && model1->GetPosition().y < 0)
         && (model2->GetPosition().x == 0 && model2->GetPosition().y < 0)) {
         return true;
     }
     else{
        return false;
     }

}

// Receive the reply packets and make the surrogate's choice
void RecRepPkt(Ptr<Socket> socket) {
  Address from;
  Ptr<Packet> packet= socket->RecvFrom(from);
  Ipv4Address ipv4From = InetSocketAddress::ConvertFrom(from).GetIpv4();

  std::cout << "[CLIENT] Received reply from: "<< ipv4From << std::endl;

  // Returns the distance between the client and the surrogates
  for (size_t i = 0; i < surrogates.GetN(); i++) {

      if (ipv4From == ips.GetAddress(array[i])){
          Ptr<MobilityModel> model1 = clients.Get(0)->GetObject<MobilityModel>();
          Ptr<MobilityModel> model2 = surrogates.Get(i)->GetObject<MobilityModel>();

          double distance = GetDistance(model1,model2);

          double mySpeed = GetSpeed(model1);
          double surrogateSpeed = GetSpeed(model2);

          std::cout << "\n[SYSTEM] Distance from client to server "<< ips.GetAddress(array[i]) << " == "
          << distance
          << std::endl;
          std::cout << "[SYSTEM] Speed of the client: " << mySpeed << std::endl;
          std::cout << "[SYSTEM] Speed of the surrogate: "<< surrogateSpeed << std::endl;
          std::cout << "POS X: "<< model1->GetPosition().x << std::endl;
          std::cout << "POS Y: "<< model1->GetPosition().y << std::endl;

          // Desicion making
          if ((sameDirection(model1,model2) || distance <= GOOD_DISTANCE) ){//&& mySpeed >= surrogateSpeed) {
              providers.Add(surrogates.Get(i));
              arrayb[i] = array[i];
              std::cout << "\n[SYSTEM] Surrogate "<< ips.GetAddress(array[i]) << " choosed!\n" << std::endl;
              Simulator::Schedule(Simulator::Now(),clientSide,arrayb[i]);
          }
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

  // Create the nodes
  c.Create(numberOfNodes);

  ns2.Install ();
  Config::Connect ("/NodeList/*/$ns3::MobilityModel/CourseChange",
                   MakeBoundCallback (&CourseChange, &os));

  // The below set of helpers will help us to put together the wifi NICs we want
  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();

  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::NakagamiPropagationLossModel");
  // wifiChannel.AddPropagationLoss("ns3::RangePropagationLossModel", "MaxRange",DoubleValue(100.0));


  Ptr<YansWifiChannel> channel = wifiChannel.Create ();
  wifiPhy.SetChannel (channel);

  // ns-3 supports generate a pcap trace
  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11);
  wifiPhy.Set("TxGain",DoubleValue(4.5));
  wifiPhy.Set("RxGain",DoubleValue(4.5));
  wifiPhy.Set("EnergyDetectionThreshold",DoubleValue(-100.0));

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

  // DHCP service
  Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  ips = ipv4.Assign (devices);

  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  // Crio socket cliente
  uint32_t the_client = rand() % numberOfNodes + 0;
  std::cout << "[SYSTEM] The client ==> "<< ips.GetAddress(the_client) << std::endl;
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
  // uint32_t s = 0;
  for (size_t x = 0; x < numberOfSurrogates; x++) {
      uint32_t s = rand() % numberOfNodes + 0;
      while (s == the_client) {
            s = rand() % numberOfNodes + 0;
      }

      std::cout << "[SYSTEM] Server online ==> "<< ips.GetAddress(s) << std::endl;

      surrogates.Add(c.Get(s));
      array[x] = s;

      Ptr<Socket> sink = Socket::CreateSocket (c.Get (s), tid);
      sink->Bind(local);
      sink->SetRecvCallback(MakeCallback(&listenForRequests));
  }

  std::cout << "[SYSTEM] Number of surrogates: "<< surrogates.GetN() << std::endl;

  // Animacao gerada para analise
  AnimationInterface anim ("vanets-animation.xml");

  // Habilitando trace
  AsciiTraceHelper ascii;
  wifiPhy.EnableAsciiAll (ascii.CreateFileStream ("resultados.tr"));

  wifiPhy.EnablePcap ("client-80211p", devices.Get(the_client),true);

  // Pcap for surrogates
  for (size_t i = 0; i < numberOfSurrogates; i++) {
    wifiPhy.EnablePcap("server-80211p",devices.Get(array[i]),true);
  }


  // Agendo a execucao da acao
  uint32_t t = rand() % 50 + 10;
  std::cout << "\t\t[SYSTEM] Execution started at "<< t << " seconds" << std::endl;
  Simulator::Schedule(Seconds(t),SendPkt,client);

  Simulator::Stop (Seconds (300.0));
  Simulator::Run ();
  Simulator::Destroy ();

  os.close (); // close log file
  return 0;
}
