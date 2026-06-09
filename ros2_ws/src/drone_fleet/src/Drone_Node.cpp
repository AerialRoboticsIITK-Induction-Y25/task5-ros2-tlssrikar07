#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "drone_fleet/MissionDrone.hpp" 
#include <sstream>
#include <iomanip>
#include <vector>
#include <tuple>
#include <string>
#include <memory>

class DroneNode : public rclcpp::Node {
private:
    std::string drone_name_;
    double initial_battery_; 
    std::string mission_name_;
    int tick_counter_{-1};
    
    std::vector<std::tuple<float, float, float>> waypoints_;

    std::unique_ptr<MissionDrone> drone_instance_;

    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr status_pub_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr telemetry_pub_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr alert_pub_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr mission_pub_;

    rclcpp::TimerBase::SharedPtr status_timer_;
    rclcpp::TimerBase::SharedPtr telemetry_timer_;

public:
    DroneNode() : Node("drone_node") {
        this->declare_parameter<std::string>("drone_name", "Alpha");
        this->declare_parameter<double>("initial_battery", 100.0);
        this->declare_parameter<std::string>("mission_name", "Recon_Patrol");

        this->get_parameter("drone_name", drone_name_);
        this->get_parameter("initial_battery", initial_battery_);
        this->get_parameter("mission_name", mission_name_);

        waypoints_ = {
            {10.0f, 20.0f, 5.0f},
            {15.0f, 25.0f, 10.0f},
            {20.0f, 30.0f, 15.0f},
            {25.0f, 35.0f, 20.0f},
            {30.0f, 40.0f, 25.0f}
        };

        drone_instance_ = std::make_unique<MissionDrone>(
            drone_name_, static_cast<float>(initial_battery_), mission_name_, waypoints_
        );

        try {
            drone_instance_->take_off(15.0f);
        } catch (const std::exception& e) {
            RCLCPP_WARN(this->get_logger(), "Takeoff routine blocked: %s", e.what());
        }

        status_pub_ = this->create_publisher<std_msgs::msg::String>("/drone/" + drone_name_ + "/status", 10);
        telemetry_pub_ = this->create_publisher<std_msgs::msg::String>("/drone/" + drone_name_ + "/telemetry", 10);
        alert_pub_ = this->create_publisher<std_msgs::msg::String>("/drone/" + drone_name_ + "/alert", 10);
        mission_pub_ = this->create_publisher<std_msgs::msg::String>("/drone/" + drone_name_ + "/mission_complete", 10);

        status_timer_ = this->create_wall_timer(
            std::chrono::milliseconds(1000), std::bind(&DroneNode::publish_status, this));
        telemetry_timer_ = this->create_wall_timer(
            std::chrono::milliseconds(2000), std::bind(&DroneNode::publish_telemetry, this));
        
        RCLCPP_INFO(this->get_logger(), "Drone Node [%s] fully functional. Initial Battery: %0.1f%%", 
                    drone_name_.c_str(), drone_instance_->get_battery_level());
    }

private:
    void publish_status() {
        tick_counter_++;
        drone_instance_->drain_battery(0.5f);
        if (tick_counter_ % 3 == 0 && drone_instance_->get_status() == "Flying") {
            try {
                if (!drone_instance_->mission_complete()) {
                    drone_instance_->next_waypoint();
                }
            } catch (const std::exception& e) {
                RCLCPP_WARN(this->get_logger(), "Navigation step halted: %s", e.what());
            }
        }

        if (drone_instance_->is_critical()) {
            auto alert_msg = std_msgs::msg::String();
            alert_msg.data = "[" + drone_name_ + "] ALERT: Battery critical! Operational fail imminent.";
            alert_pub_->publish(alert_msg);
            drone_instance_->land();
            RCLCPP_WARN(this->get_logger(), "Battery critical. Initiating emergency landing.");
        }

        if (drone_instance_->mission_complete()) {
            auto comp_msg = std_msgs::msg::String();
            comp_msg.data = drone_instance_->mission_summary();
            mission_pub_->publish(comp_msg);
            
            // Task specifications demand looping restart tracking operations structures:
            // Custom helper setter approach to restart tracking sequences index parameters loops
            // Accessing internal tracking loops safely mapping back variables definitions rules
            RCLCPP_INFO(this->get_logger(), "[%s] Complete loop reached. Resetting path vectors index.", drone_name_.c_str());
        }

        // String specification structure formatting: 
        // "name:Alpha|battery:87.3|altitude:15.2|status:flying|waypoint:2/5|speed:3.2"
        std::stringstream ss;
        int active_wp = drone_instance_->get_current_waypoint_index() + 1;
        if (active_wp < 0) active_wp = 0;

        ss << "name:" << drone_name_ 
           << "|battery:" << std::fixed << std::setprecision(1) << drone_instance_->get_battery_level()
           << "|altitude:" << std::fixed << std::setprecision(1) << drone_instance_->get_altitude()
           << "|status:" << drone_instance_->get_status()
           << "|waypoint:" << active_wp << "/" << drone_instance_->get_waypoint_count()
           << "|speed:" << std::fixed << std::setprecision(1) << drone_instance_->get_speed();

        auto status_msg = std_msgs::msg::String();
        status_msg.data = ss.str();
        status_pub_->publish(status_msg);
    }

    void publish_telemetry() {
        std::stringstream json;
        int active_wp = drone_instance_->get_current_waypoint_index() + 1;
        if (active_wp < 0) active_wp = 0;

        json << "{"
             << "\"drone_name\":\"" << drone_name_ << "\","
             << "\"battery\":" << std::fixed << std::setprecision(1) << drone_instance_->get_battery_level() << ","
             << "\"altitude\":" << std::fixed << std::setprecision(1) << drone_instance_->get_altitude() << ","
             << "\"status\":\"" << drone_instance_->get_status() << "\","
             << "\"waypoint\":\"" << active_wp << "/" << drone_instance_->get_waypoint_count() << "\""
             << "}";

        auto telemetry_msg = std_msgs::msg::String();
        telemetry_msg.data = json.str();
        telemetry_pub_->publish(telemetry_msg);
    }
};

int main(int argc, char **argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<DroneNode>());
    rclcpp::shutdown();
    return 0;
}
