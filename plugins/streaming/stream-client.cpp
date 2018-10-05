#include <boost/asio/io_service.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <plugins/streaming/stream-client.h>

using namespace boost::asio;
using namespace boost::asio::ip;

using namespace ross_damaris::streaming;
using namespace std;

void StreamClient::write(DamarisDataSampleT* samp)
{
    flatbuffers::FlatBufferBuilder fbb_;
    auto samp_fb = DamarisDataSample::Pack(fbb_, samp);
    fbb_.Finish(samp_fb);
    sample_msg *msg = new sample_msg();
    msg->size = fbb_.GetSize();

    // Get the fb to release the raw pointer to us,
    // so it doesn't disappear before we actually send it
    // now we're responsible for deleting this memory
    size_t size, offset;
    msg->raw = fbb_.ReleaseRaw(size, offset);
    msg->buffer = &msg->raw[offset];
    write_msgs_.push_back(msg);
}

void StreamClient::write(flatbuffers::FlatBufferBuilder* data)
{
    sample_msg *msg = new sample_msg();
    msg->size = data->GetSize();
    msg->buffer = data->GetBufferPointer();
    //msg->builder = data;
    write_msgs_.push_back(msg);
}

void StreamClient::close()
{
    if (!connected_)
        return;
    if (!write_msgs_.empty())
        cout << "StreamClient closing socket, but there are still messages waiting to be streamed!\n";

    service_.post(
            [this]() {
                cout << "StreamClient closing socket\n";
                connected_ = false;
                socket_.close();
            });
}

void StreamClient::do_connect(tcp::resolver::iterator it)
{
    boost::asio::async_connect(socket_, it,
            [this](boost::system::error_code ec, tcp::resolver::iterator /*it*/)
            {
                if (ec)
                {
                    cout << ec.category().name() << " error " << ec.value();
                    cout << ": StreamClient do_connect() " << ec.message();
                    cout << ". Closing Socket\n";
                    connected_ = false;
                    socket_.close();
                }
                else
                {
                    cout << "StreamClient successfully connected!\n" << endl;
                    connected_ = true;
                    do_read();
                }
            });
}

void StreamClient::do_write()
{
    if (write_msgs_.empty() || !connected_)
        return;
    std::vector<boost::asio::const_buffer> buf;
    buf.push_back(boost::asio::buffer(&write_msgs_.front()->size, sizeof(flatbuffers::uoffset_t)));
    buf.push_back(boost::asio::buffer(write_msgs_.front()->buffer, write_msgs_.front()->size));
    boost::asio::async_write(socket_, buf,
            [this](boost::system::error_code ec, std::size_t /*length*/)
            {
                if (!ec)
                {
                    sample_msg *msg = write_msgs_.front();
                    delete msg;
                    write_msgs_.pop_front();
                    if (!write_msgs_.empty())
                        do_write();
                    else
                        do_read();
                }
                else
                {
                    cout << ec.category().name() << " error " << ec.value();
                    cout << ": StreamClient do_write() " << ec.message();
                    cout << ". Closing Socket\n";
                    connected_ = false;
                    socket_.close();
                }
            });
}

void StreamClient::do_read()
{
    if (!connected_)
        return;
    boost::asio::async_read(socket_,
            boost::asio::buffer(dummy_buf_, 0),
            [this](boost::system::error_code ec, std::size_t /*len*/)
            {
                if (!ec)
                {
                    if (!write_msgs_.empty())
                        do_write();
                    else
                        do_read();
                }
                else
                {
                    cout << ec.category().name() << " error " << ec.value();
                    cout << ": StreamClient do_read() " << ec.message();
                    cout << ". Closing Socket\n";
                    connected_ = false;
                    socket_.close();
                }
            });
}

