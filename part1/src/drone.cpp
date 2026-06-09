#include "drone.hpp"
#include "drone_exceptions.hpp"
#include <iostream>

Drone::Drone(std::string name, float battery, float max_alt, float spd) 
    : Vehicle(std::move(name), battery), max_altitude(max_alt), speed(spd) {}

void Drone::get_info() const {
    std::cout << "Name: " << get_name() << " | Status: " << get_status() << "\n"
              << "Battery: " << get_battery_level() << "% | Altitude: " << altitude 
              << "m / " << max_altitude << "m | Speed: " << speed << " m/s\n";
}

void Drone::take_off(float target_altitude) {
    if (get_battery_level() <= 0.0f) throw BatteryDepletedError();
    if (get_status() != "Idle") throw InvalidStateError();
    if (target_altitude > max_altitude) throw AltitudeError();

    altitude = target_altitude;
    set_status("Flying");
    log_event("Successful takeoff to altitude: " + std::to_string(altitude) + "m");
}

void Drone::land() {
    if (get_status() != "Flying") throw InvalidStateError();
    log_event("Descending to land from " + std::to_string(altitude) + "m");
    altitude = 0.0f;
    set_status("Idle");
}

void Drone::emergency_stop() {
    log_event("EMERGENCY STOP!");
    altitude = 0.0f;
    set_status("Idle");
    drain_battery(30.0f); 
}
