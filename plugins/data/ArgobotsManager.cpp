#include <plugins/data/ArgobotsManager.h>
#include <plugins/data/analysis-tasks.h>

using namespace ross_damaris::data;
using namespace ross_damaris::sample;

void ArgobotsManager::set_shared_ptrs(boost::shared_ptr<DataManager>& dm_ptr,
            boost::shared_ptr<streaming::StreamClient>& sc_ptr,
            boost::shared_ptr<config::SimConfig>& conf_ptr)
{
    data_manager_ = boost::shared_ptr<DataManager>(dm_ptr);
    stream_client_ = boost::shared_ptr<streaming::StreamClient>(sc_ptr);
    sim_config_ = boost::shared_ptr<config::SimConfig>(conf_ptr);
}

void ArgobotsManager::start_processing()
{
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
    ABT_task_create(*pool_, insert_data_mic, NULL, NULL);
    cout << "[ArgobotsManager] task created\n";


    //std::cout << "Thread " << std::this_thread::get_id() << " start_processing()\n";
    // TODO this can have a loop that does various types of processing
    //while (true)
    //{
    //    if (last_processed_gvt_ <= data_manager_->most_recent_gvt())
    //    {
    //        //std::cout << "[ArgobotsManager] aggregate_data()\n";
    //        aggregate_data(InstMode_GVT, last_processed_gvt_, data_manager_->most_recent_gvt());
    //    }
    //    // TODO how to break out for simulation end?
    //}
    //ABT_finalize();
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
    ABT_xstream_join(*proc_xstream_);
    ABT_xstream_free(proc_xstream_);
    ABT_finalize();
}
