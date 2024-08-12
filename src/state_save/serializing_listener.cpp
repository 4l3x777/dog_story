#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <fstream>
#include <filesystem>
#include <logger/logger.h>
#include <state_save/serializing_listener.h>

namespace serializing_listener {

using namespace std::literals::chrono_literals;


using InputArchive = boost::archive::text_iarchive;
using OutputArchive = boost::archive::text_oarchive;

SerializingListener::SerializingListener(
    std::shared_ptr<application::Application> application,
    const std::string& save_file, 
    milliseconds save_period) 
    : application_(std::move(application))
    , save_file_(save_file)
    , save_period_(save_period) 
{
    time_since_save_ = 0ms;
}

void SerializingListener::OnTick(milliseconds time_delta_ms) {
    time_since_save_ += time_delta_ms;
    if (time_since_save_ >= save_period_) {
        boost::json::object log_data;
        log_data["time since save"] = std::to_string(time_since_save_.count());
        log_data["save period"] = std::to_string(save_period_.count());
        LOG().print(
            log_data,
            "Periodic save game state"        
        );
        Save();
        time_since_save_ = 0ms;
    }
}

void SerializingListener::Load() {
    boost::json::object log_data;
    if (!std::filesystem::exists(save_file_)) {
        return;
    }
    try {
        std::ifstream archive_{save_file_};
        InputArchive input_archive{ archive_ };
        serialization::ApplicationRepr repr;
        input_archive >> repr;
        repr.Restore(*application_);
    }
    catch (const std::exception& ex) {
        log_data["error"] = ex.what();
        LOG().print(
            log_data,
            "Load save error"
        );
        throw std::runtime_error("error game saved state file corrupted");
    }
}

void SerializingListener::Save() {
    boost::json::object log_data;
    try {
        std::ofstream archive_{save_file_};
        OutputArchive output_archive{archive_};
        output_archive << serialization::ApplicationRepr{*application_};
    }
    catch (const std::exception& ex) {
        log_data["error"] = ex.what();
        LOG().print(
            log_data,
            "Error save game state"
        );
        throw std::runtime_error("error save game state");
    }
}

} // namespace infrastructure