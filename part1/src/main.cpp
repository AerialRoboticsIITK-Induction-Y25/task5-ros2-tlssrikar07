#include "drone_exceptions.hpp"
#include "drone.hpp"
#include "mission_drone.hpp"
#include "autonomous_drone.hpp"
#include <iostream>
#include <vector>

int main() {
    std::vector<Vehicle*> fleet;

    Drone simple_drone("Scout-1", 85.0f);
    MissionDrone survey_drone("Survey-2", 90.0f, "Mapping_Grid");
    AutonomousDrone auto_drone("Alpha-AI", 95.0f, "Deep_Recon", {0.0f, 0.0f, 0.0f});

    fleet.push_back(&simple_drone);
    fleet.push_back(&survey_drone);
    fleet.push_back(&auto_drone);

    std::cout << "=== Showing Polymorphic get_info() ===\n";
    for (const auto* vehicle : fleet) {
        vehicle->get_info();
        std::cout << "-----------------------------------\n";
    }

    // Protection Verification Proof:
    // simple_drone.battery_level = 100.0f; // COMPILE ERROR: 'battery_level_' is private
    // simple_drone.status_ = "Flying";     // COMPILE ERROR: 'status_' is private

    std::cout << "\n=== Exception Handling Validation Routine ===\n";
    try {
        simple_drone.drain_battery(150.0f); 
    } catch (const DroneException& e) {    
        std::cout << "Successfully caught via DroneException Base: " << e.what() << "\n";
    }

    std::cout << "\n=== Complete Automated Autonomous Mission Test Cycle ===\n";
    try {
        auto_drone.add_waypoint(10.0f, 10.0f, 15.0f);
        auto_drone.add_waypoint(20.0f, 30.0f, 25.0f);
        
        auto_drone.take_off(20.0f);
        auto_drone.set_ai_mode("auto");

        while (!auto_drone.mission_complete()) {
            auto_drone.next_waypoint();
        }

        std::tuple<float, float, float> danger_obs{21.0f, 31.0f, 25.0f};
        auto_drone.detect_obstacle(danger_obs, "high");

    } catch (const DroneException& e) {
        std::cout << "Mission Event Caught: " << e.what() << "\n";
    }

    std::cout << "\n=== Flight Log Summary Report Output ===\n";
    std::cout << auto_drone.mission_summary();

    return 0;
}
