#include <fstream>
#include <iostream>
#include <sstream>
#include <map>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/aodv-module.h"
#include "ns3/olsr-module.h"
#include "ns3/dsdv-module.h"
#include "ns3/dsr-module.h"
#include "ns3/applications-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns2-node-utility.h"

using namespace ns3;
using namespace dsr;

NS_LOG_COMPONENT_DEFINE ("manet-routing-compare");

class RoutingExperiment
{
public:
  RoutingExperiment ();
  void Run (int nSinks, double txp, std::string CSVfileName, std::string tcl_file);
  std::string CommandSetup (int argc, char **argv);
  std::string GetTclFile() const { return m_tcl_file; }

private:
  Ptr<Socket> SetupPacketReceive (Ipv4Address addr, Ptr<Node> node);
  void ReceivePacket (Ptr<Socket> socket);
  void CheckThroughput ();
  void TransmitPacket (Ptr<const Packet> packet);
  void CalculateAverageDelay ();
  void CalculateAverageHops ();
  std::string PrintReceivedPacket (Ptr<Socket> socket, Ptr<Packet> packet, Address senderAddress);
  std::set<uint64_t> sentPacketIds;
  uint32_t totalHops;
  std::map<uint64_t, uint32_t> packetFirstHop;

  uint32_t port;
  uint32_t bytesTotal;
  uint32_t packetsReceived;
  uint32_t packetsSent;
  
  // Add variables for delay calculation
  double totalDelay;
  std::map<uint64_t, Time> packetTimes;

  std::string m_CSVfileName;
  int m_nSinks;
  std::string m_protocolName;
  double m_txp;
  bool m_traceMobility;
  uint32_t m_protocol;
  std::string m_tcl_file;
};

RoutingExperiment::RoutingExperiment ()
  : port (9),
    bytesTotal (0),
    packetsReceived (0),
    packetsSent (0),
    totalDelay (0.0),
    totalHops (0),
    m_CSVfileName ("manet-routing.output.csv"),
    m_traceMobility (false),
    m_protocol (4),
    m_tcl_file ("ns2_mobility.tcl")
{
  sentPacketIds.clear();
  packetFirstHop.clear();
}

std::string
RoutingExperiment::PrintReceivedPacket (Ptr<Socket> socket, Ptr<Packet> packet, Address senderAddress)
{
  std::ostringstream oss;

  oss << Simulator::Now ().GetSeconds () << " " << socket->GetNode ()->GetId ();

  if (InetSocketAddress::IsMatchingType (senderAddress))
    {
      InetSocketAddress addr = InetSocketAddress::ConvertFrom (senderAddress);
      oss << " received one packet from " << addr.GetIpv4 ();
    }
  else
    {
      oss << " received one packet!";
    }
  return oss.str ();
}

void
RoutingExperiment::TransmitPacket (Ptr<const Packet> packet)
{
  uint64_t uid = packet->GetUid();
  if (sentPacketIds.find(uid) == sentPacketIds.end())
  {
    sentPacketIds.insert(uid);
    packetTimes[uid] = Simulator::Now();
    packetFirstHop[uid] = 0;  // Initialize hop count tracking
    packetsSent += 1;
    std::cout << "New packet sent " << packetsSent << " at time " << Simulator::Now().GetSeconds() << "s" << std::endl;
  }
}

void
RoutingExperiment::ReceivePacket (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address senderAddress;
  while ((packet = socket->RecvFrom (senderAddress)))
    {
      uint64_t uid = packet->GetUid();
      auto sendTimeIter = packetTimes.find(uid);
      if (sendTimeIter != packetTimes.end())
      {
        // Calculate delay
        Time delay = Simulator::Now() - sendTimeIter->second;
        totalDelay += delay.GetSeconds();
        
        // Calculate hops - difference between final receiving node and first hop
        uint32_t finalNode = socket->GetNode()->GetId();
        uint32_t sourceNode = packetFirstHop[uid];
        uint32_t hopCount = (finalNode >= sourceNode) ? 
                           (finalNode - sourceNode + 1) : 
                           (sourceNode - finalNode + 1);
        totalHops += hopCount;
        
        // Clean up
        packetTimes.erase(sendTimeIter);
        packetFirstHop.erase(uid);
      }
      
      bytesTotal += packet->GetSize ();
      packetsReceived += 1;
      NS_LOG_UNCOND (PrintReceivedPacket (socket, packet, senderAddress));
    }
}

