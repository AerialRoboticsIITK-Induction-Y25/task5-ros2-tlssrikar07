#include <memory>
#include <string>
#include <vector>
#include <map>
#include <deque>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <chrono>
#include <cmath>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

struct BatterySample {
    std::chrono::steady_clock::time_point timestamp;
    float battery_level;
};

struct DroneHealthState {
    std::string name;
    float current_battery{100.0f};
    float current_altitude{0.0f};
    std::string current_status{"Unknown"};
    std::string current_waypoint{"0/0"};
    std::deque<BatterySample> history; 
    float drain_rate_per_second{0.0f};
};

class HealthMonitor : public rclcpp::Node {
public:
    HealthMonitor() : Node("health_monitor") {
        std::vector<std::string> target_drones = {"Alpha", "Beta", "Gamma"};

        warning_pub_ = this->create_publisher<std_msgs::msg::String>("/fleet/health_warning", 10);
        summary_pub_ = this->create_publisher<std_msgs::msg::String>("/fleet/health_summary", 10);

        for (const auto& name : target_drones) {
            fleet_health_map_[name] = {name, 100.0f, 0.0f, "Offline", "0/0", {}, 0.0f};

            telemetry_subs_.push_back(this->create_subscription<std_msgs::msg::String>(
                "/drone/" + name + "/telemetry", 10,
                [this](const std_msgs::msg::String::SharedPtr msg) { this->telemetry_callback(msg); }));
        }

        diagnostics_timer_ = this->create_wall_timer(
            std::chrono::seconds(10), std::bind(&HealthMonitor::process_and_print_diagnostics, this));

        RCLCPP_INFO(this->get_logger(), "Health Monitor diagnostics subsystem active.");
    }

private:
    std::map<std::string, DroneHealthState> fleet_health_map_;
    
    std::vector<rclcpp::Subscription<std_msgs::msg::String>::SharedPtr> telemetry_subs_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr warning_pub_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr summary_pub_;
    
    rclcpp::TimerBase::SharedPtr diagnostics_timer_;

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

    void telemetry_callback(const std_msgs::msg::String::SharedPtr msg) {
        std::string raw_json = msg->data;
        std::string name = extract_json_value(raw_json, "drone_name", true);
        if (name.empty()) return;

        try {
            float battery = std::stof(extract_json_value(raw_json, "battery", false));
            auto& drone = fleet_health_map_[name];
            
            drone.current_battery = battery;
            drone.current_altitude = std::stof(extract_json_value(raw_json, "altitude", false));
            drone.current_status = extract_json_value(raw_json, "status", true);
            drone.current_waypoint = extract_json_value(raw_json, "waypoint", true);

            drone.history.push_back({std::chrono::steady_clock::now(), battery});
            if (drone.history.size() > 10) {
                drone.history.pop_front();
            }

            calculate_drain_rate(drone);
        } catch (...) {}
    }

    void calculate_drain_rate(DroneHealthState& drone) {
        if (drone.history.size() < 2) return;

        const auto& first_sample = drone.history.front();
        const auto& latest_sample = drone.history.back();

        std::chrono::duration<float> elapsed = latest_sample.timestamp - first_sample.timestamp;
        float time_seconds = elapsed.count();

        if (time_seconds <= 0.001f) return;

        float battery_delta = first_sample.battery_level - latest_sample.battery_level;
        drone.drain_rate_per_second = battery_delta / time_seconds;

        if (drone.drain_rate_per_second > 1.5f) {
            auto warn_msg = std_msgs::msg::String();
            warn_msg.data = "CRITICAL WARNING: [" + drone.name + "] Drain Rate Exceeds threshold: " + 
                            std::to_string(drone.drain_rate_per_second) + "% per second!";
            warning_pub_->publish(warn_msg);
        }
    }

    void process_and_print_diagnostics() {
        std::stringstream ss;
        std::stringstream json_summary;

        ss << "\n========================================================================================\n"
           << "                               FLEET HEALTH DIAGNOSTICS                                  \n"
           << "========================================================================================\n"
           << " | " << std::left << std::setw(8) << "Drone"
           << " | " << std::setw(12) << "Drain Rate/s"
           << " | " << std::setw(12) << "Cur Battery"
           << " | " << std::setw(20) << "Est. Time to Critical"
           << " | " << std::setw(20) << "Est. Time Depletion" << " |\n"
           << "----------------------------------------------------------------------------------------\n";

        json_summary << "{ \"fleet_summary\": [";
        bool first = true;

        for (const auto& [name, drone] : fleet_health_map_) {
            float time_to_crit = -1.0f;
            float time_to_depleted = -1.0f;

            if (drone.drain_rate_per_second > 0.001f) {
                if (drone.current_battery > 30.0f) {
                    time_to_crit = (drone.current_battery - 30.0f) / drone.drain_rate_per_second;
                } else {
                    time_to_crit = 0.0f;
                }
                time_to_depleted = drone.current_battery / drone.drain_rate_per_second;
            }

            auto format_time_string = [](float seconds) -> std::string {
                if (seconds < 0) return "N/A (No Drain)";
                if (seconds == 0) return "CRITICAL NOW";
                std::stringstream ts;
                ts << std::fixed << std::setprecision(1) << seconds << "s";
                return ts.str();
            };

            ss << " | " << std::left << std::setw(8) << drone.name
               << " | " << std::fixed << std::setprecision(2) << std::setw(10) << drone.drain_rate_per_second << "%/s"
               << " | " << std::setw(10) << drone.current_battery << "%"
               << " | " << std::setw(20) << format_time_string(time_to_crit)
               << " | " << std::setw(20) << format_time_string(time_to_depleted) << " |\n";

            if (!first) json_summary << ",";
            first = false;
            json_summary << "{"
                         << "\"drone\":\"" << drone.name << "\","
                         << "\"drain_rate\":" << std::fixed << std::setprecision(2) << drone.drain_rate_per_second << ","
                         << "\"time_to_critical\":" << time_to_crit << ","
                         << "\"time_to_depletion\":" << time_to_depleted
                         << "}";
        }
        ss << "========================================================================================\n";
        json_summary << "]}";

        std::cout << ss.str() << std::flush;

        auto summary_msg = std_msgs::msg::String();
        summary_msg.data = json_summary.str();
        summary_pub_->publish(summary_msg);
    }
};

int main(int argc, char* argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<HealthMonitor>());
    rclcpp::shutdown();
    return 0;
}
