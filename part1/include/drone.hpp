#ifndef DRONE_HPP
#define DRONE_HPP

#include "vehicle.hpp"

class Drone : public Vehicle {
protected:
    float altitude{0.0f};
    float max_altitude{120.0f};
private:
    float speed{5.0f};

public:
    Drone(std::string name, float battery, float max_alt = 150.0f, float spd = 5.0f);
    void get_info() const override;

    void take_off(float target_altitude);
    void land();
    void emergency_stop();

    float get_altitude() const { return altitude; }
    float get_max_altitude() const { return max_altitude; }
    float get_speed() const { return speed; }
};

#endif
