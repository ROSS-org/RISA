#ifndef RD_SERVER_H
#define RD_SERVER_H

/**
 * @file RDServer.h
 *
 * This is the main representation of Damaris ranks.
 */

#include <ostream>
#include <iostream>

#include <boost/asio.hpp>
#include <damaris/env/Environment.hpp>
#include <damaris/buffer/DataSpace.hpp>
#include <damaris/buffer/Buffer.hpp>

#include <plugins/util/SimConfig.h>
#include <plugins/streaming/StreamClient.h>
#include <plugins/flatbuffers/data_sample_generated.h>
#include <plugins/data/ArgobotsManager.h>

namespace ross_damaris {
namespace server {

// TODO perhaps provide some static functions in case
// the StreamClient or DataProcessor need something?

/**
 * @brief Manages all of the functionality of the Damaris Ranks.
 */
class RDServer {
public:
    /**
     * @brief Sets up sim config, StreamClient, and DataProcessor
     */
    RDServer();

    /**
     * @brief Closes any open output files, and notifies the StreamClient to close.
     */
    void finalize();

    void update_gvt(int32_t step);

    void initial_data_tasks(int32_t step) { argobots_manager_->create_insert_data_mic_task(step); }

    void set_model_metadata();

private:
    std::shared_ptr<config::SimConfig> sim_config_;
    // TODO need to improve output to file
    // for now this is just for testing, but
    // eventually we'd like to save some data to file
    std::ofstream data_file_;

    // streaming-related members
    std::shared_ptr<streaming::StreamClient> client_;

    // data processing members
    //boost::shared_ptr<data::DataProcessor> processor_;
    std::shared_ptr<data::ArgobotsManager> argobots_manager_;

    sample::InstMode cur_mode_;
    double cur_ts_;
    double last_gvt_;
    std::list<int> my_pes_;
};

} // end namespace server
} // end namespace ross_damaris

#endif // end RD_SERVER_H
