#include "vehicle.hpp"
#include "drone_exceptions.hpp"
#include <chrono>
#include <sstream>
#include <iomanip>

Vehicle::Vehicle(std::string v_name, float initial_bat) 
    : name_(std::move(v_name)), battery_level_(initial_bat), status_("Idle") {
    if (battery_level_ > 100.0f) battery_level_ = 100.0f;
    if (battery_level_ < 0.0f) battery_level_ = 0.0f;
}

std::string Vehicle::get_timestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    struct tm time_info;
#if defined(_MSC_VER)
    gmtime_s(&time_info, &time_t_now);
#else
    auto* tm_ptr = std::gmtime(&time_t_now);
    if (tm_ptr) time_info = *tm_ptr;
#endif
    ss << std::put_time(&time_info, "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

void Vehicle::set_status(const std::string& new_status) {
    if (new_status != "Idle" && new_status != "Flying" && new_status != "Charging") {
        throw InvalidStateError();
    }
    status_ = new_status;
    log_event("Status changed to: " + status_);
}

void Vehicle::log_event(const std::string& event) {
    flight_log_.push_back("[" + get_timestamp() + "] " + event);
}

std::string Vehicle::get_flight_log() const {
    std::stringstream ss;
    for (const auto& log : flight_log_) {
        ss << log << "\n";
    }
    return ss.str();
}

void Vehicle::drain_battery(float amount) {
    if (battery_level_ <= 0.0f) throw BatteryDepletedError();
    battery_level_ -= amount;
    if (battery_level_ <= 0.0f) {
        battery_level_ = 0.0f;
        log_event("Battery fully depleted.");
        throw BatteryDepletedError();
    }
}

void Vehicle::charge_battery(float amount, int duration_seconds) {
    if (status_ != "Charging") throw InvalidStateError();
    battery_level_ += amount * (duration_seconds / 10.0f);
    if (battery_level_ > 100.0f) battery_level_ = 100.0f;
    log_event("Charged battery level up to " + std::to_string(battery_level_) + "%");
}

bool Vehicle::is_critical() const {
    return battery_level_ <= 30.0f;
}
