/**
 * @file server.cpp
 *
 * This is the main representation of Damaris ranks.
 * This rank will get split into 3 threads: 
 *    1) handle events called by Damaris (and any MPI calls needed)
 *    2) handle data processing tasks (eg garbage collection, aggregation, etc)
 *       (DataProcessor class takes over here)
 *    3) handle streaming functionality (StreamClient class takes over here)
 */

#include <plugins/server/server.h>
#include <plugins/util/damaris-util.h>

using namespace ross_damaris::server;
using namespace ross_damaris::util;
namespace bip = boost::asio::ip;

void RDServer::setup_simulation_config()
{
    sim_config_.total_pe = damaris::Environment::CountTotalClients();
    // TODO ClientsPerNode is number of ROSS PEs per Damaris server, I think
    // need to double check
    sim_config_.num_pe = damaris::Environment::ClientsPerNode();

    for (int i = 0; i < sim_config_.num_pe; i++)
    {
        auto val = *(static_cast<int*>(DUtil::get_value_from_damaris("ross/nlp", i, 0, 0)));
        sim_config_.num_lp.push_back(val);
    }
    sim_config_.kp_per_pe = *(static_cast<int*>(DUtil::get_value_from_damaris("ross/nkp", 1, 0, 0)));

    auto opts = set_options();
    po::variables_map vars;
    string config_file = &((char *)DUtil::get_value_from_damaris("ross/inst_config", 0, 0, 0))[0];
    ifstream ifs(config_file.c_str());
    parse_file(ifs, opts, vars);
    sim_config_.set_parameters(vars);
    sim_config_.print_parameters();

    if (sim_config_.write_data)
        data_file_.open("test-fb.bin", ios::out | ios::trunc | ios::binary);

    if (sim_config_.stream_data)
        setup_streaming();
}

void RDServer::setup_streaming()
{
    // sets up thread that performs streaming functionality
    cout << "Attempting to setup connection to " << sim_config_.stream_addr << ":" << sim_config_.stream_port << endl;
    bip::tcp::resolver::query q(sim_config_.stream_addr, sim_config_.stream_port);
    auto it = resolver_.resolve(q);
    client_ = new ross_damaris::streaming::StreamClient(service_, it);
    t_ = new std::thread([this](){ this->service_.run(); });
}

void RDServer::setup_data_processing()
{
    // sets up thread that performs data processing tasks
}

void RDServer::finalize()
{
    if (sim_config_.write_data)
        data_file_.close();

    if (sim_config_.stream_data)
    {
        client_->close();
        delete client_;
        t_->join();
        delete t_;
    }
}
