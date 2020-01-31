#include <string>
#include <HTTPClient.h>

// TODO: Use event or raw information.
#include "event.hpp"

// TODO: Use templates?
class FirebaseClient
{
    private:
        // TODO: Make readonly.
        HTTPClient _client;
        std::string _url;
        std::string _deviceid;

        std::string _payload_creation(const Event& event) const;

    public:
        // TODO: Create interface for this, or is that too heavy now? Would allow easier porting to different devices eventually?
        // Can create base and then esp32 variant of this code.
        FirebaseClient(const std::string& endpoint, const std::string& path, const std::string& deviceid);

        FirebaseClient& operator= (const FirebaseClient& client);

        bool add(const Event& event);
};
