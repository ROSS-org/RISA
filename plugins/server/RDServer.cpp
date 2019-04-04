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

RDServer::RDServer() :
        cur_mode_(sample::InstMode_GVT),
        cur_ts_(0.0),
        last_gvt_(0.0),
        client_(std::make_shared<streaming::StreamClient>(streaming::StreamClient())),
        sim_config_(std::make_shared<config::SimConfig>(config::SimConfig())),
        argobots_manager_(std::make_shared<data::ArgobotsManager>(data::ArgobotsManager()))
{
    int my_id = damaris::Environment::GetEntityProcessID();
    //cout << "IsClient() " << damaris::Environment::IsClient() << endl;
    //cout << "IsServer() " << damaris::Environment::IsServer() << endl;
    //cout << "IsDedicatedCore() " << damaris::Environment::IsDedicatedCore() << endl;
    //cout << "IsDedicatedNode() " << damaris::Environment::IsDedicatedNode() << endl;
    //cout << "[" << my_id << "] ClientsPerNode() " << damaris::Environment::ClientsPerNode() << endl;
    //cout << "[" << my_id << "] CoresPerNode() " << damaris::Environment::CoresPerNode() << endl;
    //cout << "[" << my_id << "] ServersPerNode() " << damaris::Environment::ServersPerNode() << endl;
    //cout << "[" << my_id << "] CountTotalClients() " << damaris::Environment::CountTotalClients() << endl;
    //cout << "[" << my_id << "] CountTotalServers() " << damaris::Environment::CountTotalServers() << endl;
    // I think CountTotalClients() is computed incorrectly in Damaris if you have
    // more than one Damaris Rank per node
    // CountTotalServers() seems to only be Damaris Ranks
    sim_config_->num_local_pe_ = damaris::Environment::ClientsPerNode() /
            damaris::Environment::ServersPerNode();
    //cout << "My PEs: " << sim_config_->num_local_pe_ << endl;
    sim_config_->total_pe_ = sim_config_->num_local_pe_ *
            damaris::Environment::CountTotalServers();
    //cout << "Total PEs: " << sim_config_->total_pe_ << endl;

    // TODO should this be for this Server's PEs or all PEs?
    my_pes_ = damaris::Environment::GetKnownLocalClients();
    for (int pe : my_pes_)
    {
        //cout << "[" << my_id << "] pe: " << pe << endl;
        auto val = *(static_cast<int*>(DUtil::get_value_from_damaris("ross/nlp", pe, 0, 0)));
        sim_config_->num_lp_.push_back(val);
        sim_config_->num_kp_pe_ = *(static_cast<int*>(DUtil::get_value_from_damaris(
                        "ross/nkp", pe, 0, 0)));
    }

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
        client_->set_config_ptr(std::move(sim_config_));
        client_->connect();
    }

    argobots_manager_->set_shared_ptrs(client_, sim_config_);
}

void RDServer::set_model_metadata()
{
    sim_config_->set_model_metadata();
}

void RDServer::finalize()
{
    if (sim_config_->write_data_)
        data_file_.close();

    if (sim_config_->stream_data_)
        client_->close();

    argobots_manager_->finalize();
}

void RDServer::update_gvt(int32_t step)
{
    last_gvt_ = *(static_cast<double*>(DUtil::get_value_from_damaris("ross/last_gvt",
                    my_pes_.front(), step, 0)));
    cout << "[RDServer] last GVT " << last_gvt_ << endl;
}

