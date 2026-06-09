#ifndef DRONE_EXCEPTIONS_HPP
#define DRONE_EXCEPTIONS_HPP

#include <exception>
#include <string>
#include <utility>

class DroneException : public std::exception {
protected:
    std::string message_;
public:
    explicit DroneException(std::string msg) : message_(std::move(msg)) {}
    const char* what() const noexcept override { return message_.c_str(); }
};

class BatteryDepletedError : public DroneException {
public:
    BatteryDepletedError() : DroneException("BatteryDepletedError: Battery level is at 0%.") {}
};

class InvalidStateError : public DroneException {
public:
    InvalidStateError() : DroneException("InvalidStateError: Operation invalid for current state.") {}
};

class AltitudeError : public DroneException {
public:
    AltitudeError() : DroneException("AltitudeError: Target exceeds maximum allowable altitude.") {}
};

#endif
