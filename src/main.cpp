/**
 * @file main.cpp
 * @brief Main entry point for Pi5 Fan Controller application
 */

#include "fan_controller.hpp"
#include "config_parser.hpp"
#include <iostream>
#include <csignal>
#include <cstdlib>
#include <unistd.h>

// Global controller instance for signal handler access
static FanController* g_controller = nullptr;

/**
 * @brief Signal handler for graceful shutdown
 * @param signal Signal number (SIGINT, SIGTERM)
 */
void signalHandler(int signal) {
    if (g_controller) {
        std::cerr << "Received signal " << signal << ", shutting down..." << std::endl;
        g_controller->stop();
    }
}

/**
 * @brief Main entry point
 *
 * Parses configuration from file, environment variables, or command line,
 * initializes the controller, and runs the main control loop.
 */
int main(int argc, char* argv[]) {
    // Parse configuration with priority: config file > environment > defaults
    FanControllerConfig config;

    // Try configuration file first
    const char* config_path = "/etc/pi5-fan-controller/pi5-fan-controller.conf";
    if (access(config_path, R_OK) == 0) {
        config = ConfigParser::parseConfigFile(config_path);
    } else {
        // Fall back to environment variables or defaults
        config = ConfigParser::parseEnvironment();
    }

    // Override with command line arguments if provided
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--config" && i + 1 < argc) {
            config = ConfigParser::parseConfigFile(argv[++i]);
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [--config <path>] [--help]\n";
            std::cout << "Configuration file: /etc/pi5-fan-controller/pi5-fan-controller.conf\n";
            std::cout << "Environment variables: FAN_PATH, HWMON0_NAME, HWMON1_NAME, etc.\n";
            return 0;
        }
    }

    // Create controller
    FanController controller(config);
    g_controller = &controller;

    // Register signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Initialize controller
    if (!controller.initialize()) {
        std::cerr << "Failed to initialize fan controller" << std::endl;
        return 1;
    }

    // Run control loop
    controller.run();

    return 0;
}

