/**
 * @file RDServer.cpp
 *
 * This is the main representation of Damaris ranks.
 * This rank will get split into 3 threads:
 *    1) handle events called by Damaris (and any MPI calls needed)
 *    2) handle data processing tasks (eg garbage collection, aggregation, etc)
 *       (DataProcessor class takes over here)
 *    3) handle streaming functionality (StreamClient class takes over here)
 */

#include <plugins/server/RDServer.h>
#include <plugins/util/damaris-util.h>
#include <damaris/data/VariableManager.hpp>

using namespace ross_damaris::server;
using namespace ross_damaris::util;
using namespace ross_damaris::sample;
using namespace ross_damaris::data;
using namespace ross_damaris::config;
using namespace std;
using namespace damaris;
namespace bip = boost::asio::ip;

RDServer* RDServer::instance = nullptr;

RDServer* RDServer::get_instance()
{
    if (!instance)
        instance = new RDServer();
    return instance;
}

RDServer::RDServer() :
        cur_mode_(sample::InstMode_GVT),
        cur_ts_(0.0),
        last_gvt_(0.0),
        stream_client_(nullptr),
        sim_config_(nullptr),
        argobots_manager_(nullptr)
{
    int my_id = damaris::Environment::GetEntityProcessID();
    int num_local_pe = damaris::Environment::ClientsPerNode() /
            damaris::Environment::ServersPerNode();
    int total_pe = num_local_pe * damaris::Environment::CountTotalServers();

    // TODO should this be for this Server's PEs or all PEs?
    my_pes_ = damaris::Environment::GetKnownLocalClients();
    vector<int> num_lp;
    int num_kp_pe = *(static_cast<int*>(DUtil::get_value_from_damaris(
                    "ross/nkp", my_pes_.front(), 0, 0)));
    for (int pe : my_pes_)
    {
        //cout << "[" << my_id << "] pe: " << pe << endl;
        auto val = *(static_cast<int*>(DUtil::get_value_from_damaris("ross/nlp", pe, 0, 0)));
        num_lp.push_back(val);
    }

    sim_config_ = SimConfig::create_instance(num_local_pe, total_pe, num_kp_pe, std::move(num_lp));
    auto opts = SimConfig::set_options();
    po::variables_map vars;
    string config_file = &((char *)DUtil::get_value_from_damaris(
                "ross/inst_config", my_pes_.front(), 0, 0))[0];
    ifstream ifs(config_file.c_str());
    SimConfig::parse_file(ifs, opts, vars);
    sim_config_->set_parameters(vars);
    //sim_config_->print_parameters();

    if (sim_config_->write_data_)
        data_file_.open("test-fb.bin", ios::out | ios::trunc | ios::binary);

    if (sim_config_->stream_data_)
    {
        stream_client_ = streaming::StreamClient::create_instance();
        stream_client_->connect();
    }

    argobots_manager_ = ArgobotsManager::create_instance();
}


void RDServer::set_model_metadata()
{
    sim_config_->set_model_metadata();
}

void RDServer::finalize()
{
    argobots_manager_->finalize();

    if (sim_config_->write_data_)
        data_file_.close();

    if (sim_config_->stream_data_)
    {
        stream_client_->close();
        stream_client_->free_instance();
    }

    sim_config_->free_instance();

    if (instance)
        delete instance;
}

void RDServer::update_gvt(int32_t step)
{
    last_gvt_ = *(static_cast<double*>(DUtil::get_value_from_damaris("ross/last_gvt",
                    my_pes_.front(), step, 0)));
    //cout << "[RDServer] last GVT " << last_gvt_ << endl;
}