void
RoutingExperiment::CalculateAverageHops ()
{
  double avgHops = 0;
  if (packetsReceived > 0)
  {
    avgHops = static_cast<double>(totalHops) / packetsReceived;
  }
  
  std::cout << "Average Number of Hops: " << avgHops << std::endl;
  
  // Save hop statistics to a separate file
  std::ofstream hopsFile ("/path/to/your/density_hops.txt", std::ios::app);
  hopsFile << avgHops << std::endl;
  hopsFile.close ();
}

void
RoutingExperiment::CheckThroughput ()
{
  double kbs = (bytesTotal * 8.0) / 1000;
  bytesTotal = 0;

  std::ofstream out (m_CSVfileName.c_str (), std::ios::app);

  // Calculate average delay
  double avgDelay = 0;
  if (packetsReceived > 0)
  {
    avgDelay = totalDelay / packetsReceived;
  }

  out << (Simulator::Now ()).GetSeconds () << ","
      << kbs << ","
      << packetsReceived << ","
      << m_nSinks << ","
      << m_protocolName << ","
      << m_txp << ","
      << avgDelay << ""  // Add average delay to CSV
      << std::endl;

  out.close ();
  Simulator::Schedule (Seconds (1.0), &RoutingExperiment::CheckThroughput, this);
}

void
RoutingExperiment::CalculateAverageDelay ()
{
  double avgDelay = 0;
  if (packetsReceived > 0)
  {
    avgDelay = totalDelay / packetsReceived;
  }
  
  std::cout << "Average End-to-End Delay: " << avgDelay * 1000 << " ms" << std::endl;
  
  // Save delay statistics to a separate file
  std::ofstream delayFile ("/path/to/your/density_delay.txt", std::ios::app);
  delayFile << avgDelay * 1000 << std::endl;  // Convert to milliseconds
  delayFile.close ();
}

Ptr<Socket>
RoutingExperiment::SetupPacketReceive (Ipv4Address addr, Ptr<Node> node)
{
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> sink = Socket::CreateSocket (node, tid);
  InetSocketAddress local = InetSocketAddress (addr, port);
  sink->Bind (local);
  sink->SetRecvCallback (MakeCallback (&RoutingExperiment::ReceivePacket, this));

  return sink;
}

std::string
RoutingExperiment::CommandSetup (int argc, char **argv)
{
  CommandLine cmd (__FILE__);
  cmd.AddValue ("CSVfileName", "The name of the CSV output file name", m_CSVfileName);
  cmd.AddValue ("traceMobility", "Enable mobility tracing", m_traceMobility);
  cmd.AddValue ("protocol", "1=OLSR;2=AODV;3=DSDV;4=DSR", m_protocol);
  cmd.AddValue ("tcl_file", "Path to the NS-2 movement trace file", m_tcl_file);
  cmd.Parse (argc, argv);
  return m_CSVfileName;
}

