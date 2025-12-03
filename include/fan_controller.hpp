#ifndef FAN_CONTROLLER_HPP
#define FAN_CONTROLLER_HPP

/**
 * @file fan_controller.hpp
 * @brief Fan controller for Raspberry Pi 5 temperature-based fan speed control
 */

#include "config_parser.hpp"
#include <string>
#include <atomic>
#include <memory>

// Fan speed levels
enum class FanSpeed : int {
    OFF = 0,
    LOW = 1,
    MEDIUM = 2,
    HIGH = 3,
    FULL = 4
};

class FanController {
public:
    explicit FanController(const FanControllerConfig& config);
    ~FanController() = default;

    bool initialize();
    void run();
    void stop();

private:
    FanControllerConfig config_;
    std::atomic<FanSpeed> current_fan_speed_;
    std::atomic<bool> running_;

    double readTemperatureSensor(const std::string& temp_path) const;
    double getAverageTemperature() const;
    FanSpeed determineTargetSpeed(double temperature) const;
    bool checkHysteresis(double temperature, FanSpeed target_speed) const;
    double getThresholdForSpeed(FanSpeed speed) const;
    bool setFanSpeed(FanSpeed speed);
    FanSpeed readFanSpeed() const;
    bool verifyFanSpeedWrite(FanSpeed expected_speed) const;
    void logMessage(const std::string& message) const;
    void logDebug(const std::string& message) const;
    std::string fanSpeedToString(FanSpeed speed) const;
    std::string formatTemperature(double temp) const;
};

#endif // FAN_CONTROLLER_HPP

