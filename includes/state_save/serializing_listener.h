#pragma once

#include <chrono>
#include <memory>
#include <save_application.h>

namespace serializing_listener {

using milliseconds = std::chrono::milliseconds;

class SerializingListener {
public:
    SerializingListener(
        std::shared_ptr<application::Application> application, 
        const std::string& save_file, 
        milliseconds save_period
    );

    void OnTick(milliseconds time_delta_ms);
    void Save();
    void Load();

private:
    std::shared_ptr<application::Application> application_;
    const std::string save_file_;
    milliseconds save_period_;
    milliseconds time_since_save_;
};

} // namespace infrastructure