void
RoutingExperiment::Run (int nSinks, double txp, std::string CSVfileName, std::string tcl_file)
{
  Packet::EnablePrinting ();
  m_nSinks = 9;  // Fixed to 9 sink nodes
  m_txp = txp;
  m_CSVfileName = CSVfileName;

  // Create Ns2NodeUtility instance first
  Ns2NodeUtility ns2_utility(tcl_file);
  uint32_t nVehicles = ns2_utility.GetNNodes();
  double TotalTime = ns2_utility.GetSimulationTime();

  // First create and setup sink nodes
  NodeContainer sinkNodes;
  sinkNodes.Create(m_nSinks);

  // Setup fixed positions for sink nodes
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
  
  double x_min = 200.0;
  double x_max = 3100.0;
  double y_min = 250.0;
  double y_max = 2300.0;
  
  double x_step = (x_max - x_min) / 2.0;
  double y_step = (y_max - y_min) / 2.0;

  // Add grid positions for sinks
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      double x = x_min + j * x_step;
      double y = y_min + i * y_step;
      positionAlloc->Add(Vector(x, y, 0.0));
    }
  }

  // Install static mobility on sink nodes
  MobilityHelper mobilityStatic;
  mobilityStatic.SetPositionAllocator(positionAlloc);
  mobilityStatic.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobilityStatic.Install(sinkNodes);

  // Now handle vehicle nodes from TCL file
  NodeContainer vehicleNodes;
  vehicleNodes.Create(nVehicles);
  
  // Configure mobility from TCL file for vehicle nodes using the template version
  Ns2MobilityHelper mobility(tcl_file);
  mobility.Install(vehicleNodes.Begin(), vehicleNodes.End());  // Install on all vehicle nodes at once

  // Combine all nodes
  NodeContainer adhocNodes;
  adhocNodes.Add(sinkNodes);
  adhocNodes.Add(vehicleNodes);

  std::string rate ("2048bps");
  std::string phyMode ("DsssRate11Mbps");
  std::string tr_name ("manet-routing-compare");

  Config::SetDefault ("ns3::OnOffApplication::PacketSize", StringValue ("64"));
  Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue (rate));
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue (phyMode));

  // Setting up wifi phy and channel using helpers
  WifiHelper wifi;
  wifi.SetStandard (WIFI_STANDARD_80211b);

  YansWifiPhyHelper wifiPhy;
  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");
  wifiPhy.SetChannel (wifiChannel.Create ());

  // Add a mac and disable rate control
  WifiMacHelper wifiMac;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                               "DataMode", StringValue (phyMode),
                               "ControlMode", StringValue (phyMode));

  wifiPhy.Set ("TxPowerStart", DoubleValue (txp));
  wifiPhy.Set ("TxPowerEnd", DoubleValue (txp));

  wifiMac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer adhocDevices = wifi.Install (wifiPhy, wifiMac, adhocNodes);

  // Set up routing
  AodvHelper aodv;
  OlsrHelper olsr;
  DsdvHelper dsdv;
  DsrHelper dsr;
  DsrMainHelper dsrMain;
  Ipv4ListRoutingHelper list;
  InternetStackHelper internet;

  switch (m_protocol)
    {
    case 1:
      list.Add (olsr, 100);
      m_protocolName = "OLSR";
      break;
    case 2:
      list.Add (aodv, 100);
      m_protocolName = "AODV";
      break;
    case 3:
      list.Add (dsdv, 100);
      m_protocolName = "DSDV";
      break;
    case 4:
      m_protocolName = "DSR";
      break;
    default:
      NS_FATAL_ERROR ("No such protocol:" << m_protocol);
    }

  if (m_protocol < 4)
    {
      internet.SetRoutingHelper (list);
      internet.Install (adhocNodes);
    }
  else if (m_protocol == 4)
    {
      internet.Install (adhocNodes);
      dsrMain.Install (dsr, adhocNodes);
    }

  // Setup IP addresses
  Ipv4AddressHelper addressAdhoc;
  addressAdhoc.SetBase ("10.1.0.0", "255.255.0.0");
  Ipv4InterfaceContainer adhocInterfaces;
  adhocInterfaces = addressAdhoc.Assign (adhocDevices);

  // Setup applications
  OnOffHelper onoff1 ("ns3::UdpSocketFactory", Address ());
  onoff1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
  onoff1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"));

  // Install applications on vehicle nodes to send to sink nodes
  for (uint32_t i = m_nSinks; i < adhocNodes.GetN(); i++)
  {
    // Choose a random sink node as the destination
    int sinkIndex = rand() % m_nSinks;
    Ptr<Socket> sink = SetupPacketReceive (adhocInterfaces.GetAddress (sinkIndex), adhocNodes.Get (sinkIndex));

    AddressValue remoteAddress (InetSocketAddress (adhocInterfaces.GetAddress (sinkIndex), port));
    onoff1.SetAttribute ("Remote", remoteAddress);

    // Use entry/exit times for vehicle nodes
    double startTime = ns2_utility.GetEntryTimeForNode(i - m_nSinks);
    double stopTime = ns2_utility.GetExitTimeForNode(i - m_nSinks);

    ApplicationContainer temp = onoff1.Install (adhocNodes.Get (i));
    temp.Start (Seconds (startTime));
    temp.Stop (Seconds (stopTime));

    Ptr<Application> app = temp.Get (0);
    Ptr<OnOffApplication> onoffApp = DynamicCast<OnOffApplication> (app);
    if (onoffApp)
    {
      onoffApp->TraceConnectWithoutContext ("Tx", MakeCallback (&RoutingExperiment::TransmitPacket, this));
    }
  }

  // Enable traces
  AsciiTraceHelper ascii;
  MobilityHelper::EnableAsciiAll (ascii.CreateFileStream (tr_name + ".mob"));

  // Run simulation
  CheckThroughput ();
  
  Simulator::Stop (Seconds (TotalTime));
  Simulator::Run ();

  // Output results
  std::cout << "Sent: " << packetsSent << std::endl;
  std::cout << "Received: " << packetsReceived << std::endl;
  double deliveryRate = 0.0;
    if (packetsSent > 0)
    {
        deliveryRate = (double(packetsReceived) / packetsSent) * 100.0;
    }

    double avgDelay = 0.0;
    if (packetsReceived > 0)
    {
        avgDelay = totalDelay / packetsReceived;
    }

    double avgHops = 0.0;
    if (packetsReceived > 0)
    {
        avgHops = static_cast<double>(totalHops) / packetsReceived;
    }

    // Print to console
    std::cout << "Sent: " << packetsSent << std::endl;
    std::cout << "Received: " << packetsReceived << std::endl;
    std::cout << "Delivery Rate: " << deliveryRate << "%" << std::endl;
    std::cout << "Average End-to-End Delay: " << avgDelay * 1000 << " ms" << std::endl;
    std::cout << "Average Number of Hops: " << avgHops << std::endl;

    // Write to single CSV file
    std::ofstream experimentFile("/path/to/your/experiment_v2i.csv", std::ios::app);
    experimentFile << m_protocolName << ","
                  << deliveryRate << ","
                  << avgDelay * 1000 << "," // Convert to ms
                  << avgHops << ","
                  << packetsSent << ","
                  << packetsReceived << ","
                  << m_txp << std::endl;
    experimentFile.close();

  Simulator::Destroy ();
}

int
main (int argc, char *argv[])
{
    RoutingExperiment experiment;
    std::string CSVfileName = experiment.CommandSetup (argc, argv);

    // Check if file exists
    std::ifstream checkFile("/path/to/your/experiment_v2i.csv");
    if (!checkFile.good()) {
        // File doesn't exist, create it with headers
        std::ofstream experimentFile("/path/to/your/experiment_v2i.csv");
        experimentFile << "Protocol,"
                      << "DeliveryRate(%),"
                      << "AverageDelay(ms),"
                      << "AverageHops,"
                      << "PacketsSent,"
                      << "PacketsReceived,"
                      << "TransmissionPower"
                      << std::endl;
        experimentFile.close();
    }
    checkFile.close();

    int nSinks = 10;
    double txp = 1600.0;

    experiment.Run (nSinks, txp, CSVfileName, experiment.GetTclFile());
    return 0;
}