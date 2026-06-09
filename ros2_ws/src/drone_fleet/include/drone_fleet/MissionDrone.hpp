#ifndef MISSION_DRONE_HPP
#define MISSION_DRONE_HPP

#include <vector>
#include <tuple>
#include <string>
#include <stdexcept>

class MissionDrone {
public:
    MissionDrone(const std::string& name, float battery, const std::string& mission,
                 const std::vector<std::tuple<float, float, float>>& waypoints)
        : drone_name_(name), battery_level_(battery), mission_name_(mission),
          waypoints_(waypoints), current_waypoint_index_(-1), altitude_(0.0f),
          speed_(0.0f), status_("Idle") {}

    void take_off(float target_altitude) {
        if (battery_level_ <= 10.0f) {
            throw std::runtime_error("Battery too low for takeoff");
        }
        altitude_ = target_altitude;
        status_ = "Flying";
        current_waypoint_index_ = 0;
    }

    void land() {
        altitude_ = 0.0f;
        status_ = "Landed";
        current_waypoint_index_ = -1;
    }

    void next_waypoint() {
        if (current_waypoint_index_ < static_cast<int>(waypoints_.size()) - 1) {
            current_waypoint_index_++;
            speed_ = 3.2f;
        }
    }

    void drain_battery(float amount) {
        battery_level_ = std::max(0.0f, battery_level_ - amount);
        if (battery_level_ <= 0.0f) {
            status_ = "Emergency_Land";
        }
    }

    float get_battery_level() const { return battery_level_; }
    float get_altitude() const { return altitude_; }
    float get_speed() const { return speed_; }
    std::string get_status() const { return status_; }
    int get_current_waypoint_index() const { return current_waypoint_index_; }
    size_t get_waypoint_count() const { return waypoints_.size(); }
    bool mission_complete() const { return current_waypoint_index_ >= static_cast<int>(waypoints_.size() - 1) && status_ == "Landed"; }
    bool is_critical() const { return battery_level_ <= 10.0f; }

    std::string mission_summary() const {
        return mission_name_ + " completed by " + drone_name_ + " with " +
               std::to_string(static_cast<int>(battery_level_)) + "% battery remaining.";
    }

private:
    std::string drone_name_;
    float battery_level_;
    std::string mission_name_;
    std::vector<std::tuple<float, float, float>> waypoints_;
    int current_waypoint_index_;
    float altitude_;
    float speed_;
    std::string status_;
};

#endif // MISSION_DRONE_HPP
