#ifndef VEHICLE_HPP
#define VEHICLE_HPP

#include <string>
#include <vector>

class Vehicle {
private:
    std::string name_;
    float battery_level_; 
    std::string status_;   
    std::vector<std::string> flight_log_;

protected:
    std::string get_timestamp() const;
    void set_status(const std::string& new_status);
    void log_event(const std::string& event);

public:
    Vehicle(std::string v_name, float initial_bat);
    virtual ~Vehicle() = default;

    virtual void get_info() const = 0; 

    std::string get_name() const { return name_; }
    float get_battery_level() const { return battery_level_; }
    std::string get_status() const { return status_; }
    std::string get_flight_log() const;

    void drain_battery(float amount);
    void charge_battery(float amount, int duration_seconds);
    bool is_critical() const;
};

#endif
