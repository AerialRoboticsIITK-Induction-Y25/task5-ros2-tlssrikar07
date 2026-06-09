#include <iostream>
#include <vector>
#include <string>
#include <exception>
#include <tuple>
#include <utility>
#include <chrono>
#include <sstream>
#include <iomanip>

class BatteryDepletedError : public std::exception {
public:
    const char* what() const noexcept override { return "BatteryDepletedError: Battery level is at 0%."; }
};

class InvalidStateError : public std::exception {
public:
    const char* what() const noexcept override { return "InvalidStateError: Operation invalid for current state."; }
};

class AltitudeError : public std::exception {
public:
    const char* what() const noexcept override { return "AltitudeError: Target exceeds maximum allowable altitude."; }
};

class Vehicle {
private:
    std::string name;
    float battery_level; 
    std::string status;   
    std::vector<std::string> flight_log;

protected:
    std::string get_timestamp() const {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&time_t_now), "%Y-%m-%dT%H:%M:%SZ");
        return ss.str();
    }

    void set_status(const std::string& new_status) {
        if (new_status != "Idle" && new_status != "Flying" && new_status != "Charging") {
            throw InvalidStateError();
        }
        status = new_status;
        log_event("Status changed to: " + status);
    }

    void log_event(const std::string& event) {
        flight_log.push_back("[" + get_timestamp() + "] " + event);
    }

public:
    Vehicle(std::string v_name, float initial_bat) 
        : name(std::move(v_name)), battery_level(initial_bat), status("Idle") {
        if (battery_level > 100.0f) battery_level = 100.0f;
        if (battery_level < 0.0f) battery_level = 0.0f;
    }
    
    virtual ~Vehicle() = default;

    virtual void get_info() const = 0; 

    std::string get_name() const { return name; }
    float get_battery_level() const { return battery_level; }
    std::string get_status() const { return status; }

    std::string get_flight_log() const {
        std::stringstream ss;
        for (const auto& log : flight_log) {
            ss << log << "\n";
        }
        return ss.str();
    }

    void drain_battery(float amount) {
        if (battery_level <= 0.0f) {
            throw BatteryDepletedError();
        }
        battery_level -= amount;
        if (battery_level <= 0.0f) {
            battery_level = 0.0f;
            log_event("Battery fully depleted.");
            throw BatteryDepletedError();
        }
    }

    void charge_battery(float amount, int duration_seconds) {
        if (status != "Charging") {
            throw InvalidStateError();
		}
        float added = amount * (duration_seconds / 10.0f); 
        battery_level += added;
        if (battery_level > 100.0f) battery_level = 100.0f;
        log_event("Charged battery level up to " + std::to_string(battery_level) + "%");
    }

    bool is_critical() const {
        return battery_level <= 30.0f;
    }
};

class Drone : public Vehicle {
protected:
    float altitude{0.0f};
    float max_altitude{120.0f};
private:
    float speed{0.0f};

public:
    Drone(std::string name, float battery, float max_alt = 150.0f, float spd = 5.0f) 
        : Vehicle(std::move(name), battery), max_altitude(max_alt), speed(spd) {}

    void get_info() const override {
        std::cout
                  << "Name: " << get_name() << " | Status: " << get_status() << "\n"
                  << "Battery: " << get_battery_level() << "% | Altitude: " << altitude 
                  << "m / " << max_altitude << "m | Speed: " << speed << " m/s\n";
    }

    void take_off(float target_altitude) {
        if (get_battery_level() <= 0.0f) throw BatteryDepletedError();
        if (get_status() != "Idle") throw InvalidStateError();
        if (target_altitude > max_altitude) throw AltitudeError();

        altitude = target_altitude;
        set_status("Flying");
        log_event("Successful takeoff to altitude: " + std::to_string(altitude) + "m");
    }

    void land() {
        if (get_status() != "Flying") throw InvalidStateError();
        log_event("Descending to land from " + std::to_string(altitude) + "m");
        altitude = 0.0f;
        speed = 0.0f;
        set_status("Idle");
    }

    void emergency_stop() {
        log_event("EMERGENCY STOP");
        altitude = 0.0f;
        speed = 0.0f;
        set_status("Idle");
        drain_battery(30.0f); 
    }
};

class MissionDrone : public Drone {
private:
    std::vector<std::pair<std::tuple<float, float, float>, std::string>> visited_waypoints;

protected:
    std::string mission_name;
    std::vector<std::tuple<float, float, float>> waypoints;
    int current_waypoint_index{-1};

public:
    MissionDrone(std::string name, float battery, std::string m_name, std::vector<std::tuple<float, float, float>> wps = {})
        : Drone(std::move(name), battery), mission_name(std::move(m_name)), waypoints(std::move(wps)) {}

