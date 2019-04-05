#include <boost/asio/io_service.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <plugins/streaming/StreamClient.h>

using namespace boost::asio;
using namespace boost::asio::ip;

using namespace ross_damaris::streaming;
using namespace ross_damaris::config;
using namespace ross_damaris::sample;
using namespace std;

StreamClient* StreamClient::instance = nullptr;

StreamClient* StreamClient::create_instance()
{
    if (instance)
        cout << "StreamClient error!\n";
    instance = new StreamClient();
    return instance;
}

StreamClient* StreamClient::get_instance()
{
    if (!instance)
        cout << "StreamClient error!\n";
    return instance;
}

void StreamClient::free_instance()
{
    if (instance)
        delete instance;
}

void StreamClient::connect()
{
        cout << "[StreamClient " << process_id_ << "] Attempting to setup connection to " <<
            sim_config_->stream_addr() << ":" << sim_config_->stream_port() << endl;
        tcp::resolver::query q(sim_config_->stream_addr(), sim_config_->stream_port());
        auto it = resolver_.resolve(q);
        do_connect(it);
        t_ = new std::thread([this](){ this->service_.run(); });
}

void StreamClient::enqueue_data(DamarisDataSampleT* samp)
{
    flatbuffers::FlatBufferBuilder fbb;
    auto samp_fb = DamarisDataSample::Pack(fbb, samp);
    fbb.Finish(samp_fb);
    sample_msg *msg = new sample_msg();
    msg->size_ = static_cast<int>(fbb.GetSize());

    // Get the fb to release the raw pointer to us,
    // so it doesn't disappear before we actually send it
    // now we're responsible for deleting this memory
    size_t size, offset;
    msg->raw_ = fbb.ReleaseRaw(size, offset);
    msg->data_ = &msg->raw_[offset];
    write_msgs_.push_back(msg);
}

void StreamClient::enqueue_data(uint8_t *raw, uint8_t *data, int size)
{
    sample_msg *msg = new sample_msg();
    msg->size_ = size;
    msg->raw_ = raw;
    msg->data_ = data;
    write_msgs_.push_back(msg);
}

void StreamClient::close()
{
    if (!connected_)
        return;
    if (!write_msgs_.empty())
        cout << "[StreamClient " << process_id_ << "] closing socket, but there are still messages queued!\n";

    service_.post(
            [this]() {
                cout << "[StreamClient " << process_id_ << "] closing socket\n";
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
                    cout << ": StreamClient " << process_id_ << " do_connect() " << ec.message();
                    cout << ". Closing Socket\n";
                    connected_ = false;
                    socket_.close();
                }
                else
                {
                    cout << "[StreamClient " << process_id_ << "] successfully connected!\n" << endl;
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
    buf.push_back(boost::asio::buffer(&write_msgs_.front()->size_,
                sizeof(write_msgs_.front()->size_)));
    buf.push_back(boost::asio::buffer(write_msgs_.front()->data_, write_msgs_.front()->size_));
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
                    cout << ": StreamClient " << process_id_ << " do_write() " << ec.message();
                    cout << ". Closing Socket\n";
                    connected_ = false;
                    socket_.close();
                }
            });
}

// right now the do_read calls keeps the io_service from running out of work
// could potentially change to use asio::io_service::work
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
                    cout << ": StreamClient " << process_id_ << " do_read() " << ec.message();
                    cout << ". Closing Socket\n";
                    connected_ = false;
                    socket_.close();
                }
            });
}

