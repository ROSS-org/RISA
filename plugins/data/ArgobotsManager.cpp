#include <plugins/data/ArgobotsManager.h>

using namespace ross_damaris::data;
using namespace ross_damaris::sample;

void ArgobotsManager::start_processing()
{
    ABT_init(0, NULL);
    ABT_xstream_self(my_xstream_);
    std::cout << "Thread " << std::this_thread::get_id() << " start_processing()\n";
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
    ABT_finalize();
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
