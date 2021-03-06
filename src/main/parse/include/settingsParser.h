#ifndef __SETTINGS_PARSER_CONF_H_INCLUDED__
#define __SETTINGS_PARSER_CONF_H_INCLUDED__

// forward declared dependencies

// include dependencies
#include <string>

// declarations
enum class Mode {
    PARSE,
    CONTAINER_GEN,
    NS3_GEN, 
    NS3_RUN,
    NORMAL
};

void create_template_settings_file(std::string settings_file);

class Settings {
public:
    static Mode run_mode;
	static std::string ns3_path;
	static std::string output_dest;
    static std::string temp_dir;
    static std::string container_config_dir;
	static std::string script_dest;
    static std::string container_type;

    // static int parse_config_args();
	static int parse_settings_file(std::string settings_file);

    static bool genContainers(){ return (run_mode == Mode::NORMAL || run_mode == Mode::CONTAINER_GEN); }
    static bool genNS3() { return (run_mode == Mode::NORMAL || run_mode == Mode::NS3_GEN); }
    static bool runNS3() { return (run_mode == Mode::NORMAL || run_mode == Mode::NS3_RUN); }
};

#endif
