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
namespace fb = flatbuffers;

/* forward declarations for static functions */
static void combine_pe_flatbuffers(sample::InstMode mode, double lower_ts, double upper_ts);
static void combine_flatbuffers(sample::InstMode mode, double lower_ts, double upper_ts);
static void sum_data(SimEngineMetricsT* pe_metrics, SimEngineMetricsT* entity_metrics);
static void iterate_kps(DamarisDataSampleT* ds, SimEngineMetricsT *pe_metrics, int *pe_id);
static void iterate_lps(DamarisDataSampleT* ds, SimEngineMetricsT *pe_metrics, int *pe_id);

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
        else if (sim_config->kp_data() || sim_config->lp_data())
            combine_flatbuffers(mode, lower_ts, upper_ts);
        else
            cout << "[ArgobotsManager] aggregate_data(): how did we get here?\n";
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

void combine_flatbuffers(sample::InstMode mode, double lower_ts, double upper_ts)
{
    cout << "combine_flatbuffers()\n";
    SamplesByKey::iterator it, end;
    data_manager->find_data(mode, lower_ts, upper_ts, it, end);
    std::vector<fb::Offset<PEData>> pes;
    while (it != end)
    {
        // it is iterator to a DataSample
        fb::FlatBufferBuilder fbb;
        DamarisDataSampleT ds;
        auto outer_it = (*it)->ds_ptrs_begin();
        auto outer_end = (*it)->ds_ptrs_end();
        double virtual_ts, real_ts, last_gvt;

        while (outer_it != outer_end)
        {
            //outer_it is iterator to a damaris DataSpace
            //which for RTS and GVT sampling represents a single PE sample
            SimEngineMetricsT pe_metrics;
            auto dataspace = (*outer_it).second[-1];
            auto data_fb = GetDamarisDataSample(dataspace.GetData());
            data_fb->UnPackTo(&ds);
            virtual_ts = ds.virtual_ts;
            real_ts = ds.real_ts;
            last_gvt = ds.last_gvt;

            int pe_id;
            if (sim_config->kp_data())
                iterate_kps(&ds, &pe_metrics, &pe_id);
            else if (sim_config->lp_data())
                iterate_lps(&ds, &pe_metrics, &pe_id);

            fb::Offset<SimEngineMetrics> pe_metrics_fb = SimEngineMetrics::Pack(fbb, &pe_metrics);
            pes.push_back(CreatePEData(fbb, pe_id, pe_metrics_fb));
            ++outer_it;
        }

        auto sample_fb = CreateDamarisDataSampleDirect(fbb, virtual_ts, real_ts, last_gvt,
                mode, &pes);
        fbb.Finish(sample_fb);
        int fb_size = static_cast<int>(fbb.GetSize());
        size_t size, offset;
        uint8_t *raw = fbb.ReleaseRaw(size, offset);
        stream_client->enqueue_data(raw, &raw[offset], fb_size);
        //stream_client->enqueue_data(&combined_sample);

        pes.clear();
        ++it;
    }
}

void iterate_kps(DamarisDataSampleT* ds, SimEngineMetricsT *pe_metrics, int *pe_id)
{
    cout << "iterate_kps()\n";
    auto kp_it = ds->kp_data.begin();
    auto kp_end = ds->kp_data.end();
    *pe_id = (*kp_it)->peid;
    while (kp_it != kp_end)
    {
        // kp_it is iterator to a KPDataT
        sum_data(pe_metrics, (*kp_it)->data.get());
        ++kp_it;
    }
}

void iterate_lps(DamarisDataSampleT* ds, SimEngineMetricsT *pe_metrics, int *pe_id)
{
    cout << "iterate_lps()\n";
    auto lp_it = ds->lp_data.begin();
    auto lp_end = ds->lp_data.end();
    *pe_id = (*lp_it)->peid;
    while (lp_it != lp_end)
    {
        // lp_it is iterator to a lpDataT
        sum_data(pe_metrics, (*lp_it)->data.get());
        ++lp_it;
    }
}

//template <typename T>
void sum_data(SimEngineMetricsT* pe_metrics, SimEngineMetricsT* entity_metrics)
{
    // TODO perhaps provide averages as well for aggregated data?
    pe_metrics->nevent_processed += entity_metrics->nevent_processed;
    pe_metrics->nevent_abort += entity_metrics->nevent_abort;
    pe_metrics->nevent_rb += entity_metrics->nevent_rb;
    pe_metrics->rb_total += entity_metrics->rb_total;
    pe_metrics->rb_prim += entity_metrics->rb_prim;
    pe_metrics->rb_sec += entity_metrics->rb_sec;
    pe_metrics->fc_attempts += entity_metrics->fc_attempts;
    pe_metrics->pq_qsize += entity_metrics->pq_qsize;
    pe_metrics->network_send += entity_metrics->network_send;
    pe_metrics->network_recv += entity_metrics->network_recv;
    pe_metrics->num_gvt += entity_metrics->num_gvt;
    pe_metrics->event_ties += entity_metrics->event_ties;
    //pe_metrics->efficiency += entity_metrics.efficiency;
    // KPs/LPs don't collect the below data
    //pe_metrics->net_read_time += entity_metrics.net_read_time;
    //pe_metrics->net_other_time += entity_metrics.net_other_time;
    //pe_metrics->gvt_time += entity_metrics.gvt_time;
    //pe_metrics->fc_time += entity_metrics.fc_time;
    //pe_metrics->event_abort_time += entity_metrics.event_abort_time;
    //pe_metrics->event_proc_time += entity_metrics.event_proc_time;
    //pe_metrics->pq_time += entity_metrics.pq_time;
    //pe_metrics->rb_time += entity_metrics.rb_time;
    //pe_metrics->cancel_q_time += entity_metrics.cancel_q_time;
    //pe_metrics->avl_time += entity_metrics.avl_time;
}

