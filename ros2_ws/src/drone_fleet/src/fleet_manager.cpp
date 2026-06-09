#include <memory>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <chrono>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "std_srvs/srv/trigger.hpp"

using std::placeholders::_1;
using std::placeholders::_2;

// Simple structure to maintain extracted telemetry data states internally
struct DroneTelemetryData {
    std::string name;
    float battery;
    float altitude;
    std::string status;
    std::string waypoint;
};

class FleetManager : public rclcpp::Node {
public:
    FleetManager() : Node("fleet_manager") {
        // List of targets matching prompt requirements
        std::vector<std::string> target_drones = {"Alpha", "Beta", "Gamma"};

        // Step 1: Dynamically generate subscription matrices across the fleet array
        for (const auto& name : target_drones) {
            // Seed default values into state cache tracking dictionary
            fleet_state_cache_[name] = {name, 0.0f, 0.0f, "Offline", "0/0"};

            status_subs_.push_back(this->create_subscription<std_msgs::msg::String>(
                "/drone/" + name + "/status", 10,
                [this](const std_msgs::msg::String::SharedPtr msg) { this->status_callback(msg); }));

            alert_subs_.push_back(this->create_subscription<std_msgs::msg::String>(
                "/drone/" + name + "/alert", 10,
                [this](const std_msgs::msg::String::SharedPtr msg) { this->alert_callback(msg); }));

            mission_subs_.push_back(this->create_subscription<std_msgs::msg::String>(
                "/drone/" + name + "/mission_complete", 10,
                [this](const std_msgs::msg::String::SharedPtr msg) { this->mission_callback(msg); }));

            telemetry_subs_.push_back(this->create_subscription<std_msgs::msg::String>(
                "/drone/" + name + "/telemetry", 10,
                [this](const std_msgs::msg::String::SharedPtr msg) { this->telemetry_callback(msg); }));
        }

        // Step 2: Initialize immediate report Trigger service endpoint inside node class lifecycle scope
        report_service_ = this->create_service<std_srvs::srv::Trigger>(
            "/fleet/status_report", std::bind(&FleetManager::handle_report_service, this, _1, _2));

        // Step 3: Set up 5-second automatic console logging print reporting execution timer loop
        report_timer_ = this->create_wall_timer(
            std::chrono::seconds(5), std::bind(&FleetManager::print_fleet_report_table, this));
    }

private:
    // Core structural storage parameters mapping dictionary tracking instances
    std::map<std::string, DroneTelemetryData> fleet_state_cache_;

    // Comms Collections lists interfaces handles definitions layouts arrays
    std::vector<rclcpp::Subscription<std_msgs::msg::String>::SharedPtr> status_subs_;
    std::vector<rclcpp::Subscription<std_msgs::msg::String>::SharedPtr> alert_subs_;
    std::vector<rclcpp::Subscription<std_msgs::msg::String>::SharedPtr> mission_subs_;
    std::vector<rclcpp::Subscription<std_msgs::msg::String>::SharedPtr> telemetry_subs_;

    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr report_service_;
    rclcpp::TimerBase::SharedPtr report_timer_;

    // Manual string data index token parser to extract string items without external JSON libraries
    std::string extract_json_value(const std::string& json, const std::string& key, bool is_string = false) {
        std::string look_for = "\"" + key + "\":";
        size_t start_pos = json.find(look_for);
        if (start_pos == std::string::npos) return "";

        start_pos += look_for.length();
        
        if (is_string) {
            if (json[start_pos] == '"') start_pos++;
            size_t end_pos = json.find('"', start_pos);
            return json.substr(start_pos, end_pos - start_pos);
        } else {
            size_t end_pos = json.find_first_of(",}", start_pos);
            return json.substr(start_pos, end_pos - start_pos);
        }
    }

    void status_callback(const std_msgs::msg::String::SharedPtr msg) {
        (void)msg;
    }

    void alert_callback(const std_msgs::msg::String::SharedPtr msg) {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        struct tm* tm_ptr = std::gmtime(&time_t_now);
        
        char timestamp[32];
        std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", tm_ptr);

        RCLCPP_WARN(this->get_logger(), "[%s] CRITICAL FLEET INCIDENT DETECTED: %s", timestamp, msg->data.c_str());
    }

    void mission_callback(const std_msgs::msg::String::SharedPtr msg) {
        RCLCPP_INFO(this->get_logger(), "Mission Complete:\n%s", msg->data.c_str());
    }

    void telemetry_callback(const std_msgs::msg::String::SharedPtr msg) {
        std::string raw_json = msg->data;

        // Perform text scans manually to populate local struct fields cleanly
        std::string name = extract_json_value(raw_json, "drone_name", true);
        if (name.empty()) return;

        try {
            fleet_state_cache_[name].name = name;
            fleet_state_cache_[name].battery = std::stof(extract_json_value(raw_json, "battery", false));
            fleet_state_cache_[name].altitude = std::stof(extract_json_value(raw_json, "altitude", false));
            fleet_state_cache_[name].status = extract_json_value(raw_json, "status", true);
            fleet_state_cache_[name].waypoint = extract_json_value(raw_json, "waypoint", true);
        } catch (...) {
            // Protect against empty parsing segments during data stream init states
        }
    }

    std::string generate_report_string() {
        std::stringstream ss;
        ss << " | " << std::left << std::setw(10) << "Drone"
           << " | " << std::setw(10) << "Battery"
           << " | " << std::setw(10) << "Altitude"
           << " | " << std::setw(10) << "Waypoint"
           << " | " << std::setw(12) << "Status" << "\n";
        for (const auto& [name, data] : fleet_state_cache_) {
            ss << " | " << std::left << std::setw(10) << data.name
               << " | " << std::fixed << std::setprecision(1) << std::setw(8) << data.battery << "%"
               << " | " << std::setw(8) << data.altitude << "m"
               << " | " << std::setw(10) << data.waypoint
               << " | " << std::setw(12) << data.status << "\n";
        }
        return ss.str();
    }

    void print_fleet_report_table() {
        std::cout << generate_report_string() << std::flush;
    }

    void handle_report_service(const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
                               std::shared_ptr<std_srvs::srv::Trigger::Response> response) {
        (void)request;
        response->success = true;
        response->message = generate_report_string();
        RCLCPP_INFO(this->get_logger(), "Dispatched Fleet Status Report ");
    }
};

int main(int argc, char* argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<FleetManager>());
    rclcpp::shutdown();
    return 0;
}
