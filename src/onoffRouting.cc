#include "ns3/application-container.h"
#include "ns3/average.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/core-module.h"
#include "ns3/csma-helper.h"
#include "ns3/data-rate.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-flow-classifier.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/log.h"
#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/nstime.h"
#include "ns3/on-off-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/pointer.h"
#include "ns3/random-variable-stream.h"
#include "ns3/rip-helper.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/simulator.h"
#include "ns3/ipv4-global-routing-helper.h" 
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("OnOffRoutingExperiment");

int
main(int argc, char* argv[])
{
    // --- PARÁMETROS DE SIMULACIÓN ---
    bool enableLogs = false; // <--  Flag para controlar los logs
    uint32_t num_usuarios = 100;
    std::string bitrate_str = "8Mbps";
    uint32_t semilla = 1;

    CommandLine cmd;
    cmd.AddValue("enableLogs", "Habilitar logs detallados", enableLogs); // <-- argumento para activar logs
    cmd.AddValue("num_usuarios", "Numero de usuarios a simular", num_usuarios);
    cmd.AddValue("bitrate", "Bitrate del enlace L1", bitrate_str);
    cmd.AddValue("semilla", "Semilla para el generador aleatorio", semilla);
    cmd.Parse(argc, argv);

   
    if (enableLogs)
    {
        LogComponentEnable("OnOffRoutingExperiment", LOG_LEVEL_ALL);
        NS_LOG_FUNCTION(argc << argv);
        NS_LOG_INFO("Iniciando simulación con los siguientes parámetros:");
        NS_LOG_INFO("  - Número de Usuarios Totales: " << num_usuarios);
        NS_LOG_INFO("  - Bitrate del enlace L1: " << bitrate_str);
        NS_LOG_INFO("  - Semilla: " << semilla);
    }


    RngSeedManager::SetSeed(semilla);
    Time::SetResolution(Time::US);

    double bitrate_val = std::stod(bitrate_str.substr(0, bitrate_str.find("Mbps")));

    // --- NODOS Y PILA DE RED ---
    uint32_t numValFHD = num_usuarios * 0.7 * 0.7;
    uint32_t numValHD = num_usuarios * 0.2 * 0.7;
    uint32_t numValSD = num_usuarios * 0.1 * 0.7;
    uint32_t numBalFHD = num_usuarios * 0.7 * 0.3;
    uint32_t numBalHD = num_usuarios * 0.2 * 0.3;
    uint32_t numBalSD = num_usuarios * 0.1 * 0.3;
    uint32_t num_usuarios_valencia = numValFHD + numValHD + numValSD;
    uint32_t num_usuarios_baleares = numBalFHD + numBalHD + numBalSD;

    NodeContainer allNodes, serverNode, routerNodes, valenciaUsers, balearesUsers;
    serverNode.Create(1);
    routerNodes.Create(3);
    if (num_usuarios_valencia > 0) valenciaUsers.Create(num_usuarios_valencia);
    if (num_usuarios_baleares > 0) balearesUsers.Create(num_usuarios_baleares);

    allNodes.Add(serverNode);
    allNodes.Add(routerNodes);
    allNodes.Add(valenciaUsers);
    allNodes.Add(balearesUsers);
    
    if (enableLogs) {
        NS_LOG_INFO("Nodos creados: 1 Servidor, 3 Routers, " << num_usuarios_valencia << " usuarios en Valencia, " << num_usuarios_baleares << " en Baleares.");
    }

    InternetStackHelper stack;
    Ipv4ListRoutingHelper listRouting;
    listRouting.Add(Ipv4StaticRoutingHelper(), 0);
    listRouting.Add(RipHelper(), 10);
    stack.SetRoutingHelper(listRouting);
    stack.Install(allNodes);

    // --- TOPOLOGÍA Y DIRECCIONES IP ---
    CsmaHelper backboneCsma, lanCsma;
    
    backboneCsma.SetChannelAttribute("DataRate", DataRateValue(DataRate("1Gbps")));
    lanCsma.SetChannelAttribute("DataRate", DataRateValue(DataRate("100Mbps")));
    Ipv4AddressHelper ipHelper;

    Ipv4InterfaceContainer serverRouter1Interfaces;
    ipHelper.SetBase("10.1.1.0", "255.255.255.0");
    serverRouter1Interfaces = ipHelper.Assign(backboneCsma.Install(NodeContainer(serverNode.Get(0), routerNodes.Get(0))));

    Ipv4InterfaceContainer router1Router2Interfaces;
    backboneCsma.SetChannelAttribute("DataRate", DataRateValue(DataRate(bitrate_str)));
    ipHelper.SetBase("20.1.1.0", "255.255.255.0");
    router1Router2Interfaces = ipHelper.Assign(backboneCsma.Install(NodeContainer(routerNodes.Get(0), routerNodes.Get(1))));
    
    Ipv4InterfaceContainer router1Router3Interfaces;
    backboneCsma.SetChannelAttribute("DataRate", DataRateValue(DataRate("1Gbps")));
    ipHelper.SetBase("30.1.1.0", "255.255.255.0");
    router1Router3Interfaces = ipHelper.Assign(backboneCsma.Install(NodeContainer(routerNodes.Get(0), routerNodes.Get(2))));

    Ipv4InterfaceContainer valenciaInterfaces, balearesInterfaces;
    if (num_usuarios_valencia > 0) {
        ipHelper.SetBase("40.1.0.0", "255.255.252.0"); 
        valenciaInterfaces = ipHelper.Assign(lanCsma.Install(NodeContainer(routerNodes.Get(1), valenciaUsers)));
    }
    if (num_usuarios_baleares > 0) {
        ipHelper.SetBase("50.1.0.0", "255.255.252.0");
        balearesInterfaces = ipHelper.Assign(lanCsma.Install(NodeContainer(routerNodes.Get(2), balearesUsers)));
    }

    if (enableLogs)
    {
        NS_LOG_DEBUG("--- Direcciones IP Asignadas ---");
        NS_LOG_DEBUG("Servidor: " << serverRouter1Interfaces.GetAddress(0));
        NS_LOG_DEBUG("Router 1 (eth0): " << serverRouter1Interfaces.GetAddress(1));
        NS_LOG_DEBUG("Router 1 (eth1): " << router1Router2Interfaces.GetAddress(0));
        NS_LOG_DEBUG("Router 2 (eth0): " << router1Router2Interfaces.GetAddress(1));
        NS_LOG_DEBUG("Router 1 (eth2): " << router1Router3Interfaces.GetAddress(0));
        NS_LOG_DEBUG("Router 3 (eth0): " << router1Router3Interfaces.GetAddress(1));
        if (num_usuarios_valencia > 0) NS_LOG_DEBUG("Router 2 (LAN Valencia): " << valenciaInterfaces.GetAddress(0));
        if (num_usuarios_baleares > 0) NS_LOG_DEBUG("Router 3 (LAN Baleares): " << balearesInterfaces.GetAddress(0));
        NS_LOG_DEBUG("---------------------------------");

        NS_LOG_INFO("Se ha programado la impresión de las tablas de enrutamiento en t=11s.");
        RipHelper routingHelper;
        Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper>(&std::cout);
        routingHelper.PrintRoutingTableAllAt(Seconds(11.0), routingStream);
    }

    // --- APLICACIONES ---
    Time startTime = Seconds(10.0);
    ApplicationContainer sourceApps;
    Ptr<WeibullRandomVariable> onTimeKplus = CreateObject<WeibullRandomVariable>();
    onTimeKplus->SetAttribute("Scale", DoubleValue(300));
    onTimeKplus->SetAttribute("Shape", DoubleValue(1.1));
    Ptr<WeibullRandomVariable> onTimeKminus = CreateObject<WeibullRandomVariable>();
    onTimeKminus->SetAttribute("Scale", DoubleValue(300));
    onTimeKminus->SetAttribute("Shape", DoubleValue(0.9));
    std::vector<Ptr<RandomVariableStream>> onTimeList = {onTimeKplus, onTimeKminus};
    Ptr<ExponentialRandomVariable> offTime = CreateObject<ExponentialRandomVariable>();
    offTime->SetAttribute("Mean", DoubleValue(6000));
    
    if (num_usuarios_valencia > 0) {
        PacketSinkHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 9)).Install(valenciaUsers).Start(startTime);
        uint32_t user_idx = 0;
        if (enableLogs) {
            NS_LOG_LOGIC("Creando aplicaciones para Valencia: " << (numValFHD * 0.4) << " FHD, " << (numValHD * 0.4) << " HD, " << (numValSD * 0.4) << " SD.");
        }
        for (uint32_t i = 0; i < (numValFHD * 0.4); ++i, ++user_idx) { OnOffHelper h("ns3::TcpSocketFactory", InetSocketAddress(valenciaInterfaces.GetAddress(user_idx + 1), 9)); h.SetAttribute("OnTime", PointerValue(onTimeList[i % 2])); h.SetAttribute("OffTime", PointerValue(offTime)); h.SetConstantRate(DataRate("800kb/s")); sourceApps.Add(h.Install(serverNode.Get(0))); }
        for (uint32_t i = 0; i < (numValHD * 0.4); ++i, ++user_idx) { OnOffHelper h("ns3::TcpSocketFactory", InetSocketAddress(valenciaInterfaces.GetAddress(user_idx + 1), 9)); h.SetAttribute("OnTime", PointerValue(onTimeList[i % 2])); h.SetAttribute("OffTime", PointerValue(offTime)); h.SetConstantRate(DataRate("500kb/s")); sourceApps.Add(h.Install(serverNode.Get(0))); }
        for (uint32_t i = 0; i < (numValSD * 0.4); ++i, ++user_idx) { OnOffHelper h("ns3::TcpSocketFactory", InetSocketAddress(valenciaInterfaces.GetAddress(user_idx + 1), 9)); h.SetAttribute("OnTime", PointerValue(onTimeList[i % 2])); h.SetAttribute("OffTime", PointerValue(offTime)); h.SetConstantRate(DataRate("200kb/s")); sourceApps.Add(h.Install(serverNode.Get(0))); }
    }
    
    if (num_usuarios_baleares > 0) {
        PacketSinkHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 10)).Install(balearesUsers).Start(startTime);
        uint32_t user_idx_bal = 0;
        if (enableLogs) {
            NS_LOG_LOGIC("Creando aplicaciones para Baleares: " << (numBalFHD * 0.4) << " FHD, " << (numBalHD * 0.4) << " HD, " << (numBalSD * 0.4) << " SD.");
        }
        for (uint32_t i = 0; i < (numBalFHD * 0.4); ++i, ++user_idx_bal) { OnOffHelper h("ns3::TcpSocketFactory", InetSocketAddress(balearesInterfaces.GetAddress(user_idx_bal + 1), 10)); h.SetAttribute("OnTime", PointerValue(onTimeList[i % 2])); h.SetAttribute("OffTime", PointerValue(offTime)); h.SetConstantRate(DataRate("800kb/s")); sourceApps.Add(h.Install(serverNode.Get(0))); }
        for (uint32_t i = 0; i < (numBalHD * 0.4); ++i, ++user_idx_bal) { OnOffHelper h("ns3::TcpSocketFactory", InetSocketAddress(balearesInterfaces.GetAddress(user_idx_bal + 1), 10)); h.SetAttribute("OnTime", PointerValue(onTimeList[i % 2])); h.SetAttribute("OffTime", PointerValue(offTime)); h.SetConstantRate(DataRate("500kb/s")); sourceApps.Add(h.Install(serverNode.Get(0))); }
        for (uint32_t i = 0; i < (numBalSD * 0.4); ++i, ++user_idx_bal) { OnOffHelper h("ns3::TcpSocketFactory", InetSocketAddress(balearesInterfaces.GetAddress(user_idx_bal + 1), 10)); h.SetAttribute("OnTime", PointerValue(onTimeList[i % 2])); h.SetAttribute("OffTime", PointerValue(offTime)); h.SetConstantRate(DataRate("200kb/s")); sourceApps.Add(h.Install(serverNode.Get(0))); }
    }

    sourceApps.Start(startTime);
    sourceApps.Stop(Seconds(40.0));

    // --- SIMULACIÓN Y RECOLECCIÓN DE DATOS ---
    FlowMonitorHelper flowmonHelper;
    Ptr<FlowMonitor> flowmon = flowmonHelper.InstallAll();

    Simulator::Stop(Seconds(45.0));
    Simulator::Run();

    Average<double> avgDelay, avgJitter;
    double totalTxPackets = 0, totalLostPackets = 0;
    for (auto const& [flowId, flowStats] : flowmon->GetFlowStats()) {
        if (flowStats.txPackets > 10) {
            totalTxPackets += flowStats.txPackets;
            totalLostPackets += flowStats.lostPackets;
            if (flowStats.rxPackets > 0) avgDelay.Update((flowStats.delaySum.GetSeconds() / flowStats.rxPackets) * 1000);
            if (flowStats.rxPackets > 1) avgJitter.Update(flowStats.jitterSum.GetSeconds() / (flowStats.rxPackets - 1) * 1000);
        }
    }
    double lossRatio = (totalTxPackets > 0) ? (totalLostPackets / totalTxPackets * 100) : 0;
    
    std::ofstream outFile("results.dat", std::ios::app);
    outFile << num_usuarios << " " << bitrate_val << " " << lossRatio << " " << avgDelay.Mean() << " " << avgJitter.Mean() << std::endl;
    outFile.close();

    Simulator::Destroy();
    
    // Este log de resumen final se imprime siempre para poder seguir el progreso.
    NS_LOG_INFO("Fin de la réplica. Resumen -> Usuarios: " << num_usuarios << ", Bitrate: " << bitrate_str << ", Delay: " << avgDelay.Mean() << " ms, Jitter: " << avgJitter.Mean() << " ms, Pérdidas: " << lossRatio << " %");
    
    return 0;
}
