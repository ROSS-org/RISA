#include <plugins/data/ArgobotsManager.h>
#include <plugins/data/analysis-tasks.h>

using namespace ross_damaris::data;
using namespace ross_damaris::sample;

ArgobotsManager::ArgobotsManager() :
    last_processed_gvt_(0.0),
    last_processed_rts_(0.0),
    last_processed_vts_(0.0),
    data_manager_(nullptr),
    stream_client_(nullptr),
    sim_config_(nullptr)
{
    primary_xstream_ = (ABT_xstream*)malloc(sizeof(ABT_xstream));
    proc_xstream_ = (ABT_xstream*)malloc(sizeof(ABT_xstream));
    pool_ = (ABT_pool*)malloc(sizeof(ABT_pool));
    scheduler_ = (ABT_sched*)malloc(sizeof(ABT_sched));

    // initialize Argobots library
    cout << "[ArgobotsManager] start_processing()\n";
    ABT_init(0, NULL);

    // create shared pool
    ABT_pool_create_basic(ABT_POOL_FIFO, ABT_POOL_ACCESS_MPMC, ABT_TRUE, pool_);
    cout << "[ArgobotsManager] shared pool created\n";

    ABT_xstream_self(primary_xstream_);
    cout << "[ArgobotsManager] got self pointer\n";
    cout << "[ArgobotsManager] creating secondary xstream\n";
    ABT_xstream_create_basic(ABT_SCHED_DEFAULT, 1, pool_, ABT_SCHED_CONFIG_NULL, proc_xstream_);
    ABT_xstream_start(*proc_xstream_);
    //ABT_xstream_set_main_sched_basic(*proc_xstream_, ABT_SCHED_DEFAULT, 1, pool_);
    cout << "[ArgobotsManager] about to create task\n";
    printf("address of pool %p\n", (ABT_pool *) pool_);
    //ABT_task_create(*pool_, insert_data_mic, NULL, NULL);
    cout << "[ArgobotsManager] task created\n";
}

ArgobotsManager::ArgobotsManager(ArgobotsManager&& m) :
    last_processed_gvt_(m.last_processed_gvt_),
    last_processed_rts_(m.last_processed_rts_),
    last_processed_vts_(m.last_processed_vts_),
    data_manager_(std::move(m.data_manager_)),
    stream_client_(std::move(m.stream_client_)),
    sim_config_(std::move(m.sim_config_)),
    primary_xstream_(m.primary_xstream_),
    proc_xstream_(m.proc_xstream_),
    pool_(m.pool_),
    scheduler_(m.scheduler_)
{
    m.primary_xstream_ = nullptr;
    m.proc_xstream_ = nullptr;
    m.pool_ = nullptr;
    m.scheduler_ = nullptr;
}

ArgobotsManager::~ArgobotsManager()
{
    if (primary_xstream_)
        free(primary_xstream_);
    if (proc_xstream_)
        free(proc_xstream_);
    if (pool_)
        free(pool_);
    if (scheduler_)
        free(scheduler_);
}

void ArgobotsManager::set_shared_ptrs(boost::shared_ptr<DataManager>& dm_ptr,
            boost::shared_ptr<streaming::StreamClient>& sc_ptr,
            boost::shared_ptr<config::SimConfig>& conf_ptr)
{
    data_manager_ = boost::shared_ptr<DataManager>(dm_ptr);
    stream_client_ = boost::shared_ptr<streaming::StreamClient>(sc_ptr);
    sim_config_ = boost::shared_ptr<config::SimConfig>(conf_ptr);

}

void ArgobotsManager::aggregate_data(InstMode mode, double lower_ts, double upper_ts)
{
    // want to aggregate data regularly
    // currently only worried about sim engine data here, not model
    // two cases to consider here:
    // 1) we're collecting data on PE level already, we don't need to do anything,
    //    except combine multiple PE flatbuffers for a sampling point into one sample
    // 2) collecting at KP and/or LP level, but not PE, then do KP/LP aggregation as well
    //    as combining the flatbuffers from different PEs
    //
    // Need to do this for each sampling mode that is in use
    // GVT-based and RT sampling are relatively easy. One sample per time point
    // For VTS, we may have 1 or more samples per virtual time point (because of rollbacks).
    // Options on handling this:
    // 1) aggregate into a single sample for this sampling point
    // 2) aggregate entities within a sample, but maintain multiple samples (with real time
    //    differentiating them)
    //
    // get data iterators
    SamplesByKey::iterator begin, end;
    data_manager_->find_data(mode, lower_ts, upper_ts, begin, end);

    if (mode == InstMode_GVT || mode == InstMode_RT)
    {
        if (sim_config_->pe_data())
            combine_pe_flatbuffers(mode, lower_ts, upper_ts);
        else if (sim_config_->kp_data())
            combine_kp_flatbuffers(mode, lower_ts, upper_ts);
        else if (sim_config_->lp_data())
            combine_lp_flatbuffers(mode, lower_ts, upper_ts);
        else
            cout << "[ArgobotsManager] aggregate_data(): how did we get here?\n";
    }
    else if (mode == InstMode_VT)
    {

    }
}


void ArgobotsManager::combine_pe_flatbuffers(sample::InstMode mode,
        double lower_ts, double upper_ts)
{
    SamplesByKey::iterator it, end;
    data_manager_->find_data(mode, lower_ts, upper_ts, it, end);
    while (it != end)
    {
        DamarisDataSampleT combined_sample;
        DamarisDataSampleT ds;
        auto outer_it = (*it)->ds_ptrs_begin();
        auto outer_end = (*it)->ds_ptrs_end();
        auto dataspace = (*outer_it).second[-1];
        auto data_fb = GetDamarisDataSample(dataspace.GetData());
        data_fb->UnPackTo(&combined_sample);
        // we don't care about the KPs/LPs for aggregate data
        combined_sample.kp_data.clear();
        combined_sample.lp_data.clear();
        // TODO should we keep model data?
        combined_sample.model_data.clear();
        ++outer_it;

        while (outer_it != outer_end)
        {
            dataspace = (*outer_it).second[-1];
            data_fb = GetDamarisDataSample(dataspace.GetData());
            data_fb->UnPackTo(&ds);
            std::move(ds.pe_data.begin(), ds.pe_data.end(),
                    std::back_inserter(combined_sample.pe_data));
            ++outer_it;
        }
        stream_client_->enqueue_data(&combined_sample);
        ++it;
    }
    set_last_processed(mode, upper_ts);
}

void ArgobotsManager::combine_kp_flatbuffers(sample::InstMode mode,
        double lower_ts, double upper_ts)
{

}

void ArgobotsManager::combine_lp_flatbuffers(sample::InstMode mode,
        double lower_ts, double upper_ts)
{

}

void ArgobotsManager::forward_model_data()
{

}

void ArgobotsManager::create_init_data_proc_task(int32_t step)
{
    ABT_task_create(*pool_, insert_data_mic, NULL, NULL);
}

void ArgobotsManager::finalize()
{
    // Can't put these calls in the destructor, else they might get called before
    // they're supposed to
    if (ABT_initialized() == ABT_SUCCESS)
    {
        ABT_xstream_join(*proc_xstream_);
        ABT_xstream_free(proc_xstream_);
        ABT_finalize();
    }

}
