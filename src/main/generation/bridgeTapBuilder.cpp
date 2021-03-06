#include <vector>
#include <memory>
#include <string>
#include <cstdlib>
#include <iostream>

#include "node.h"
#include "iface.h"
#include "bridgeTapBuilder.h"

using namespace std;

int assignBridgesTaps(ns3lxc::Topology *top, int curNum){
    for(auto nodePtr : top->nodes){
        for(auto it : nodePtr->ifaces){
            it.second->bridgeName = to_string(curNum) + "_b";
            it.second->tapName = to_string(curNum) + "_t";
            curNum++;
        }
    }
    for(auto topPtr : top->subTopologies){
        curNum = assignBridgesTaps(topPtr.get(), curNum);
    }
    return curNum;
}

void buildAllBridgesTaps(ns3lxc::Topology *top){
    for(auto topPtr : top->subTopologies){
        buildAllBridgesTaps(topPtr.get());
    }
    for(auto nodePtr : top->nodes){
        int i = 0;
        for(auto it : nodePtr->ifaces){
            
            if(it.second->ip != nullptr && it.second->subnetMask != nullptr){
                cout << nodePtr->name << ": " << it.first << endl;
                buildBridgeTap(it.second);
                i++;
            }
        }
    }
}

void buildBridgeTap(std::shared_ptr<ns3lxc::Iface> ifacePtr){
    int err;
    string tap = ifacePtr->tapName;
    string bridge = ifacePtr->bridgeName;
    cout << ifacePtr->name << " " << ifacePtr->ip << endl;
    //brctl addbr 'bridge'
    err = system(("brctl addbr " + bridge).c_str());
    if(err){
        cerr << "BAD" << endl;
    }
    //ip tuntap add 'tap' mode tap
    err = system(("ip tuntap add " + tap + " mode tap").c_str());
    if(err){
        cerr << "BAD2" << endl;
    }
    //ifconfig 'tap' 0.0.0.0 promisc up
    err = system(("ifconfig " + tap + " 0.0.0.0 promisc up").c_str());
    if(err){
        cerr << "BAD3" << endl;
    }
    //brctl addif 'bridge' 'tap'
    err = system(("brctl addif " + bridge + " " + tap).c_str());
    if(err){
        cerr << "BAD4" << endl;
    }

    //ifconfig 'bridge' 'ipAddr' netmask 'subnetAddr' up  ### adds two to IPADDR??? 
    string sub = ifacePtr->ip->str();
    sub = sub.substr(0, sub.find_last_of(".")) + '2';
    err = system(("ifconfig " + bridge + " " + sub + \
        " netmask " + ifacePtr->subnetMask->str() + " up").c_str());
    if(err){
        cerr << "BAD5" << endl;
    }
}

void tearDownAllBridgesTaps(ns3lxc::Topology *top){
    for(auto topPtr : top->subTopologies){
        tearDownAllBridgesTaps(topPtr.get());
    }
    for(auto nodePtr : top->nodes){
        for(auto it : nodePtr->ifaces){
            if(it.second->ip != nullptr && it.second->subnetMask != nullptr){
                tearDownBridgeTap(it.second);
            }
        }
    }
}

void tearDownBridgeTap(std::shared_ptr<ns3lxc::Iface> ifacePtr){
    int err;
    string tap = ifacePtr->tapName;
    string bridge = ifacePtr->bridgeName;
    string nullRedir = " &> /dev/null";
    //ifconfig 'bridge' down
    // err = system(("ifconfig " + bridge + " down" + nullRedir).c_str());
    pclose(popen(("ifconfig " + bridge + " down" + nullRedir).c_str(), "r"));
    if(err){

    }
    //brctl delif 'bridge' 'tap'
    // err = system(("brctl delif " + bridge + " " + tap + nullRedir).c_str());
    pclose(popen(("brctl delif " + bridge + " " + tap + nullRedir).c_str(), "r"));
    if(err){

    }
    //brctl delbr 'bridge'
    // err = system(("brctl delbr " + bridge + nullRedir).c_str());
    pclose(popen(("brctl delbr " + bridge + nullRedir).c_str(), "r"));
    if(err){

    }
    //ifconfig 'tap' down
    // err = system(("ifconfig " + tap + " down" + nullRedir).c_str());
    pclose(popen(("ifconfig " + tap + " down" + nullRedir).c_str(), "r"));
    if(err){

    }
    //ip tuntap del 'tap' mode tap
    // err = system(("ip tuntap del " + tap + " mode tap" + nullRedir).c_str());
    pclose(popen(("ip tuntap del " + tap + " mode tap" + nullRedir).c_str(), "r"));
    if(err){

    }
}