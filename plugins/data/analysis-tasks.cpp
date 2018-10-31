#include <plugins/data/analysis-tasks.h>
#include <plugins/data/DataSample.h>
#include <plugins/flatbuffers/data_sample_generated.h>
#include <plugins/util/damaris-util.h>

#include <damaris/buffer/DataSpace.hpp>
#include <damaris/buffer/Buffer.hpp>
#include <damaris/data/Variable.hpp>

using namespace ross_damaris;
using namespace ross_damaris::util;
using namespace ross_damaris::sample;
using namespace ross_damaris::data;

/* forward declarations for static functions */
static void combine_pe_flatbuffers(sample::InstMode mode, double lower_ts, double upper_ts);

boost::shared_ptr<DataManager> data_manager = nullptr;
boost::shared_ptr<config::SimConfig> sim_config = nullptr;
boost::shared_ptr<streaming::StreamClient> stream_client = nullptr;

void initialize_task(void *arguments)
{
    init_args *args;
    args = (init_args*)arguments;
    data_manager = *reinterpret_cast<boost::shared_ptr<DataManager>*>(args->data_manager);
    sim_config = *reinterpret_cast<boost::shared_ptr<config::SimConfig>*>(args->sim_config);
    stream_client = *reinterpret_cast<boost::shared_ptr<streaming::StreamClient>*>(args->stream_client);
    free(args);
}

void aggregate_data(void *arguments)
{
    data_agg_args *args;
    args = (data_agg_args*)arguments;
    InstMode mode = static_cast<InstMode>(args->mode);
    double lower_ts = args->lower_ts;
    double upper_ts = args->upper_ts;
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

    // get data iterators
    SamplesByKey::iterator begin, end;
    data_manager->find_data(mode, lower_ts, upper_ts, begin, end);

    if (mode == InstMode_GVT || mode == InstMode_RT)
    {
        if (sim_config->pe_data())
            combine_pe_flatbuffers(mode, lower_ts, upper_ts);
    //    else if (sim_config_->kp_data())
    //        combine_kp_flatbuffers(mode, lower_ts, upper_ts);
    //    else if (sim_config_->lp_data())
    //        combine_lp_flatbuffers(mode, lower_ts, upper_ts);
    //    else
    //        cout << "[ArgobotsManager] aggregate_data(): how did we get here?\n";
    }
    //else if (mode == InstMode_VT)
    //{

    //}
}

void initial_data_processing(void *arguments)
{
    // first get my ES and pool pointers
    ABT_xstream xstream;
    ABT_pool pool;
    ABT_xstream_self(&xstream);
    ABT_xstream_get_main_pools(xstream, 1, &pool);
    int pool_id;
    ABT_pool_get_id(pool, &pool_id);

    initial_task_args *args;
    args = (initial_task_args*)arguments;
    int step = args->step;
    data_agg_args *agg_args = (data_agg_args*)malloc(sizeof(data_agg_args));
    //printf("initial_data_processing() called at step %d, pool_id %d\n", step, pool_id);
    damaris::BlocksByIteration::iterator begin, end;
    DUtil::get_damaris_iterators("ross/sample", step, begin, end);
    double lowest = DBL_MAX;
    double highest = 0.0;

    while (begin != end)
    {
        initial_task_args * task_args = (initial_task_args*)malloc(sizeof(initial_task_args));
        task_args->step = step;
        //cout << "ds.RefCount() = " << (*begin)->GetDataSpace().RefCount() << endl;
        task_args->ds = reinterpret_cast<const void*>(&((*begin)->GetDataSpace()));

        damaris::DataSpace<damaris::Buffer> ds((*begin)->GetDataSpace());
        auto data_fb = GetDamarisDataSample(ds.GetData());
        // TODO need to handle for multiple modes
        agg_args->mode = static_cast<int>(data_fb->mode());

        if (data_fb->last_gvt() < lowest)
            lowest = data_fb->last_gvt();
        if (data_fb->last_gvt() > highest)
            highest = data_fb->last_gvt();

        //cout << "src: " << (*begin)->GetSource() << " iteration: " << (*begin)->GetIteration()
        //    << " domain id: " << (*begin)->GetID() << endl;
        ABT_task_create(pool, insert_data_mic, task_args, NULL);
        begin++;
    }

    // Now set up a task to do data aggregation
    // Since we use a FIFO queue along with just one processing ES,
    // things should be done in the correct order.
    // In the future, we'll need to make sure to handle correctly
    agg_args->lower_ts = lowest;
    agg_args->upper_ts = highest;
    // TODO if using multiple inst modes, need to make sure to do a tasklet for each?
    ABT_task_create(pool, aggregate_data, agg_args, NULL);

    free(args);
}

void insert_data_mic(void *arguments)
{
    initial_task_args *args;
    args = (initial_task_args*)arguments;
    int step = args->step;
    //printf("insert_data_mic() called at step %d\n", step);
    const damaris::DataSpace<damaris::Buffer> ds = *reinterpret_cast<const damaris::DataSpace<damaris::Buffer>*>(args->ds);
    //cout << "ds.RefCount() = " << ds.RefCount() << endl;
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
    auto sample_it = data_manager->find_data(data_fb->mode(), ts);
    if (sample_it != data_manager->end())
    {
        (*sample_it)->push_ds_ptr(id, data_fb->event_id(), ds);
        //cout << "num ds_ptrs " << (*sample_it)->get_ds_ptr_size() << endl;
    }
    else // couldn't find it
    {
        DataSample s(ts, data_fb->mode(), DataStatus_speculative);
        s.push_ds_ptr(id, data_fb->event_id(), ds);
        data_manager->insert_data(std::move(s));
    }

    //cur_mode_ = data_fb->mode();
    //cur_ts_ = ts;

    free(args);
}

void remove_data_mic(void *arg)
{

}

/*** static functions used for data processing ***/
void combine_pe_flatbuffers(sample::InstMode mode, double lower_ts, double upper_ts)
{
    cout << "combine_pe_flatbuffers()\n";
    SamplesByKey::iterator it, end;
    data_manager->find_data(mode, lower_ts, upper_ts, it, end);
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
        stream_client->enqueue_data(&combined_sample);
        ++it;
    }
    //set_last_processed(mode, upper_ts);
}