    void add_waypoint(float x, float y, float z) {
        waypoints.push_back({x, y, z});
    }

    std::tuple<float, float, float> next_waypoint() {
        if (get_status() != "Flying") throw InvalidStateError();
        if (mission_complete()) throw InvalidStateError();

        drain_battery(1.5f);
        current_waypoint_index++;
        
        auto current_wp = waypoints[current_waypoint_index];
        visited_waypoints.push_back({current_wp, get_timestamp()});
        log_event("Navigated to Waypoint index: " + std::to_string(current_waypoint_index));
        
        return current_wp;
    }

    void skip_waypoint(const std::string& reason) {
        if (get_status() != "Flying") throw InvalidStateError();
        if (current_waypoint_index + 1 >= static_cast<int>(waypoints.size())) throw InvalidStateError();

        current_waypoint_index++; 
        auto skipped_wp = waypoints[current_waypoint_index];
        visited_waypoints.push_back({skipped_wp, "SKIPPED: " + reason});
        log_event("Waypoint index " + std::to_string(current_waypoint_index) + " SKIPPED. Reason: " + reason);
    }

    bool mission_complete() const {
        if (waypoints.empty()) return true;
        return current_waypoint_index >= static_cast<int>(waypoints.size()) - 1;
    }

    std::string mission_summary() const {
        std::stringstream ss;
        ss << mission_name << " ---\n";
        for (size_t i = 0; i < visited_waypoints.size(); ++i) {
            auto wp = visited_waypoints[i].first;
            ss << "  Track [" << i << "] Spatial Position: (" << std::get<0>(wp) << ", " 
               << std::get<1>(wp) << ", " << std::get<2>(wp) << ") - At: " << visited_waypoints[i].second << "\n";
        }
        return ss.str();
    }

    void get_info() const override {
        Drone::get_info();
        std::cout << "Mission Target: " << mission_name << " | Total Waypoints Map: " << waypoints.size()
                  << " | Current Target Index: " << current_waypoint_index << "\n";
    }
};

class AutonomousDrone : public MissionDrone {
private:
    std::vector<std::string> obstacle_log;

public:
    std::string ai_mode; 
    std::tuple<float, float, float> home_position;

    AutonomousDrone(std::string name, float battery, std::string m_name, std::tuple<float, float, float> home) 
        : MissionDrone(std::move(name), battery, std::move(m_name)), ai_mode("manual"), home_position(home) {}

    void set_ai_mode(const std::string& mode) {
        if (mode != "manual" && mode != "auto" && mode != "return_home") {
            throw InvalidStateError();
        }
        ai_mode = mode;
        log_event("AI Navigation Mode updated to: " + ai_mode);

        if (ai_mode == "return_home") {
            if (current_waypoint_index + 1 < static_cast<int>(waypoints.size())) {
                waypoints.insert(waypoints.begin() + current_waypoint_index + 1, home_position);
            } else {
                waypoints.push_back(home_position);
            }
            log_event("Injected Home coordinates into waypoint path.");
        }
    }

    void detect_obstacle(std::tuple<float, float, float> position, const std::string& severity) {
        if (get_status() != "Flying") throw InvalidStateError();
        
        std::string obs_entry = "Obs at (" + std::to_string(std::get<0>(position)) + "," + 
                                std::to_string(std::get<1>(position)) + ") Severity: " + severity;
        obstacle_log.push_back("[" + get_timestamp() + "] " + obs_entry);
        log_event("OBSTACLE DETECTED! " + obs_entry);

        if (severity == "high") {
            emergency_stop();
        }
    };

    std::vector<std::tuple<float, float, float>> auto_replan(const std::vector<std::tuple<float, float, float>>& obstacles) {
        if (get_status() != "Flying") throw InvalidStateError();
        return obstacles;
    };
};
int main() {
    MissionDrone missionDrone("Falcon", 85.0f, "Recon", {{0.0f, 0.0f, 10.0f}, {10.0f, 10.0f, 20.0f}});
    missionDrone.get_info();
    missionDrone.take_off(20.0f);
    auto waypoint = missionDrone.next_waypoint();
    std::cout << "Arrived at waypoint: " << std::get<0>(waypoint) << ", " << std::get<1>(waypoint) << ", " << std::get<2>(waypoint) << "
";
    missionDrone.land();
    std::cout << missionDrone.get_flight_log();
    return 0;
}
