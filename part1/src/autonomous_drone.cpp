#include "autonomous_drone.hpp"
#include "drone_exceptions.hpp"
#include <iostream>
#include <cmath>

AutonomousDrone::AutonomousDrone(std::string name, float battery, std::string m_name, std::tuple<float, float, float> home) 
    : MissionDrone(std::move(name), battery, std::move(m_name)), ai_mode("manual"), home_position(home) {}

void AutonomousDrone::set_ai_mode(const std::string& mode) {
    if (mode != "manual" && mode != "auto" && mode != "return_home") throw InvalidStateError();
    ai_mode = mode;
    log_event("AI Navigation Mode updated to: " + ai_mode);

    if (ai_mode == "return_home") {
        if (current_waypoint_index + 1 < static_cast<int>(waypoints.size())) {
            waypoints.insert(waypoints.begin() + current_waypoint_index + 1, home_position);
        } else {
            waypoints.push_back(home_position);
        }
        log_event("Injected Home coordinates into waypoint path matrix.");
    }
}

void AutonomousDrone::detect_obstacle(std::tuple<float,float,float> position, const std::string& severity) {
    std::string obstacle_str = "Obstacle detected at (" + std::to_string(std::get<0>(position)) + ", " + 
                               std::to_string(std::get<1>(position)) + ") Severity: " + severity;
    obstacle_log.push_back("[" + get_timestamp() + "] " + obstacle_str);
    log_event(obstacle_str);

    if (severity == "high") {
        emergency_stop();
    }
}

std::vector<std::tuple<float,float,float>> AutonomousDrone::auto_replan(const std::vector<std::tuple<float,float,float>>& obstacles) {
    std::vector<std::tuple<float,float,float>> filtered_wps;
    for (const auto& wp : waypoints) {
        bool unsafe = false;
        for (const auto& obs : obstacles) {
            float dist = std::sqrt(std::pow(std::get<0>(wp) - std::get<0>(obs), 2) +
                                   std::pow(std::get<1>(wp) - std::get<1>(obs), 2));
            if (dist < 5.0f) {
                unsafe = true;
                break;
            }
        }
        if (!unsafe) filtered_wps.push_back(wp);
    }
    log_event("Auto Replan completed. Valid points kept.");
    return filtered_wps;
}

void AutonomousDrone::get_info() const {
    MissionDrone::get_info();
    std::cout << "AI Mode: " << ai_mode << " | Obstacles Encountered: " << obstacle_log.size() << "\n";
}
