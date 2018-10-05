#ifndef RD_SERVER_H
#define RD_SERVER_H

/**
 * @file server.h
 *
 * This is the main representation of Damaris ranks.
 */

#include <ostream>

#include <boost/asio.hpp>
#include <damaris/env/Environment.hpp>
#include <damaris/buffer/DataSpace.hpp>
#include <damaris/buffer/Buffer.hpp>

#include <plugins/util/sim-config.h>
#include <plugins/data/DataManager.h>
#include <plugins/data/DataSample.h>
#include <plugins/data/DataProcessor.h>
#include <plugins/data/SampleIndex.h>
#include <plugins/streaming/stream-client.h>
#include <plugins/flatbuffers/data_sample_generated.h>

namespace ross_damaris {
namespace server {

// TODO perhaps provide some static functions in case
// the StreamClient or DataProcessor need something?

class RDServer {
public:
    RDServer() :
        resolver_(service_),
        t_(nullptr),
        cur_mode_(sample::InstMode_GVT),
        cur_ts_(0.0),
        data_manager_(boost::make_shared<data::DataManager>(data::DataManager())){  }

    void setup_simulation_config();
    void finalize();

    int num_pe() { return sim_config_.num_pe; }
    int num_kp() { return sim_config_.kp_per_pe; }

    void process_sample(boost::shared_ptr<damaris::Block> block);
    void forward_data();

    boost::shared_ptr<data::DataManager> get_manager_pointer() { return data_manager_; }

private:
    SimConfig sim_config_;
    // TODO need to improve output to file
    // for now this is just for testing, but
    // eventually we'd like to save some data to file
    std::ofstream data_file_;

    // streaming-related members
    boost::asio::io_service service_;
    boost::asio::ip::tcp::resolver resolver_;
    boost::shared_ptr<streaming::StreamClient> client_;
    std::thread *t_; // thread is to handle boost async IO operations

    // data processing members
    data::DataProcessor processor_;

    // storage for data to be processed
    //data::SampleIndex data_index_;
    boost::shared_ptr<data::DataManager> data_manager_;
    sample::InstMode cur_mode_;
    double cur_ts_;

    void setup_data_processing();
    void setup_streaming();
};

} // end namespace server
} // end namespace ross_damaris

#endif // end RD_SERVER_H
