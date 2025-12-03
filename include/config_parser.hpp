#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

/**
 * @file config_parser.hpp
 * @brief Configuration parsing for Pi5 Fan Controller
 */

#include <string>
#include <map>
#include <cstdint>

// Configuration structure with default values
struct FanControllerConfig {
    std::string fan_path = "/sys/class/thermal/cooling_device0/cur_state";
    std::string hwmon0_name = "cpu_thermal";
    std::string hwmon1_name = "rp1_adc";
    std::string temp_hwmon0_path;
    std::string temp_hwmon1_path;

    double hysteresis = 2.0;
    double off_threshold = 53.0;
    double low_threshold = 54.0;
    double medium_threshold = 59.0;
    double high_threshold = 64.0;
    double full_threshold = 70.0;

    int interval_seconds = 15;
    bool debug = false;
};

class ConfigParser {
public:
    static FanControllerConfig parseConfigFile(const std::string& config_path);
    static FanControllerConfig parseEnvironment();
    static FanControllerConfig getDefaultConfig();

private:
    static std::string trim(const std::string& str);
    static std::map<std::string, std::string> parseKeyValueFile(const std::string& path);
    static std::string findHwmonDeviceByName(const std::string& device_name);
};

#endif // CONFIG_PARSER_HPP

