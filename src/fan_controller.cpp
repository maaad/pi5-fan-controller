/**
 * @file fan_controller.cpp
 * @brief Implementation of fan controller for Raspberry Pi 5
 */

#include "fan_controller.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <vector>
#include <iomanip>
#include <unistd.h>
#include <fcntl.h>
#include <filesystem>

namespace fs = std::filesystem;

FanController::FanController(const FanControllerConfig& config)
    : config_(config)
    , current_fan_speed_(FanSpeed::OFF)
    , running_(false)
{
}

bool FanController::initialize() {
    // Validate paths
    if (!fs::exists(config_.fan_path)) {
        std::cerr << "Fan control file does not exist: " << config_.fan_path << std::endl;
        return false;
    }

    if (config_.temp_hwmon0_path.empty() && config_.temp_hwmon1_path.empty()) {
        std::cerr << "No temperature sensor paths configured" << std::endl;
        return false;
    }

    // Read current fan speed
    current_fan_speed_ = readFanSpeed();

    // Validate thresholds
    if (config_.off_threshold >= config_.low_threshold ||
        config_.low_threshold >= config_.medium_threshold ||
        config_.medium_threshold >= config_.high_threshold ||
        config_.high_threshold >= config_.full_threshold) {
        std::cerr << "Temperature thresholds not in ascending order" << std::endl;
        return false;
    }

    std::string speed_name = fanSpeedToString(current_fan_speed_.load());
    std::string msg = "Fan controller initialized: current speed " + speed_name +
                      " (" + std::to_string(static_cast<int>(current_fan_speed_.load())) + "), "
                      "thresholds OFF<" + formatTemperature(config_.off_threshold) + "°C "
                      "LOW<" + formatTemperature(config_.low_threshold) + "°C "
                      "MEDIUM<" + formatTemperature(config_.medium_threshold) + "°C "
                      "HIGH<" + formatTemperature(config_.high_threshold) + "°C "
                      "FULL>=" + formatTemperature(config_.full_threshold) + "°C, "
                      "hysteresis=" + formatTemperature(config_.hysteresis) + "°C";
    logMessage(msg);

    return true;
}

void FanController::run() {
    running_ = true;

    while (running_) {
        double temp_average = getAverageTemperature();

        if (std::isnan(temp_average)) {
            logDebug("Failed to read temperature, skipping this cycle");
            sleep(config_.interval_seconds);
            continue;
        }

        FanSpeed target_speed = determineTargetSpeed(temp_average);

        if (checkHysteresis(temp_average, target_speed)) {
            if (target_speed != current_fan_speed_.load()) {
                FanSpeed old_speed = current_fan_speed_.load();
                if (setFanSpeed(target_speed)) {
                    std::string msg = "T:" + formatTemperature(temp_average) + "°C S:" +
                                    fanSpeedToString(old_speed) + " -> " +
                                    fanSpeedToString(target_speed);
                    logMessage(msg);
                } else {
                    // Re-sync fan speed from hardware
                    current_fan_speed_ = readFanSpeed();
                }
            }
        } else if (config_.debug) {
            std::string msg = "T:" + formatTemperature(temp_average) + "°C S:" +
                            fanSpeedToString(current_fan_speed_.load());
            logDebug(msg);
        }

        sleep(config_.interval_seconds);
    }
}

void FanController::stop() {
    running_ = false;
}

