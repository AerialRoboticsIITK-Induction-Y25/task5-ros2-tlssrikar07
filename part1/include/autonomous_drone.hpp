#ifndef AUTONOMOUS_DRONE_HPP
#define AUTONOMOUS_DRONE_HPP

#include "mission_drone.hpp"

class AutonomousDrone : public MissionDrone {
private:
    std::vector<std::string> obstacle_log;

public:
    std::string ai_mode; 
    std::tuple<float, float, float> home_position;

    AutonomousDrone(std::string name, float battery, std::string m_name, std::tuple<float, float, float> home);

    void set_ai_mode(const std::string& mode);
    void detect_obstacle(std::tuple<float,float,float> position, const std::string& severity);
    std::vector<std::tuple<float,float,float>> auto_replan(const std::vector<std::tuple<float,float,float>>& obstacles);
    void get_info() const override;
};

#endif
