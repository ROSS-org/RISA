#ifndef STREAM_CLIENT_H
#define STREAM_CLIENT_H

/*
 * Adapted from example found at:
 * https://github.com/boostorg/asio/blob/develop/example/cpp11/nonblocking/third_party_lib.cpp
 * original has support for reading from and writing to a socket, but I think we only need writing
 * But in the future perhaps we want to support user input for simulation steering?
 */

#include <cstdlib>
#include <deque>
#include <iostream>
#include <thread>
#include <boost/asio.hpp>
#include "schemas/data_sample_generated.h"


namespace damaris_streaming {
using boost::asio::ip::tcp;
using namespace ross_damaris::sample;

// need to figure out the sized prefixed flatbuffer, to make this easier
struct sample_msg
{
    flatbuffers::uoffset_t size;
    uint8_t *buffer;

    sample_msg() :
        size(0),
        buffer(nullptr)
    {  }
};

class session
{
    private:
    tcp::socket socket_;

    public:


};

class StreamClient
{
    private:
    typedef std::deque<sample_msg> sample_queue;

    boost::asio::io_service& service_;
    tcp::socket socket_;
    sample_queue write_msgs_;
	
    void do_connect(tcp::resolver::iterator it);
    void do_write();

    public:
    StreamClient(boost::asio::io_service& service, tcp::resolver::iterator it)
        : service_(service),
          socket_(service)
    {
        do_connect(it);
    }

    void write(const flatbuffers::FlatBufferBuilder& data);
    void close();
};

} // end namespace damaris_streaming

#endif // STREAM_CLIENT_H
