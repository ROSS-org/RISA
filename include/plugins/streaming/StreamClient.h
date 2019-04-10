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
#include <damaris/env/Environment.hpp>


namespace ross_damaris {
namespace streaming {

namespace bip = boost::asio::ip;

/**
 * @brief Handles streaming flatbuffer data to external vis tools
 */
class StreamClient
{
public:
    static StreamClient* get_instance();

    /**
     * @brief Attempts to connect to specified address/port,
     * starting a new thread in the process.
     */
    void connect();

    /**
     * @brief put in StreamClient's buffer of data to send
     *
     * Expects a DamarisDataSampleT object, which is packed into a flatbuffer.
     * StreamClient then takes ownership of the data and places it in a FIFO
     * queue to be sent.
     */
    void enqueue_data(sample::DamarisDataSampleT* samp);
    void enqueue_data(uint8_t *raw, uint8_t *data, size_t size);

    /**
     * @brief Close the streaming connection
     */
    void close();

private:

    StreamClient() :
        resolver_(service_),
        socket_(service_),
        t_(nullptr),
        connected_(false),
        sim_config_(config::SimConfig::get_instance()) {
            process_id_ = damaris::Environment::GetEntityProcessID();
        }

    StreamClient(const StreamClient&) = delete;
    StreamClient& operator=(const StreamClient&) = delete;

    ~StreamClient()
    {
        if (t_ && t_->joinable())
        {
            t_->join();
            delete t_;
        }
    }

    struct sample_msg
    {
        uint8_t *raw_;  // points to start of fb dynamically allocated memory
        uint8_t *data_; // points to actual start of data
        int size_; // size of data portion of flatbuffer

        // we take ownership of the memory allocated by the flatbuffer
        // so we don't need to do any initialization of our own
        sample_msg() :
            raw_(nullptr),
            data_(nullptr),
            size_(0) {  }

        ~sample_msg()
        {
            // only need to delete the raw pointer
            // since data points to a location within raw
            if (raw_)
                delete[] raw_;
        }
    };

    int process_id_;
    typedef std::deque<sample_msg*> sample_queue;
    sample_queue write_msgs_;

    boost::asio::io_service service_;
    bip::tcp::resolver resolver_;
    bip::tcp::socket socket_;
    std::thread *t_;

    bool connected_;
    std::array<char, 1> dummy_buf_;
    config::SimConfig* sim_config_;

    void do_connect(bip::tcp::resolver::iterator it);
    void do_write();
    void do_read();

};

} // end namespace streaming
} // end namespace ross_damaris

#endif // STREAM_CLIENT_H
