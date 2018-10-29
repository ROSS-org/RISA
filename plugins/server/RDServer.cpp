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

using namespace ross_damaris::server;
using namespace ross_damaris::util;
using namespace ross_damaris::sample;
using namespace ross_damaris::data;
using namespace ross_damaris::config;
namespace bip = boost::asio::ip;

RDServer::RDServer() :
        cur_mode_(sample::InstMode_GVT),
        cur_ts_(0.0),
        last_gvt_(0.0),
        use_threads_(true),
        client_(boost::make_shared<streaming::StreamClient>(streaming::StreamClient())),
        sim_config_(boost::make_shared<config::SimConfig>(config::SimConfig())),
        data_manager_(boost::make_shared<data::DataManager>(data::DataManager())),
        argobots_manager_(boost::make_shared<data::ArgobotsManager>(data::ArgobotsManager())),
        processor_(nullptr)
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

    if (use_threads_)
    {
        argobots_manager_->set_shared_ptrs(data_manager_, client_, sim_config_);
        argobots_manager_->start_processing();
    }
    else
        setup_data_processing();

}

void RDServer::setup_data_processing()
{
    processor_.reset(new data::DataProcessor(std::move(data_manager_),
                std::move(client_), std::move(sim_config_)));
    // sets up thread that performs data processing tasks
}

void RDServer::finalize()
{
    if (sim_config_->write_data_)
        data_file_.close();

    if (sim_config_->stream_data_)
        client_->close();

    data_manager_->clear();
    if (use_threads_)
        argobots_manager_->finalize();
}

void RDServer::update_gvt(int32_t step)
{
    last_gvt_ = *(static_cast<double*>(DUtil::get_value_from_damaris("ross/last_gvt",
                    my_pes_.front(), step, 0)));
    if (!use_threads_)
        processor_->last_gvt(last_gvt_);
    cout << "[RDServer] last GVT " << last_gvt_ << endl;
}

void RDServer::process_sample(boost::shared_ptr<damaris::Block> block)
{
        argobots_manager_->create_init_data_proc_task(1);
        damaris::DataSpace<damaris::Buffer> ds(block->GetDataSpace());
        auto data_fb = GetDamarisDataSample(ds.GetData());
        double ts;
        switch (data_fb->mode())
        {
            case InstMode_GVT:
                ts = data_fb->last_gvt();
                break;
            case InstMode_VT:
                ts = data_fb->virtual_ts();
                break;
            case InstMode_RT:
                ts = data_fb->real_ts();
                break;
            default:
                break;
        }

        int id = data_fb->entity_id();

        // first we need to check to see if this is data for an existing sampling point
        auto sample_it = data_manager_->find_data(data_fb->mode(), ts);
        if (sample_it != data_manager_->end())
        {
            (*sample_it)->push_ds_ptr(id, data_fb->event_id(), ds);
            //cout << "num ds_ptrs " << (*sample_it)->get_ds_ptr_size() << endl;
        }
        else // couldn't find it
        {
            DataSample s(ts, data_fb->mode(), DataStatus_speculative);
            s.push_ds_ptr(id, data_fb->event_id(), ds);
            data_manager_->insert_data(std::move(s));
            //data_manager_->print_manager_info();
        }

        cur_mode_ = data_fb->mode();
        cur_ts_ = ts;

        //auto obj = data_fb->UnPack();
        //flatbuffers::FlatBufferBuilder fbb;
        //auto new_samp = DamarisDataSample::Pack(fbb, obj);
        //fbb.Finish(new_samp);
        //cout << "FB size: " << fbb.GetSize() << endl;
        //auto mode = data_fb->mode();
}

void RDServer::forward_data(int32_t step)
{
    if (use_threads_)
        return;

    if (cur_mode_ == InstMode_VT)
        processor_->forward_data(cur_mode_, last_gvt_, step);
    else
        processor_->forward_data(cur_mode_, cur_ts_, step);
}

