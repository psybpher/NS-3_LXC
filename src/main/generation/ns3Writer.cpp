#include <string>
#include <ostream>
#include <fstream>
#include <iostream>

#include "settingsParser.h"
#include "topology.h"
#include "linkTypeMap.h"
#include "ns3Writer.h"

using namespace std;

void Ns3Writer::writeScript(ns3lxc::Topology *top, std::string scriptLoc){
    ofstream str;
    cout << "Saving ns-3 file @ " + Settings::script_dest + "/" + top->name + ".cc" << endl;
    str.open(scriptLoc);

    writeIncludes(str);
    writeTimingFunction(str);

    str << \
    R"(
int main( int argc, char *argv[]){
    CommandLine cmd;
    cmd.Parse (argc, argv);
    NodeContainer nodes;
    ns3::Packet::EnablePrinting();
    GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
    GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
    )" << endl;
    writeInit(str, top);
    writeLinksForTopology(str, top);
    writePositions(str, top);
    str << "for(uint64_t i = 0; i <= " + to_string(top->runTime) + "; i += 10){Simulator::Schedule(Seconds(i), &printTime);}" << endl;
    str << "Simulator::Run ();" << endl;
    str << "Simulator::Destroy ();" << endl;
    str << "}" << endl;

    str.close();
}

void Ns3Writer::writeIncludes(std::ostream& str){
    str << "#include <iostream>" << endl;
    str << "#include <chrono>" << endl;
    str << "#include <iomanip>" << endl;
    str << "#include <fstream>" << endl;
    str << "#include <fcntl.h>" << endl;
    str << "#include <unistd.h>" << endl;
    str << "#include <iosfwd>" << endl;
    str << "#include <sstream>" << endl;
    str << "#include <math.h>" << endl;

    str << "#include \"ns3/core-module.h\"" << endl;
    str << "#include \"ns3/network-module.h\"" << endl;
    str << "#include \"ns3/internet-module.h\"" << endl;
    str << "#include \"ns3/tap-bridge-module.h\"" << endl;
    str << "#include \"ns3/netanim-module.h\"" << endl;
    str << "#include \"ns3/mobility-module.h\"" << endl;
    str << "#include \"ns3/ipv4-global-routing-helper.h\"" << endl;
    str << "#include \"ns3/flow-monitor-module.h\"" << endl;
    str << "#include \"sys/ioctl.h\"" << endl;
    for(auto it : linkTypeMap){
        if(it.second->isUsed()){
            it.second->writeIncludes(str);
        }
    }
    str << endl;

    str << "typedef std::chrono::duration<int, std::ratio_multiply<std::chrono::hours::period, std::ratio<24>>::type> TimestampDays;" << endl;
    str << "using namespace ns3;" << endl;

    
}
void Ns3Writer::writeTimingFunction(std::ostream& str){
    str << \
R"(
void printTime(){
    std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    TimestampDays days = std::chrono::duration_cast<TimestampDays>(duration);
    duration -= days;
    auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
    duration -= hours;
    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
    duration -= minutes;
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    duration -= seconds;
    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration);
    std::cout << "\r" << "Simulation Time: " << Simulator::Now().GetSeconds() << "\tReal Time [" << hours.count() << ":" << minutes.count() << ":" << seconds.count() << "." << microseconds.count() << " UTC] ";
    std::cout.flush();
}
)" << endl;
}

void Ns3Writer::writeInit(std::ostream& str, ns3lxc::Topology *top){
    str << "Simulator::Stop (Seconds (" + to_string(top->runTime) + ".));" << endl;
    str << "nodes.Create(" + to_string(top->nodes.size()) + ");" << endl;
    //str << "AnimationInterface anim (\"animation.xml\");" << endl;
    for(auto it : linkTypeMap){
        if(it.second->isUsed()){
            cout << "ISUSED" << endl;
            it.second->writeTypeInit(str);
        }
        cout << "USED??" << endl;
    }
}

void Ns3Writer::writeLinksForTopology(std::ostream& str, ns3lxc::Topology *top){
    for(auto subTopPtr : top->subTopologies){
        writeLinksForTopology(str, subTopPtr.get());
    }
    for(auto linkPtr : top->links){
        linkTypeMap[linkPtr->getType()]->writeLinkInit(str, linkPtr);
        linkTypeMap[linkPtr->getType()]->addIfacesToLink(str, linkPtr);
    }
}

void Ns3Writer::writePositions(std::ostream& str, ns3lxc::Topology *top){
    str << R"(
MobilityHelper mobility;
MobilityHelper waymobility;
Ptr<WaypointMobilityModel> mob;
waymobility.SetMobilityModel("ns3::WaypointMobilityModel");
    )" << endl;
}