#include "event.hpp"

Event::Event(const std::string& type, time_t timestamp) : type(type), timestamp(timestamp) { }