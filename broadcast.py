# /*
#  * This program is free software; you can redistribute it and/or modify
#  * it under the terms of the GNU General Public License version 2 as
#  * published by the Free Software Foundation;
#  *
#  * This program is distributed in the hope that it will be useful,
#  * but WITHOUT ANY WARRANTY; without even the implied warranty of
#  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  * GNU General Public License for more details.
#  *
#  * You should have received a copy of the GNU General Public License
#  * along with this program; if not, write to the Free Software
#  * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#  */

import ns.applications
import ns.core
import ns.internet
import ns.network
import ns.point_to_point
import ns.mobility

nodes = ns.network.NodeContainer()
nodes.Create(2)

pointToPoint = ns.point_to_point.PointToPointHelper()
pointToPoint.SetDeviceAttribute("DataRate", ns.core.StringValue("5Mbps"))
pointToPoint.SetChannelAttribute("Delay", ns.core.StringValue("2ms"))

devices = pointToPoint.Install(nodes)

stack = ns.internet.InternetStackHelper()
stack.Install(nodes)

address = ns.internet.Ipv4AddressHelper()
address.SetBase(ns.network.Ipv4Address("10.1.1.0"),
                ns.network.Ipv4Mask("255.255.255.0"))

interfaces = address.Assign(devices)

packetsize = 64 #bytes
pktcount = 4
pktinterval = 0.25 #seconds
port = 80

def RecPkt(socket):
    addr = ns.network.Address()
    msg = ""
    while True:
        msg = socket.RecvFrom(addr)
        if not msg: break
        print "[!] Received message!"
        print addr
    # while socket.Recv(1024,0):
    #     # print socket.GetLocalAddress()
    #     print "[+] Received one packet!"

def discover(socket):
        broadcast = ns.network.Ipv4Address("255.255.255.255")

        socket.Connect(ns.network.InetSocketAddress(broadcast, 80))
        socket.Send(ns.network.Packet(packetsize),0)
        print "[+] Initing discovering..."

def SndPkt(socket, packetsize, pktcount, pktinterval):
    if pktcount > 0:
        socket.Send(ns.network.Packet(packetsize),0)
        ns.core.Simulator.Schedule(ns.core.Seconds(pktinterval), SndPkt, socket, packetsize, pktcount-1, pktinterval)
        print "Sending one packet!"
    else:
        socket.Close()

appSource = ns.network.NodeList.GetNode(1)
appSink = ns.network.NodeList.GetNode(0)

remoteAddr = appSink.GetObject(ns.internet.Ipv4.GetTypeId()).GetAddress(1,0).GetLocal()
broadcast = ns.network.Ipv4Address("255.255.255.255")

sink = ns.network.Socket.CreateSocket(appSink, ns.core.TypeId.LookupByName("ns3::UdpSocketFactory"))
sink.Bind(ns.network.InetSocketAddress(ns.network.Ipv4Address.GetAny(), 80))
sink.Listen()
sink.SetRecvCallback(RecPkt)

source = ns.network.Socket.CreateSocket(appSource, ns.core.TypeId.LookupByName("ns3::UdpSocketFactory"))
source.SetAllowBroadcast(True)
# source.Connect(ns.network.InetSocketAddress(remoteAddr, port))
# source.Connect(ns.network.InetSocketAddress(broadcast, port))

ns.internet.Ipv4GlobalRoutingHelper.PopulateRoutingTables ()

pointToPoint.EnablePcap("testes",nodes)
ns.core.Simulator.Schedule(ns.core.Seconds(10.0),discover,source)
# ns.core.Simulator.Schedule(ns.core.Seconds(30.0), SndPkt, source, packetsize, pktcount, pktinterval)

print "Run Simulation."
ns.core.Simulator.Stop (ns.core.Seconds (61.0))
ns.core.Simulator.Run()
ns.core.Simulator.Destroy()
