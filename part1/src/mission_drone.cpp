#include "mission_drone.hpp"
#include "drone_exceptions.hpp"
#include <iostream>
#include <sstream>

MissionDrone::MissionDrone(std::string name, float battery, std::string m_name, std::vector<std::tuple<float, float, float>> wps) 
    : Drone(std::move(name), battery), mission_name(std::move(m_name)), waypoints(std::move(wps)) {}

void MissionDrone::add_waypoint(float x, float y, float z) {
    waypoints.push_back({x, y, z});
}

std::tuple<float, float, float> MissionDrone::next_waypoint() {
    if (get_status() != "Flying") throw InvalidStateError();
    if (mission_complete()) throw InvalidStateError();

    drain_battery(1.5f);
    current_waypoint_index++;
    
    auto current_wp = waypoints[current_waypoint_index];
    visited_waypoints.push_back({current_wp, get_timestamp()});
    log_event("Navigated to Waypoint index: " + std::to_string(current_waypoint_index));
    
    return current_wp;
}

void MissionDrone::skip_waypoint(const std::string& reason) {
    if (get_status() != "Flying") throw InvalidStateError();
    if (current_waypoint_index + 1 >= static_cast<int>(waypoints.size())) throw InvalidStateError();

    current_waypoint_index++; 
    auto skipped_wp = waypoints[current_waypoint_index];
    visited_waypoints.push_back({skipped_wp, "SKIPPED: " + reason});
    log_event("Waypoint index " + std::to_string(current_waypoint_index) + " SKIPPED. Reason: " + reason);
}

bool MissionDrone::mission_complete() const {
    if (waypoints.empty()) return true;
    return current_waypoint_index >= static_cast<int>(waypoints.size()) - 1;
}

std::string MissionDrone::mission_summary() const {
    std::stringstream ss;
    ss << "Mission: " << mission_name << " Summary Data:\n";
    for (size_t i = 0; i < visited_waypoints.size(); ++i) {
        auto wp = visited_waypoints[i].first;
        ss << "  [" << i << "] Position: (" << std::get<0>(wp) << ", " 
           << std::get<1>(wp) << ", " << std::get<2>(wp) << ") Status/Time: " << visited_waypoints[i].second << "\n";
    }
    return ss.str();
}

void MissionDrone::get_info() const {
    Drone::get_info();
    std::cout << "Mission Target: " << mission_name << " | Total Waypoints: " << waypoints.size()
              << " | Current Target Index: " << current_waypoint_index << "\n";
}
