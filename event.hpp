#ifndef __SURVEYBOX_EVENT_H
#define __SURVEYBOX_EVENT_H

#include <string>
#include <time.h>

// TODO: class or struct
struct Event
{
    const std::string type;
    const time_t timestamp;

    Event(const std::string& type, time_t timestamp);
};

#endif