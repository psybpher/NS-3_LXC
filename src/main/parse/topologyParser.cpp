#include <string>
#include <cctype>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <memory>

#include <arpa/inet.h>
#include <sys/stat.h>

#include "yaml-cpp/yaml.h"

#include "topology.h"
#include "node.h"
#include "link.h"
#include "iface.h"
#include "position.h"
#include "parserTags.h"
#include "nodeParser.h"
#include "linkParser.h"
#include "subTopologyParser.h"
#include "topologyParser.h"

using namespace std;

static void parseIncludes(YAML::Node includes, std::string topPath, ParsedTopology *parsedTop);
static void parseNodes(YAML::Node nodes, ParsedTopology *parsedTop);
static void parseLinks(YAML::Node links, ParsedTopology *parsedTop);
static void parseSubTopologies(YAML::Node topologies, ParsedTopology *parsedTop);
static void parseIfacesProvided(YAML::Node ifaces, ParsedTopology *parsedTop);
static void parseIfacesAccepted(YAML::Node ifacesAccepted, ParsedTopology *parsedTop);
static ns3lxc::Iface parseInterface(YAML::Node interface);

ns3lxc::Topology parseTopologyFile(std::string topPath){	
	ParsedTopology parsedTop;
	
	YAML::Node topology = YAML::LoadFile(topPath);

	if(topology[TAG_INCLUDE]){
		cout << "Parsing includes" << endl;
		parseIncludes(topology[TAG_INCLUDE], topPath, &parsedTop);
	} else if (topology[pluralize(TAG_INCLUDE)]){
		cout << "Parsing includes" << endl;
		parseIncludes(topology[pluralize(TAG_INCLUDE)], topPath, &parsedTop);
	}

	if(topology[TAG_TIME]){
		parsedTop.topology.runTime = topology[TAG_TIME].as<int>();
	}

	std::string topName = topPath.substr(topPath.find_last_of("\\/") + 1, topPath.find_last_of(".yaml") - topPath.find_last_of("\\/") - 5);

	if(topology[topName]){
		topology = topology[topName];
	} else if ('a' <= topName[0] <= 'z'){
		topName[0] = toupper(topName[0]);
		if(topology[topName]){
			topology = topology[topName];
		}
	} else if ('A' <= topName[0] <= 'Z'){
		topName[0] = tolower(topName[0]);
		if(topology[topName]){
			topology = topology[topName];
		}
	}
	parsedTop.topology.name = topName;
	parseTopology(topology, &parsedTop);
	return parsedTop.topology;
}

void parseTopology(YAML::Node topology, ParsedTopology *parsedTop){
	
	cout << "PARSING TOPOLOGY " << parsedTop->topology.name << endl;

	if(topology[TAG_NODE]){
		parseNodes(topology[TAG_NODE], parsedTop);
	} else if (topology[pluralize(TAG_NODE)]) {
		parseNodes(topology[pluralize(TAG_NODE)], parsedTop);
	}
	if(topology[TAG_TOPOLOGY]){
		parseSubTopologies(topology[TAG_TOPOLOGY], parsedTop);
	} else if (topology[pluralize(TAG_TOPOLOGY)]) {
		parseSubTopologies(topology[pluralize(TAG_TOPOLOGY)], parsedTop);
	}

	if(topology[TAG_LINK]){
		parseLinks(topology[TAG_LINK], parsedTop);
	} else if (topology[pluralize(TAG_LINK)]){
		parseLinks(topology[pluralize(TAG_LINK)], parsedTop);
	}

	if(topology[TAG_IFACES_PROVIDED]){
		parseIfacesProvided(topology[TAG_IFACES_PROVIDED], parsedTop);
	}

	if(topology[TAG_IFACES_ACCEPTED]){
		parseIfacesAccepted(topology[TAG_IFACES_ACCEPTED], parsedTop);
	}

	cout << "DONE PARSING TOP " << parsedTop->topology.name << endl;

}

