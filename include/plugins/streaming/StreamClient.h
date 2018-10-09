#ifndef STREAM_CLIENT_H
#define STREAM_CLIENT_H

/*
 * Adapted from examples found at:
 * https://github.com/boostorg/asio/blob/develop/example/cpp11
 * Although we only need to write out data, reading functionality has been left in
 * this keeps the connection open.
 * The client will attempt to read continuously until it detects there is something
 * in the write_msgs_ queue to stream.
 */

#include <cstdlib>
#include <deque>
#include <iostream>
#include <thread>
#include <boost/asio.hpp>
#include <plugins/flatbuffers/data_sample_generated.h>
#include <plugins/util/SimConfig.h>


namespace ross_damaris {
namespace streaming {

namespace bip = boost::asio::ip;

/**
 * @brief Handles streaming flatbuffer data to external vis tools
 */
class StreamClient
{
public:
    StreamClient() :
        socket_(service_),
        resolver_(service_),
        connected_(false),
        t_(nullptr),
        sim_config_(nullptr) {  }

    StreamClient(StreamClient&& sc) :
        socket_(service_),
        resolver_(service_),
        connected_(sc.connected_),
        t_(sc.t_),
        sim_config_(std::move(sc.sim_config_)),
        write_msgs_(std::move(sc.write_msgs_)) {  }

    ~StreamClient()
    {
        if (t_)
        {
            t_->join();
            delete t_;
        }
    }

    /**
     * @brief Attempts to connect to specified address/port,
     * starting a new thread in the process.
     */
    void connect();

    /**
     * @brief get shared ownership to the SimConfig
     */
    void set_config_ptr(boost::shared_ptr<config::SimConfig>&& ptr);

    /**
     * @brief put in StreamClient's buffer of data to send
     *
     * Expects a DamarisDataSampleT object, which is packed into a flatbuffer.
     * StreamClient then takes ownership of the data and places it in a FIFO
     * queue to be sent.
     */
    void enqueue_data(sample::DamarisDataSampleT* samp);

    /**
     * @brief Close the streaming connection
     */
    void close();

private:
    struct sample_msg
    {
        uint8_t *raw_;  // points to start of fb dynamically allocated memory
        uint8_t *data_; // points to actual start of data
        flatbuffers::uoffset_t size_; // size of data portion of flatbuffer

        // we take ownership of the memory allocated by the flatbuffer
        // so we don't need to do any initialization of our own
        sample_msg() :
            size_(0),
            data_(nullptr),
            raw_(nullptr) {  }

        ~sample_msg()
        {
            // only need to delete the raw pointer
            // since data points to a location within raw
            if (raw_)
                delete[] raw_;
        }
    };

    typedef std::deque<sample_msg*> sample_queue;
    sample_queue write_msgs_;

    boost::shared_ptr<config::SimConfig> sim_config_;
    boost::asio::io_service service_;
    bip::tcp::resolver resolver_;
    bip::tcp::socket socket_;
    std::thread *t_;

    bool connected_;
    std::array<char, 1> dummy_buf_;

    void do_connect(bip::tcp::resolver::iterator it);
    void do_write();
    void do_read();

};

} // end namespace streaming
} // end namespace ross_damaris

#endif // STREAM_CLIENT_H
