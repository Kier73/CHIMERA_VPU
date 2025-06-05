#ifndef IOT_CLIENT_H
#define IOT_CLIENT_H

#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp> // For JSON parsing
#include "httplib.h"         // cpp-httplib

// Forward declaration for DeviceDescriptor if it's complex,
// or define a simplified version here.
// For now, we'll use nlohmann::json to represent device data flexibly.

class IoTClient {
public:
    IoTClient(const std::string& server_address, int server_port);

    // Fetches all registered devices from the IoT framework
    // Returns a JSON object representing the list of devices, or an empty JSON if error.
    nlohmann::json listDevices();

    // Fetches the status of a specific device
    // device_id: The ID of the device
    // Returns a JSON object representing the device status, or an empty JSON if error.
    nlohmann::json getDeviceStatus(const std::string& device_id);

    // Sends a command to a specific device
    // device_id: The ID of the device
    // command: The command string to send
    // params: A map of parameters for the command
    // Returns a JSON object representing the result of the command, or an empty JSON if error.
    nlohmann::json sendDeviceCommand(
        const std::string& device_id,
        const std::string& command,
        const nlohmann::json& params
    );

private:
    httplib::Client httpClient;
    std::string server_address_;
    int server_port_;

    // Helper to parse JSON response
    nlohmann::json parseResponse(const httplib::Result& res);
};

#endif // IOT_CLIENT_H
