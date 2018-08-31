#include <boost/asio/io_service.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/ip/tcp.hpp>
#include "stream-client.h"

using namespace boost::asio;
using namespace boost::asio::ip;

using namespace damaris_streaming;
using namespace std;

sample_msg msg;
void StreamClient::write(const flatbuffers::FlatBufferBuilder& data)
{
    msg.size = data.GetSize();
    msg.buffer = data.GetBufferPointer();
    cout << "StreamClient write() msg size = " << msg.size << endl;
    //bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(msg);
    //if (!write_in_progress)
    //    do_write();

    //service_.post([this](){
    //        cout << "StreamClient write() lambda" << endl;
    //        bool write_in_progress = !write_msgs_.empty();
    //        write_msgs_.push_back(msg);
    //        cout << "StreamClient write() lambda " << write_in_progress << " " << write_msgs_.empty() << endl;
    //        if (!write_in_progress)
    //            do_write();
    //    });
}

void StreamClient::close()
{
    cout << "Stream client closing socket" << endl;
    service_.post([this]() { cout << "closing socket\n"; socket_.close(); });
}

void StreamClient::do_connect(tcp::resolver::iterator it)
{
    cout << "stream client connecting..." << endl;
    boost::asio::async_connect(socket_, it,
            [this](boost::system::error_code ec, tcp::resolver::iterator it)
            {
                if (ec)
                {
                    socket_.close();
                    cout << "Error do_connect() closing socket" << endl;
                }
                else
                {
                    cout << "Stream client successfully connected" << endl;
                    do_read();
                }
            });
}

void StreamClient::do_write()
{
    cout << "StreamClient do_write()\n";
    if (write_msgs_.empty())
    {
        return;
    }
    std::vector<const_buffer> buf;
    buf.push_back(boost::asio::buffer(&write_msgs_.front().size, sizeof(flatbuffers::uoffset_t)));
    buf.push_back(boost::asio::buffer(write_msgs_.front().buffer, write_msgs_.front().size));
    boost::asio::async_write(socket_, buf,
            [this](boost::system::error_code ec, std::size_t /*length*/)
            {
                cout << "StreamClient do_write() lambda\n";
                if (!ec)
                {
                    cout << "StreamClient writing flatbuffer to socket" << endl;
                    write_msgs_.pop_front();
                    if (!write_msgs_.empty())
                    {
                        cout << "StreamClient initiating another write" << endl;
                        do_write();
                    }
                    else
                        do_read();
                }
                else
                {
                    socket_.close();
                    cout << "Error do_write() closing socket" << endl;
                }
            });
}

void StreamClient::do_read()
{
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
                    socket_.close();
                }
            });
}

//int main()
//{
//    boost::asio::io_service service;
//    tcp::resolver resolver(service);
//    tcp::resolver::query q("", "8000");
//    auto it = resolver.resolve(q);
//    StreamClient client(service, it);
//}
