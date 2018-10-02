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
#include <plugins/data/DataSample.h>
#include <plugins/data/SampleIndex.h>
#include <plugins/streaming/stream-client.h>
#include <plugins/flatbuffers/data_sample_generated.h>

namespace ross_damaris {
namespace server {

class RDServer {
public:
    RDServer() :
        resolver_(service_),
        client_(nullptr),
        t_(nullptr) {  }

    void setup_simulation_config();
    void finalize();

    int num_pe() { return sim_config_.num_pe; }
    int num_kp() { return sim_config_.kp_per_pe; }

    void process_sample(boost::shared_ptr<damaris::Block> block);

private:
    SimConfig sim_config_;
    // TODO need to improve output to file
    // for now this is just for testing, but
    // eventually we'd like to save some data to file
    std::ofstream data_file_;

    // streaming-related members
    boost::asio::io_service service_;
    boost::asio::ip::tcp::resolver resolver_;
    streaming::StreamClient *client_;
    std::thread *t_; // thread is to handle boost async IO operations

    // storage for data to be processed
    data::SampleIndex data_index_;

    void setup_data_processing();
    void setup_streaming();
};

} // end namespace server
} // end namespace ross_damaris

#endif // end RD_SERVER_H
