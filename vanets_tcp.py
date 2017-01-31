import ns.applications
import ns.core
import ns.network
import ns.wifi
import ns.mobility
import ns.internet
import ns.wave
import sys,random
from threading import Thread

def RecPkt(socket):
    print "Chamou"
    while socket.Recv(1024,0):
        print "Received one packet!"

def SndPkt(socket, packetsize, pktcount, pktinterval):
    if pktcount > 0:
        socket.Send(ns.network.Packet(packetsize),0)
        # socket.SendDataPacket(0,packetsize,"Hello")
        ns.core.Simulator.Schedule(ns.core.Seconds(pktinterval), SndPkt, socket, packetsize, pktcount-1, pktinterval)
        print "Sending one packet!"
    else:
        socket.Close()


def  main():
    if not len(sys.argv[1:]):
        print "Usage: {0} traceFile".format(sys.argv[0])
        sys.exit(0)

    traceFile           =           sys.argv[1]
    phyMode             =           "OfdmRate6MbpsBW10MHz"


    nodes = ns.network.NodeContainer()
    nodes.Create(25)

    # ns.core.LogComponentEnable("Ns2MobilityHelper", ns.core.LOG_LEVEL_DEBUG)

    try:
        ns2 = ns.mobility.Ns2MobilityHelper(str(traceFile))
    except Exception as err:
        print "Error: %s" % str(err)
        sys.exit(0)

    mobility = ns.mobility.MobilityHelper()
    # mobility.SetPositionAllocator ("ns3::GridPositionAllocator", "MinX", ns.core.DoubleValue(0.0),
    # 								"MinY", ns.core.DoubleValue (0.0), "DeltaX", ns.core.DoubleValue(5.0), "DeltaY", ns.core.DoubleValue(10.0),
    #                                  "GridWidth", ns.core.UintegerValue(3), "LayoutType", ns.core.StringValue("RowFirst"))
    #
    # mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel")
    mobility.Install(nodes)
    ns2.Install()

    channel = ns.wifi.YansWifiChannelHelper.Default()
    channel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel")
    channel.AddPropagationLoss("ns3::NakagamiPropagationLossModel")
    phy = ns.wifi.YansWifiPhyHelper.Default()
    phy.SetChannel(channel.Create())
    phy.SetPcapDataLinkType(ns.wifi.YansWifiPhyHelper.DLT_IEEE802_11)
    phy.Set("TxGain",ns.core.DoubleValue(4.5))
    phy.Set("RxGain",ns.core.DoubleValue(4.5))
    phy.Set("EnergyDetectionThreshold",ns.core.DoubleValue(-96.0))
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

    interfaces = ipv4.Assign(devices)

    packetsize = 64 #bytes
    pktcount = 2
    pktinterval = 0.25 #seconds
    port = 80



    server = random.randint(0,24)
    print "Server==> %s" % interfaces.GetAddress(server)

    appSink = ns.network.NodeList.GetNode(server)

    for x in range(4):
        client = random.randint(0,24)
        print "Client==> %s" % interfaces.GetAddress(client)
        appSource = ns.network.NodeList.GetNode(client)


        remoteAddr = appSink.GetObject(ns.internet.Ipv4.GetTypeId()).GetAddress(1,0).GetLocal()
        # sink = ns.network.Socket.CreateSocket(appSink, ns.core.TypeId.LookupByName("ns3::UdpSocketFactory"))
        sink = ns.network.Socket.CreateSocket(appSink, ns.core.TypeId.LookupByName("ns3::TcpSocketFactory"))
        sink.Bind(ns.network.InetSocketAddress(ns.network.Ipv4Address.GetAny(), 80))
        sink.Listen()
        # print dir(sink)
        sink.SetRecvCallback(RecPkt)

        # source = ns.network.Socket.CreateSocket(appSource, ns.core.TypeId.LookupByName("ns3::UdpSocketFactory"))
        source = ns.network.Socket.CreateSocket(appSource, ns.core.TypeId.LookupByName("ns3::TcpSocketFactory"))
        source.Connect(ns.network.InetSocketAddress(remoteAddr, port))
        source.SetRecvCallback(RecPkt)

    # sinkApp = ns.network.ApplicationContainer()
    # port = 50000
    # # apLocalAddress = ns.network.Address (ns.network.InetSocketAddress (ns.network.Ipv4Address.GetAny (), port))
    # for x in range(0,6):
    #     apLocalAddress = ns.network.Address (ns.network.InetSocketAddress (ns.network.Ipv4Address.GetAny (), port))
    #     packetSinkHelper = ns.applications.PacketSinkHelper ("ns3::TcpSocketFactory", apLocalAddress)
    #     sinkApp.Add(packetSinkHelper.Install (nodes.Get (x)))
    #
    # sinkApp.Start (ns.core.Seconds (0.0))
    # sinkApp.Stop (ns.core.Seconds (141.0))
    #
    # onoff = ns.applications.OnOffHelper ("ns3::TcpSocketFactory", ns.network.Ipv4Address.GetAny ())
    # onoff.SetAttribute ("OnTime",  ns.core.StringValue ("ns3::ConstantRandomVariable[Constant=1]"))
    # onoff.SetAttribute ("OffTime", ns.core.StringValue ("ns3::ConstantRandomVariable[Constant=0]"))
    # onoff.SetAttribute ("PacketSize", ns.core.UintegerValue (2048))
    # # onoff.SetAttribute ("DataRate", ns.network.DataRateValue (ns.network.DataRate (100000))) # bit/s
    #
    # apps = ns.network.ApplicationContainer ()
    # # remoteAddress = ns.network.AddressValue (ns.network.InetSocketAddress (interfaces.GetAddress (0), port))
    #
    # for x in range(7,14):
    #     rn = random.randint(0,3)
    #     print "Server >>: {0}".format(rn)
    #     remoteAddress = ns.network.AddressValue (ns.network.InetSocketAddress (interfaces.GetAddress (rn), port))
    #     onoff.SetAttribute ("Remote", remoteAddress)
    #     apps.Add (onoff.Install (nodes.Get (x)))
    #
    # apps.Start (ns.core.Seconds (1.0))
    # apps.Stop (ns.core.Seconds (141.0))

    ns.internet.Ipv4GlobalRoutingHelper.PopulateRoutingTables ()

    asciitracer = ns.network.AsciiTraceHelper ()
    # phy.EnableAsciiAll (asciitracer.CreateFileStream ("vanets.tr"))
    phy.EnablePcap("vanets",nodes)
    ns.core.Simulator.Schedule(ns.core.Seconds(10.0), SndPkt, source, packetsize, pktcount, pktinterval)

    ns.core.Simulator.Stop(ns.core.Seconds(161.0))
    ns.core.Simulator.Run()
    ns.core.Simulator.Destroy()

if __name__ == '__main__':
    main()
