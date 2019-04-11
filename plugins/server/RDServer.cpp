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

RDServer* RDServer::get_instance()
{
    static RDServer instance;
    return &instance;
}

RDServer::RDServer() :
        cur_mode_(sample::InstMode_GVT),
        cur_ts_(0.0),
        last_gvt_(0.0),
        stream_client_(nullptr),
        sim_config_(nullptr),
        argobots_manager_(nullptr)
{
    //int my_id = damaris::Environment::GetEntityProcessID();

    // TODO should this be for this Server's PEs or all PEs?
    my_pes_ = damaris::Environment::GetKnownLocalClients();

    sim_config_ = SimConfig::get_instance();

    if (sim_config_->write_data())
        data_file_.open("test-fb.bin", ios::out | ios::trunc | ios::binary);

    if (sim_config_->stream_data())
    {
        stream_client_ = streaming::StreamClient::get_instance();
        stream_client_->connect();
    }

    argobots_manager_ = ArgobotsManager::get_instance();
}

void RDServer::finalize()
{
    argobots_manager_->finalize();

    if (sim_config_->write_data())
        data_file_.close();

    if (sim_config_->stream_data())
    {
        stream_client_->close();
    }
}

void RDServer::update_gvt(int32_t step)
{
    last_gvt_ = *(static_cast<double*>(DUtil::get_value_from_damaris("ross/last_gvt",
                    my_pes_.front(), step, 0)));
    //cout << "[RDServer] last GVT " << last_gvt_ << endl;
    argobots_manager_->forward_vts_data(last_gvt_);
}

void RDServer::initial_data_tasks(int32_t step)
{
    argobots_manager_->create_initial_data_task(step);
}

