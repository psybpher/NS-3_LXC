#include <string>
#include <iostream>
#include <cstring>
#include <unistd.h>

#include "yaml-cpp/yaml.h"
#include "settingsParser.h"
#include "topologyParser.h"
#include "topologyGenerator.h"
#include "topology.h"
#include "node.h"
#include "link.h"

#define MAXPATHLEN 1024

#define PROJ_ROOT_DIR "NS-3_LXC"
#define SETTINGS_FILE "settings.yaml"
#define NO_FILE_PROVIDED "Exiting, no file provided"

using namespace std;

static std::string compute_settings_path(){
	char cwd[MAXPATHLEN];
	size_t substr;
	std::string dir;

	getcwd(cwd, MAXPATHLEN);
	dir = std::string(cwd);
	
	substr = dir.find(PROJ_ROOT_DIR);

	if(substr != std::string::npos){
		dir = dir.substr(0, substr + strlen(PROJ_ROOT_DIR));
		dir = dir + '/' + SETTINGS_FILE;
	} else {
		dir = SETTINGS_FILE;
	}

	return dir;
}

int main(int argc, char *argv[]){

	if(geteuid()){
		cerr << "Must run with root privileges" << endl;
		exit(-1);
	}
	
	std::string settings_path = compute_settings_path();

	// open settings file and obtain directory locations
	int result = Settings::parse_settings_file(settings_path);
	if(result != 0){
		return result;
	}
	
	ns3lxc::Topology topology;

	if(argc < 2) {
		cerr << NO_FILE_PROVIDED << endl;
		return 1;
	}

	topology = parseTopologyFile(argv[1]);
	if(argc > 2){
		Settings::run_mode = Mode::NS3_GEN;
		cout << "RUN mode ns3" << endl;
	}

	generateTopology(&topology);

	return 0;
	
}