double FanController::readTemperatureSensor(const std::string& temp_path) const {
    if (!fs::exists(temp_path)) {
        if (config_.debug) {
            logDebug("Temperature sensor path does not exist: " + temp_path);
        }
        return std::nan("");
    }

    std::ifstream temp_file(temp_path);
    if (!temp_file.is_open()) {
        if (config_.debug) {
            logDebug("Failed to open temperature sensor: " + temp_path);
        }
        return std::nan("");
    }

    std::string temp_str;
    if (!std::getline(temp_file, temp_str)) {
        if (config_.debug) {
            logDebug("Temperature sensor file is empty: " + temp_path);
        }
        return std::nan("");
    }

    try {
        int temp_millicelsius = std::stoi(temp_str);
        if (temp_millicelsius < 0) {
            if (config_.debug) {
                logDebug("Negative temperature read from " + temp_path + ": " +
                        std::to_string(temp_millicelsius) + " m°C");
            }
            return std::nan("");
        }

        double temp_celsius = temp_millicelsius / 1000.0;
        if (temp_celsius < -50.0 || temp_celsius > 150.0) {
            std::cerr << "Unreasonable temperature read from " << temp_path << ": "
                      << temp_celsius << "°C" << std::endl;
            return std::nan("");
        }

        return temp_celsius;
    } catch (const std::exception& e) {
        std::cerr << "Invalid temperature value from " << temp_path << ": " << e.what() << std::endl;
        return std::nan("");
    }
}

double FanController::getAverageTemperature() const {
    std::vector<double> temps;
    std::vector<std::string> failed_sensors;

    if (!config_.temp_hwmon0_path.empty()) {
        double temp = readTemperatureSensor(config_.temp_hwmon0_path);
        if (!std::isnan(temp)) {
            temps.push_back(temp);
        } else {
            failed_sensors.push_back(config_.temp_hwmon0_path);
        }
    }

    if (!config_.temp_hwmon1_path.empty()) {
        double temp = readTemperatureSensor(config_.temp_hwmon1_path);
        if (!std::isnan(temp)) {
            temps.push_back(temp);
        } else {
            failed_sensors.push_back(config_.temp_hwmon1_path);
        }
    }

    if (temps.empty()) {
        std::cerr << "All temperature sensors failed, cannot read temperature" << std::endl;
        return std::nan("");
    }

    if (!failed_sensors.empty()) {
        std::string msg = "Using " + std::to_string(temps.size()) + " sensor(s), " +
                         std::to_string(failed_sensors.size()) + " sensor(s) failed";
        logDebug(msg);
    }

    double sum = 0.0;
    for (double temp : temps) {
        sum += temp;
    }
    return sum / temps.size();
}

FanSpeed FanController::determineTargetSpeed(double temperature) const {
    if (temperature >= config_.full_threshold) {
        return FanSpeed::FULL;
    } else if (temperature >= config_.high_threshold) {
        return FanSpeed::HIGH;
    } else if (temperature >= config_.medium_threshold) {
        return FanSpeed::MEDIUM;
    } else if (temperature >= config_.low_threshold) {
        return FanSpeed::LOW;
    } else {
        return FanSpeed::OFF;
    }
}

bool FanController::checkHysteresis(double temperature, FanSpeed target_speed) const {
    if (config_.hysteresis <= 0.0) {
        return true;
    }

    FanSpeed current_speed = current_fan_speed_.load();

    // Always allow speed increase
    if (target_speed > current_speed) {
        return true;
    }

    // For speed decrease, check hysteresis
    if (target_speed < current_speed) {
        double threshold = getThresholdForSpeed(target_speed);
        return temperature <= (threshold - config_.hysteresis);
    }

    return true;
}

double FanController::getThresholdForSpeed(FanSpeed speed) const {
    switch (speed) {
        case FanSpeed::OFF:
            return config_.low_threshold;
        case FanSpeed::LOW:
            return config_.medium_threshold;
        case FanSpeed::MEDIUM:
            return config_.high_threshold;
        case FanSpeed::HIGH:
        case FanSpeed::FULL:
            return config_.full_threshold;
        default:
            return config_.full_threshold;
    }
}

