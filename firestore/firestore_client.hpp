// Client creation and authentication goes here...

#ifndef __MS_FIRESTORE_CLIENT_HPP
#define __MS_FIRESTORE_CLIENT_HPP

#include <string>

class firestore_client
{
    public:
        // TODO: have client manage the jwt management.
        firestore_client(std::string jwt);
};

#endif
