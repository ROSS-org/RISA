#ifndef RD_SERVER_H
#define RD_SERVER_H

/**
 * @file RDServer.h
 *
 * This is the main representation of Damaris ranks.
 */

#include <ostream>

#include <boost/asio.hpp>
#include <damaris/env/Environment.hpp>
#include <damaris/buffer/DataSpace.hpp>
#include <damaris/buffer/Buffer.hpp>

#include <plugins/util/SimConfig.h>
#include <plugins/data/DataManager.h>
#include <plugins/data/DataSample.h>
#include <plugins/data/DataProcessor.h>
#include <plugins/data/SampleIndex.h>
#include <plugins/streaming/StreamClient.h>
#include <plugins/flatbuffers/data_sample_generated.h>

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

    /**
     * @brief Initial data processing to place into DataManager
     *
     * Takes a Damaris Block, creates a DataSpace, so we essentially have
     * a shared_ptr to this data, preventing Damaris from deleting it, while we own it.
     */
    void process_sample(boost::shared_ptr<damaris::Block> block);

    /**
     * @brief Lets the DataProcessor know to just forward data for a particular
     * mode and timestamp.
     *
     * May be removed once DataProcessor functionality is more built up.
     */
    void forward_data();

    boost::shared_ptr<data::DataManager> get_manager_pointer() { return data_manager_; }

private:
    boost::shared_ptr<config::SimConfig> sim_config_;
    // TODO need to improve output to file
    // for now this is just for testing, but
    // eventually we'd like to save some data to file
    std::ofstream data_file_;

    // streaming-related members
    boost::shared_ptr<streaming::StreamClient> client_;

    // data processing members
    boost::shared_ptr<data::DataProcessor> processor_;

    // storage for data to be processed
    boost::shared_ptr<data::DataManager> data_manager_;
    sample::InstMode cur_mode_;
    double cur_ts_;

    void setup_data_processing();
};

} // end namespace server
} // end namespace ross_damaris

#endif // end RD_SERVER_H