bool FanController::setFanSpeed(FanSpeed speed) {
    if (speed == current_fan_speed_.load()) {
        return true;
    }

    int speed_value = static_cast<int>(speed);
    if (speed_value < 0 || speed_value > 4) {
        std::cerr << "Invalid fan speed level: " << speed_value << std::endl;
        return false;
    }

    if (!fs::exists(config_.fan_path)) {
        std::cerr << "Fan control file does not exist: " << config_.fan_path << std::endl;
        return false;
    }

    try {
        std::ofstream fan_file(config_.fan_path);
        if (!fan_file.is_open()) {
            std::cerr << "Failed to open fan control file: " << config_.fan_path << std::endl;
            return false;
        }

        fan_file << speed_value;
        fan_file.flush();
        fan_file.close();

        // Use direct file descriptor for fsync
        int fd = open(config_.fan_path.c_str(), O_WRONLY);
        if (fd >= 0) {
            fsync(fd);
            close(fd);
        }

        // Small delay for write to complete
        usleep(100000); // 0.1 seconds

        if (!verifyFanSpeedWrite(speed)) {
            return false;
        }

        current_fan_speed_.store(speed);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to set fan speed: " << e.what() << " (path: "
                  << config_.fan_path << ")" << std::endl;
        return false;
    }
}

FanSpeed FanController::readFanSpeed() const {
    if (!fs::exists(config_.fan_path)) {
        std::cerr << "Fan control file does not exist: " << config_.fan_path << std::endl;
        return FanSpeed::OFF;
    }

    std::ifstream fan_file(config_.fan_path);
    if (!fan_file.is_open()) {
        std::cerr << "Could not read current fan speed, starting with OFF" << std::endl;
        return FanSpeed::OFF;
    }

    std::string speed_str;
    if (!std::getline(fan_file, speed_str)) {
        std::cerr << "Fan control file is empty, starting with OFF" << std::endl;
        return FanSpeed::OFF;
    }

    try {
        int speed_value = std::stoi(speed_str);
        if (speed_value >= 0 && speed_value <= 4) {
            return static_cast<FanSpeed>(speed_value);
        } else {
            std::cerr << "Invalid fan speed read from hardware: " << speed_value
                      << ", starting with OFF" << std::endl;
            return FanSpeed::OFF;
        }
    } catch (const std::exception& e) {
        std::cerr << "Invalid value in fan control file: " << e.what()
                  << ", starting with OFF" << std::endl;
        return FanSpeed::OFF;
    }
}

bool FanController::verifyFanSpeedWrite(FanSpeed expected_speed) const {
    FanSpeed actual_speed = readFanSpeed();
    if (actual_speed != expected_speed) {
        std::cerr << "Fan speed write verification failed: wrote "
                  << static_cast<int>(expected_speed) << ", read "
                  << static_cast<int>(actual_speed) << std::endl;
        return false;
    }
    return true;
}

void FanController::logMessage(const std::string& message) const {
    // Write to stdout - systemd automatically captures stdout/stderr to journald
    std::cout << message << std::endl;
}

void FanController::logDebug(const std::string& message) const {
    if (config_.debug) {
        // Write to stderr - systemd automatically captures stdout/stderr to journald
        std::cerr << message << std::endl;
    }
}

std::string FanController::fanSpeedToString(FanSpeed speed) const {
    switch (speed) {
        case FanSpeed::OFF:
            return "OFF";
        case FanSpeed::LOW:
            return "LOW";
        case FanSpeed::MEDIUM:
            return "MEDIUM";
        case FanSpeed::HIGH:
            return "HIGH";
        case FanSpeed::FULL:
            return "FULL";
        default:
            return "UNKNOWN";
    }
}

std::string FanController::formatTemperature(double temp) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << temp;
    std::string result = oss.str();
    // Remove trailing zeros and decimal point if not needed
    size_t dot_pos = result.find('.');
    if (dot_pos != std::string::npos) {
        // Remove trailing zeros
        while (result.size() > dot_pos + 1 && result.back() == '0') {
            result.pop_back();
        }
        // Remove decimal point if no fractional part
        if (result.size() == dot_pos + 1) {
            result.pop_back();
        }
    }
    return result;
}

