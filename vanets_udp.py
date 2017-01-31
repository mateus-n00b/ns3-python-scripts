import ns.applications
import ns.core
import ns.network
import ns.wifi
import ns.mobility
import ns.internet
import ns.wave
import sys,random


if not len(sys.argv[1:]):
    print "Usage: {0} traceFile".format(sys.argv[0])
    sys.exit(0)

traceFile           =           sys.argv[1]
phyMode             =           "OfdmRate6MbpsBW10MHz"


nodes = ns.network.NodeContainer()
nodes.Create(25)

ns.core.LogComponentEnable("UdpEchoClientApplication", ns.core.LOG_LEVEL_INFO)
ns.core.LogComponentEnable("UdpEchoServerApplication", ns.core.LOG_LEVEL_INFO)
# ns.core.LogComponentEnable("Ns2MobilityHelper", ns.core.LOG_LEVEL_DEBUG)


try:
    mobility = ns.mobility.Ns2MobilityHelper(str(traceFile))
except Exception as err:
    print "Error: %s" % str(err)
    sys.exit(0)

mobility.Install()

channel = ns.wifi.YansWifiChannelHelper.Default()
# channel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel")
# channel.AddPropagationLoss("ns3::NakagamiPropagationLossModel")
phy = ns.wifi.YansWifiPhyHelper.Default()
phy.SetChannel(channel.Create())
phy.SetPcapDataLinkType(ns.wifi.YansWifiPhyHelper.DLT_IEEE802_11)
phy.Set("TxGain",ns.core.DoubleValue(4.5))
phy.Set("RxGain",ns.core.DoubleValue(4.5))
# phy.Set("EnergyDetectionThreshold",ns.core.DoubleValue(-100.0))
# phy.EnablePcap("vanets_python",nodes)

wifi80211pMac = ns.wave.NqosWaveMacHelper.Default()
wifi80211p = ns.wave.Wifi80211pHelper.Default()

wifi80211p.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                    "DataMode",ns.core.StringValue(phyMode),
                                    "ControlMode",ns.core.StringValue(phyMode))
devices = wifi80211p.Install(phy,wifi80211pMac,nodes)

internet = ns.internet.InternetStackHelper()
internet.Install(nodes)

ipv4 = ns.internet.Ipv4AddressHelper()
ipv4.SetBase(ns.network.Ipv4Address("192.168.1.0"),ns.network.Ipv4Mask("255.255.255.0"))
i = ns.internet.Ipv4InterfaceContainer()
i = ipv4.Assign(devices)

echoServer = ns.applications.UdpEchoServerHelper(9)

for x in range(0,2):
    serverApps = echoServer.Install(nodes.Get(x))
serverApps.Start(ns.core.Seconds(1.0))
serverApps.Stop(ns.core.Seconds(60.0))

for x in range(3,10):
    sv = random.randint(0,2)
    
    echoClient = ns.applications.UdpEchoClientHelper(i.GetAddress(sv), 9)
    echoClient.SetAttribute("MaxPackets", ns.core.UintegerValue(10))
    echoClient.SetAttribute("Interval", ns.core.TimeValue(ns.core.Seconds(1.0)))
    echoClient.SetAttribute("PacketSize", ns.core.UintegerValue(1024))

    clientApps = echoClient.Install(nodes.Get(x))

# for x in range(2,10):
#     clientApps = echoClient.Install(nodes.Get(x))

clientApps.Start(ns.core.Seconds(2.0))
clientApps.Stop(ns.core.Seconds(60.0))

ns.internet.Ipv4GlobalRoutingHelper.PopulateRoutingTables()
phy.EnablePcap("VANETS",nodes)

ns.core.Simulator.Run()
ns.core.Simulator.Destroy()
