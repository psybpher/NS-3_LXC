#include <iostream>
#include <fstream>
#include <cstdio>
#include <string>
#include <sys/stat.h>

#include "yaml-cpp/yaml.h"
#include "settingsParser.h"

#define NS3_PATH_STR "NS-3_PATH"
#define SCRIPT_DEST_STR "SCRIPT_DEST"
#define TEMP_DEST_STR "TEMP_DIR"
#define OUTPUT_DEST_STR "OUTPUT_DEST"
#define CONTAINER_TYPE "CONTAINER_TYPE"

#define SCRIPT_DEFAULT_DEST "../../output/scripts"
#define OUTPUT_DEFAULT_DEST "../../output"
#define DEFAULT_CONTAINER "ubuntu xenial"

#define CONTAINER_CONFIG "/container_config"

#define MKDIR_MODE S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH

using namespace std;

void create_template_settings_file(string settings_file){
	std::ofstream file_stream(settings_file.c_str(), std::ios::binary);
	std::ifstream template_file("../../resources/settings_template.conf", std::ios::binary);
	
	file_stream << template_file.rdbuf();
	
	template_file.close();
	file_stream.close();
}

Mode Settings::run_mode = Mode::NORMAL;
string Settings::ns3_path;
string Settings::script_dest;
string Settings::temp_dir;
string Settings::output_dest;
string Settings::container_config_dir;
string Settings::container_type;

static bool check_make_dir(const char *path){
	struct stat buffer;
	int stat_result = stat(path, &buffer);
	if(stat_result != 0 || !S_ISDIR(buffer.st_mode)){
		if(mkdir(path, MKDIR_MODE) != 0){
			return false;
		}
	}
	return true;
}

int Settings::parse_settings_file(std::string settings_file){
	// open settings file and obtain directory locations
	std::ifstream file_stream(settings_file.c_str(), std::ifstream::in);
	string line;
	bool found_path = false, found_script = false, found_output = false;
	int pos, comment_pos, stat_result;

	YAML::Node settings = YAML::LoadFile(settings_file);

	if(!settings){
		create_template_settings_file(settings_file);
		cerr << "No settings.conf file found, an empty template has been created in " << endl;
		return 1;
	}

	if(settings[NS3_PATH_STR]){
		Settings::ns3_path = settings[NS3_PATH_STR].as<std::string>();
	} else {
		cerr << "The NS-3 path variable is not set" << endl;
		return 2;
	}

	// check for required destinations and error if not found
	struct stat buffer;
	if(stat(Settings::ns3_path.c_str(), &buffer) != 0 || !S_ISDIR(buffer.st_mode)){
		cerr << "The NS-3 path specified does not exist or is not accesible" << endl;
		return 3;
	}

	if(settings[SCRIPT_DEST_STR]){
		Settings::script_dest = settings[SCRIPT_DEST_STR].as<std::string>();
	} else {
		Settings::script_dest = SCRIPT_DEFAULT_DEST;
	}

	if(!check_make_dir(Settings::script_dest.c_str())){
		cerr << "Cannot make default script destination, aborting" << endl;
		return 4;
	}

	if(settings[OUTPUT_DEST_STR]){
		Settings::output_dest = settings[OUTPUT_DEST_STR].as<std::string>();
	} else {
		Settings::output_dest = OUTPUT_DEFAULT_DEST;
	}

	if(!check_make_dir(Settings::output_dest.c_str())){
		cerr << "Cannot make default output destination, aborting" << endl;
		return 5;
	}

	if(settings[TEMP_DEST_STR]){
		Settings::temp_dir = settings[TEMP_DEST_STR].as<string>();
	} else {
		cerr << "No temp directory specified in settings.yaml" << endl;
		return 6;
	}

	if(!check_make_dir(Settings::temp_dir.c_str())){
		cerr << "Cannot make default output destination, aborting" << endl;
		return 7;
	}
	Settings::container_config_dir = Settings::temp_dir + CONTAINER_CONFIG;
	if(!check_make_dir(Settings::container_config_dir.c_str())){
		cerr << "Cannot make container config destination, aborting" << endl;
		return 8;
	}
	
	if(settings[CONTAINER_TYPE]){
		Settings::container_type = settings[CONTAINER_TYPE].as<std::string>();
	} else {
		Settings::container_type = DEFAULT_CONTAINER;
	}

	return 0;
}