static void parseIncludes(YAML::Node includes, std::string topPath, ParsedTopology *parsedTop){
	//Get dir of top file to search for included files
	shared_ptr<ns3lxc::Topology> includedTop;
	std::string curInclude;
	std::string searchPath;
	std::string topDir = topPath.substr(0, topPath.find_last_of("\\/"));

	for(auto i = 0; i < includes.size(); ++i){

		curInclude = includes[i].as<std::string>();
		searchPath = topDir + "/" + curInclude + ".yaml";

		struct stat buffer;
		if(stat(searchPath.c_str(), &buffer) == 0 && !S_ISDIR(buffer.st_mode)){
			includedTop = shared_ptr<ns3lxc::Topology>(new ns3lxc::Topology(parseTopologyFile(searchPath)));
		} else {
			searchPath = searchPath = topDir + "/include/" + curInclude + ".yaml";
			if(stat(searchPath.c_str(), &buffer) == 0 && !S_ISDIR(buffer.st_mode)){
				includedTop = shared_ptr<ns3lxc::Topology>(new ns3lxc::Topology(parseTopologyFile(searchPath)));
			} else {
				cerr << "Couldn't find included file " << curInclude << " while parsing " << topPath << endl;
				exit(10);
			}
		}

		parsedTop->nodes.insert(includedTop->nodeMap.begin(), includedTop->nodeMap.end());
		parsedTop->links.insert(includedTop->linkMap.begin(), includedTop->linkMap.end());
		parsedTop->includedTopologies[includedTop->name] = includedTop;
	}
}

static void parseSubTopologies(YAML::Node topologies, ParsedTopology *parsedTop){
	for(auto i = 0; i < topologies.size(); ++i){
		std::vector<std::shared_ptr<ns3lxc::Topology> > curTops = parseSubTopology(topologies[i], parsedTop);
		for(shared_ptr<ns3lxc::Topology> curTop : curTops){
			parsedTop->topology.subTopologies.push_back(curTop);
			parsedTop->topology.topMap[curTop->name] = curTop;
		}
	}
	renameSubTopologies(&parsedTop->topology);
}

static void parseNodes(YAML::Node nodes, ParsedTopology *parsedTop){
	std::vector<shared_ptr<ns3lxc::Node> > curNodes;
	for(auto i = 0; i < nodes.size(); ++i){
		curNodes = parseNode(nodes[i], parsedTop);

		for(auto j = 0; j < curNodes.size(); ++j){
			if(parsedTop->topology.nodeMap.count(curNodes[j]->name) > 0){
				cout << "NODE EXISTS" << endl;
			} else {
				cout << "Parsed node " << curNodes[j]->name << endl;
				parsedTop->topology.nodeMap.insert(std::map<std::string, std::shared_ptr<ns3lxc::Node> >::value_type(curNodes[j]->name, curNodes[j]));
				parsedTop->topology.nodes.push_back(curNodes[j]);
				curNodes[j]->nodeNum = parsedTop->topology.nodes.size() - 1;
			}

		}
	}
}

static void parseLinks(YAML::Node links, ParsedTopology *parsedTop){
	for(auto i = 0; i < links.size(); ++i){
		shared_ptr<ns3lxc::Link> curLink = parseLink(links[i], parsedTop);
		if(parsedTop->topology.linkMap.count(curLink->name) > 0){
			cout << "LINK EXISTS" << curLink->name << endl;
		} else {
			parsedTop->topology.links.push_back(curLink);
			parsedTop->topology.linkMap[curLink->name] = curLink;
		}
	}
}

static void parseIfacesProvided(YAML::Node ifaces, ParsedTopology *parsedTop){
	for(auto i = 0; i < ifaces.size(); ++i){
		vector<string> split = splitString(ifaces[i].begin()->second.as<string>());
		parsedTop->topology.ifacesProvidedSubNames[ifaces[i].begin()->first.as<string>()] = split[1];
		std::weak_ptr<ns3lxc::IfaceProvider> provPtr;
		if(parsedTop->topology.nodeMap.count(split[0]) > 0){
			provPtr = parsedTop->topology.nodeMap[split[0]];
		} else if(parsedTop->topology.topMap.count(split[0]) > 0){
			provPtr = parsedTop->topology.topMap[split[0]];
		} else {
			cerr << "COULDNT FIND" << endl;
		}
		parsedTop->topology.ifacesProvided[ifaces[i].begin()->first.as<string>()] = provPtr;
	}
}

static void parseIfacesAccepted(YAML::Node ifacesAccepted, ParsedTopology *parsedTop){
	for(auto i = 0; i < ifacesAccepted.size(); ++i){
		cout << "TURTLE" << ifacesAccepted[i] << endl;
	}
}

void renameSubTopologies(ns3lxc::Topology *topology, std::string prefix){
	if(prefix == ""){
		prefix = topology->name + "_";
	} else {
		prefix = prefix + topology->name + "_";
	}
	for(auto topPtr : topology->subTopologies){
		renameSubTopologies(topPtr.get(), prefix);
	}

	for(auto nodePtr : topology->nodes){
		nodePtr->name = prefix + nodePtr->name;
	}

	for(auto linkPtr : topology->links){
		linkPtr->name = prefix + linkPtr->name;
	}
}