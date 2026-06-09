#ifndef MISSION_DRONE_HPP
#define MISSION_DRONE_HPP

#include "drone.hpp"
#include <tuple>
#include <vector>
#include <utility>

class MissionDrone : public Drone {
private:
    std::vector<std::pair<std::tuple<float, float, float>, std::string>> visited_waypoints;

protected:
    std::string mission_name;
    std::vector<std::tuple<float, float, float>> waypoints;
    int current_waypoint_index{-1};

public:
    MissionDrone(std::string name, float battery, std::string m_name, std::vector<std::tuple<float, float, float>> wps = {});
    
    void add_waypoint(float x, float y, float z);
    std::tuple<float, float, float> next_waypoint();
    void skip_waypoint(const std::string& reason);
    bool mission_complete() const;
    std::string mission_summary() const;
    void get_info() const override;

    int get_current_waypoint_index() const { return current_waypoint_index; }
    size_t get_waypoint_count() const { return waypoints.size(); }
};

#endif
