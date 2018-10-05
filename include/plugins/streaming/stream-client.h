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


namespace ross_damaris {
namespace streaming {

using boost::asio::ip::tcp;
using namespace ross_damaris::sample;

// need to figure out the sized prefixed flatbuffer, to make this easier
struct sample_msg
{
    flatbuffers::uoffset_t size;
    // when fb releases the pointer, raw may not be the actual start
    // of the data, since it fills starting from the end
    uint8_t *buffer; // points to actual start of data
    uint8_t *raw; // points to start of fb dynamically allocated memory

    sample_msg() :
        size(0),
        buffer(nullptr),
        raw(nullptr)
    {  }

    ~sample_msg()
    {
        // only need to delete the raw pointer, not buffer
        if (raw)
            delete[] raw;
    }
};

class StreamClient
{
    private:
    typedef std::deque<sample_msg*> sample_queue;

    boost::asio::io_service& service_;
    tcp::socket socket_;
    bool connected_;
    sample_queue write_msgs_;
    std::array<char, 1> dummy_buf_;

    void do_connect(tcp::resolver::iterator it);
    void do_write();
    void do_read();

    public:
    StreamClient(boost::asio::io_service& service, tcp::resolver::iterator it)
        : service_(service),
          socket_(service),
          connected_(false)
    {
        do_connect(it);
    }

    /**
     * @brief Gets the data to the streaming client
     *
     * This may not actually write data immediately to the socket.
     * Data is placed in a FIFO queue
     */
    void write(flatbuffers::FlatBufferBuilder* data);
    void write(DamarisDataSampleT* samp);

    /**
     * @brief Close the streaming connection
     */
    void close();
};

} // end namespace streaming
} // end namespace ross_damaris

#endif // STREAM_CLIENT_H
