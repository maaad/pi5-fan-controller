/**
 * @file config_parser.cpp
 * @brief Implementation of configuration file and environment variable parsing
 */

#include "config_parser.hpp"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <dirent.h>

namespace fs = std::filesystem;

std::string ConfigParser::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t");
    if (first == std::string::npos) {
        return "";
    }
    size_t last = str.find_last_not_of(" \t");
    return str.substr(first, (last - first + 1));
}

std::map<std::string, std::string> ConfigParser::parseKeyValueFile(const std::string& path) {
    std::map<std::string, std::string> result;
    std::ifstream file(path);

    if (!file.is_open()) {
        return result;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }

        size_t eq_pos = line.find('=');
        if (eq_pos == std::string::npos) {
            continue;
        }

        std::string key = trim(line.substr(0, eq_pos));
        std::string value = trim(line.substr(eq_pos + 1));

        if (!key.empty() && !value.empty()) {
            result[key] = value;
        }
    }

    return result;
}

std::string ConfigParser::findHwmonDeviceByName(const std::string& device_name) {
    const std::string hwmon_base_path = "/sys/class/hwmon";

    if (!fs::exists(hwmon_base_path)) {
        return "";
    }

    for (const auto& entry : fs::directory_iterator(hwmon_base_path)) {
        if (!entry.is_directory()) {
            continue;
        }

        std::string entry_name = entry.path().filename().string();
        if (entry_name.find("hwmon") != 0) {
            continue;
        }

        std::string name_file = entry.path() / "name";
        if (!fs::exists(name_file)) {
            continue;
        }

        std::ifstream name_stream(name_file);
        std::string name;
        if (std::getline(name_stream, name)) {
            name = trim(name);
            if (name == device_name) {
                std::string temp_input = entry.path() / "temp1_input";
                if (fs::exists(temp_input)) {
                    return temp_input;
                }
            }
        }
    }

    return "";
}

FanControllerConfig ConfigParser::parseConfigFile(const std::string& config_path) {
    FanControllerConfig config = getDefaultConfig();
    auto kv_map = parseKeyValueFile(config_path);

    if (kv_map.find("FAN_PATH") != kv_map.end()) {
        config.fan_path = kv_map["FAN_PATH"];
    }
    if (kv_map.find("HWMON0_NAME") != kv_map.end()) {
        config.hwmon0_name = kv_map["HWMON0_NAME"];
    }
    if (kv_map.find("HWMON1_NAME") != kv_map.end()) {
        config.hwmon1_name = kv_map["HWMON1_NAME"];
    }
    if (kv_map.find("TEMP_HWMON0_PATH") != kv_map.end()) {
        config.temp_hwmon0_path = kv_map["TEMP_HWMON0_PATH"];
    }
    if (kv_map.find("TEMP_HWMON1_PATH") != kv_map.end()) {
        config.temp_hwmon1_path = kv_map["TEMP_HWMON1_PATH"];
    }
    if (kv_map.find("HYSTERESIS") != kv_map.end()) {
        config.hysteresis = std::stod(kv_map["HYSTERESIS"]);
    }
    if (kv_map.find("OFF_THRESHOLD") != kv_map.end()) {
        config.off_threshold = std::stod(kv_map["OFF_THRESHOLD"]);
    }
    if (kv_map.find("LOW_THRESHOLD") != kv_map.end()) {
        config.low_threshold = std::stod(kv_map["LOW_THRESHOLD"]);
    }
    if (kv_map.find("MEDIUM_THRESHOLD") != kv_map.end()) {
        config.medium_threshold = std::stod(kv_map["MEDIUM_THRESHOLD"]);
    }
    if (kv_map.find("HIGH_THRESHOLD") != kv_map.end()) {
        config.high_threshold = std::stod(kv_map["HIGH_THRESHOLD"]);
    }
    if (kv_map.find("FULL_THRESHOLD") != kv_map.end()) {
        config.full_threshold = std::stod(kv_map["FULL_THRESHOLD"]);
    }
    if (kv_map.find("INTERVAL_SECONDS") != kv_map.end()) {
        config.interval_seconds = std::stoi(kv_map["INTERVAL_SECONDS"]);
    }
    if (kv_map.find("DEBUG") != kv_map.end()) {
        std::string val = kv_map["DEBUG"];
        std::transform(val.begin(), val.end(), val.begin(), ::tolower);
        config.debug = (val == "true" || val == "1" || val == "yes");
    }

    // Find hwmon devices if paths not specified
    if (config.temp_hwmon0_path.empty()) {
        config.temp_hwmon0_path = findHwmonDeviceByName(config.hwmon0_name);
    }
    if (config.temp_hwmon1_path.empty()) {
        config.temp_hwmon1_path = findHwmonDeviceByName(config.hwmon1_name);
    }

    return config;
}

FanControllerConfig ConfigParser::parseEnvironment() {
    FanControllerConfig config = getDefaultConfig();

    const char* env_val;

    if ((env_val = std::getenv("FAN_PATH")) != nullptr) {
        config.fan_path = env_val;
    }
    if ((env_val = std::getenv("HWMON0_NAME")) != nullptr) {
        config.hwmon0_name = env_val;
    }
    if ((env_val = std::getenv("HWMON1_NAME")) != nullptr) {
        config.hwmon1_name = env_val;
    }
    if ((env_val = std::getenv("TEMP_HWMON0_PATH")) != nullptr) {
        config.temp_hwmon0_path = env_val;
    }
    if ((env_val = std::getenv("TEMP_HWMON1_PATH")) != nullptr) {
        config.temp_hwmon1_path = env_val;
    }
    if ((env_val = std::getenv("HYSTERESIS")) != nullptr) {
        config.hysteresis = std::stod(env_val);
    }
    if ((env_val = std::getenv("OFF_THRESHOLD")) != nullptr) {
        config.off_threshold = std::stod(env_val);
    }
    if ((env_val = std::getenv("LOW_THRESHOLD")) != nullptr) {
        config.low_threshold = std::stod(env_val);
    }
    if ((env_val = std::getenv("MEDIUM_THRESHOLD")) != nullptr) {
        config.medium_threshold = std::stod(env_val);
    }
    if ((env_val = std::getenv("HIGH_THRESHOLD")) != nullptr) {
        config.high_threshold = std::stod(env_val);
    }
    if ((env_val = std::getenv("FULL_THRESHOLD")) != nullptr) {
        config.full_threshold = std::stod(env_val);
    }
    if ((env_val = std::getenv("INTERVAL_SECONDS")) != nullptr) {
        config.interval_seconds = std::stoi(env_val);
    }
    if ((env_val = std::getenv("DEBUG")) != nullptr) {
        std::string val = env_val;
        std::transform(val.begin(), val.end(), val.begin(), ::tolower);
        config.debug = (val == "true" || val == "1" || val == "yes");
    }

    // Find hwmon devices if paths not specified
    if (config.temp_hwmon0_path.empty()) {
        config.temp_hwmon0_path = findHwmonDeviceByName(config.hwmon0_name);
    }
    if (config.temp_hwmon1_path.empty()) {
        config.temp_hwmon1_path = findHwmonDeviceByName(config.hwmon1_name);
    }

    return config;
}

FanControllerConfig ConfigParser::getDefaultConfig() {
    FanControllerConfig config;
    return config;
}

