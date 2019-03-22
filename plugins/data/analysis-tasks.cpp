#include <unordered_map>
#include <unordered_set>
#include <plugins/data/analysis-tasks.h>
#include <plugins/data/DataSample.h>
#include <plugins/flatbuffers/data_sample_generated.h>
#include <plugins/util/damaris-util.h>

#include <damaris/buffer/DataSpace.hpp>
#include <damaris/buffer/Buffer.hpp>
#include <damaris/data/Variable.hpp>

#include <instrumentation/st-instrumentation-internal.h>

using namespace ross_damaris;
using namespace ross_damaris::util;
using namespace ross_damaris::sample;
using namespace ross_damaris::data;
using namespace std;
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

const char * const inst_buffer_names[] = {
    "",
    "ross/inst_sample/gvt_inst",
    "ross/inst_sample/vts_inst",
    "ross/inst_sample/rts_inst"
};

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

// TODO add in size checking for errors
void create_flatbuffer(InstMode mode, char* buffer, sample_metadata *sample_md,
        fb::FlatBufferBuilder& fbb, vector<fb::Offset<PEData>>& pe_vector,
        vector<fb::Offset<KPData>>& kp_vector, vector<fb::Offset<LPData>>& lp_vector)
{
    if (sample_md->has_pe)
    {
        st_pe_stats *pe_stats = reinterpret_cast<st_pe_stats*>(buffer);
        buffer += sizeof(*pe_stats);
        SimEngineMetricsBuilder sim_builder(fbb);
        sim_builder.add_nevent_processed(pe_stats->s_nevent_processed);
        sim_builder.add_nevent_abort(pe_stats->s_nevent_abort);
        sim_builder.add_nevent_rb(pe_stats->s_e_rbs);
        sim_builder.add_rb_total(pe_stats->s_rb_total);
        sim_builder.add_rb_prim(pe_stats->s_rb_primary);
        sim_builder.add_rb_sec(pe_stats->s_rb_secondary);
        sim_builder.add_fc_attempts(pe_stats->s_fc_attempts);
        sim_builder.add_pq_qsize(pe_stats->s_pq_qsize);
        sim_builder.add_network_send(pe_stats->s_nsend_network);
        sim_builder.add_network_recv(pe_stats->s_nread_network);
        sim_builder.add_num_gvt(pe_stats->s_ngvts);
        sim_builder.add_event_ties(pe_stats->s_pe_event_ties);

        int net_events = pe_stats->s_nevent_processed - pe_stats->s_e_rbs;
        if (net_events > 0)
            sim_builder.add_efficiency((float) 100.0 * (1.0 - ((float) pe_stats->s_e_rbs / (float) net_events)));
        else
            sim_builder.add_efficiency(0);
        sim_builder.add_net_read_time(pe_stats->s_net_read);
        sim_builder.add_net_other_time(pe_stats->s_net_other);
        sim_builder.add_gvt_time(pe_stats->s_gvt);
        sim_builder.add_fc_time(pe_stats->s_fossil_collect);
        sim_builder.add_event_abort_time(pe_stats->s_event_abort);
        sim_builder.add_event_proc_time(pe_stats->s_event_process);
        sim_builder.add_pq_time(pe_stats->s_pq);
        sim_builder.add_rb_time(pe_stats->s_rollback);
        sim_builder.add_cancel_q_time(pe_stats->s_cancel_q);
        sim_builder.add_avl_time(pe_stats->s_avl);
        auto sim_data = sim_builder.Finish();
        pe_vector.push_back(CreatePEData(fbb, pe_stats->peid, sim_data));
    }

    if (sample_md->has_kp)
    {
        for (int i = 0; i < sample_md->has_kp; i++)
        {
            st_kp_stats *kp_stats = reinterpret_cast<st_kp_stats*>(buffer);
            buffer += sizeof(*kp_stats);
            SimEngineMetricsBuilder sim_builder(fbb);
            sim_builder.add_nevent_processed(kp_stats->s_nevent_processed);
            sim_builder.add_nevent_abort(kp_stats->s_nevent_abort);
            sim_builder.add_nevent_rb(kp_stats->s_e_rbs);
            sim_builder.add_rb_total(kp_stats->s_rb_total);
            sim_builder.add_rb_prim(kp_stats->s_rb_primary);
            sim_builder.add_rb_sec(kp_stats->s_rb_secondary);
            sim_builder.add_network_send(kp_stats->s_nsend_network);
            sim_builder.add_network_recv(kp_stats->s_nread_network);

            int net_events = kp_stats->s_nevent_processed - kp_stats->s_e_rbs;
            if (net_events > 0)
                sim_builder.add_efficiency((float) 100.0 * (1.0 - ((float) kp_stats->s_e_rbs / (float) net_events)));
            else
                sim_builder.add_efficiency(0);
            sim_builder.add_virtual_time_diff(kp_stats->time_ahead_gvt);
            auto sim_data = sim_builder.Finish();
            int kp_gid = sample_md->peid * sim_config->num_kp_pe() + kp_stats->kpid;
            cout << "pe: " << sample_md->peid << " kp: " << kp_stats->kpid << " num_kp " <<
                sim_config->num_kp_pe() << " kp_gid " << kp_gid << endl;
            kp_vector.push_back(CreateKPData(fbb, sample_md->peid, kp_stats->kpid, kp_gid, sim_data));
        }

    }

    if (sample_md->has_lp)
    {
        for (int i = 0; i < sample_md->has_lp; i++)
        {
            st_lp_stats *lp_stats = reinterpret_cast<st_lp_stats*>(buffer);
            buffer += sizeof(*lp_stats);
            SimEngineMetricsBuilder sim_builder(fbb);
            sim_builder.add_nevent_processed(lp_stats->s_nevent_processed);
            sim_builder.add_nevent_abort(lp_stats->s_nevent_abort);
            sim_builder.add_nevent_rb(lp_stats->s_e_rbs);
            sim_builder.add_network_send(lp_stats->s_nsend_network);
            sim_builder.add_network_recv(lp_stats->s_nread_network);

            int net_events = lp_stats->s_nevent_processed - lp_stats->s_e_rbs;
            if (net_events > 0)
                sim_builder.add_efficiency((float) 100.0 * (1.0 - ((float) lp_stats->s_e_rbs / (float) net_events)));
            else
                sim_builder.add_efficiency(0);
            auto sim_data = sim_builder.Finish();
            int kp_gid = sample_md->peid * sim_config->num_kp_pe() + lp_stats->kpid;
            // TODO do we actually need a local LP id?
            // i.e., this is just the index into g_tw_lp
            lp_vector.push_back(CreateLPData(fbb, sample_md->peid, lp_stats->kpid, kp_gid, 0,
                        lp_stats->lpid, sim_data));
        }
    }

    if (sample_md->has_model)
    {

    }

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
    //printf("initial_data_processing() called at step %d, pool_id %d\n", step, pool_id);
    damaris::BlocksByIteration::iterator begin, end;

    const InstMode *modes = EnumValuesInstMode();
    for (int mode = InstMode_GVT; mode <= InstMode_RT; mode++)
    {
        //TODO add support for model data
        if (!sim_config->inst_mode_sim(modes[mode]))
            continue;

        DUtil::get_damaris_iterators(inst_buffer_names[mode], step, begin, end);
        if (begin == end)
        {
            cout << "initial_data_processing: begin == end\n";
            continue;
        }
        //std::unordered_map<int, std::set<double>> sampling_points;

        // keep the iterator for the previous iteration because we're likely going to need it
        SamplesByKey::iterator sample_it = data_manager->end();

        fb::FlatBufferBuilder fbb;
        vector<fb::Offset<PEData>> pe_vector;
        vector<fb::Offset<KPData>> kp_vector;
        vector<fb::Offset<LPData>> lp_vector;
        double vts, rts, last_gvt;

        while (begin != end)
        {
            // each iterator is to a buffer sample from ROSS
            // For GVT and RTS, this is one buffer per PE per sampling point
            // for VTS, this is one buffer per KP per sampling point
            // for VTS and RTS, we may have more than one sampling point

            damaris::DataSpace<damaris::Buffer> ds((*begin)->GetDataSpace());
            char* dbuf_cur = reinterpret_cast<char*>(ds.GetData());
            sample_metadata *sample_md = reinterpret_cast<sample_metadata*>(dbuf_cur);
            dbuf_cur += sizeof(*sample_md);
            double ts = 0.0;
            switch (mode)
            {
                case InstMode_GVT:
                    ts = sample_md->last_gvt;
                    break;
                case InstMode_VT:
                    ts = sample_md->vts;
                    break;
                case InstMode_RT:
                    ts = sample_md->rts;
                    break;
                default:
                    break;
            }
            vts = sample_md->vts;
            rts = sample_md->rts;
            last_gvt = sample_md->last_gvt;

            //sampling_points[mode].insert(ts);

            // TODO kpid for VTS
            int id = sample_md->peid;
            create_flatbuffer(modes[mode], dbuf_cur, sample_md, fbb, pe_vector, kp_vector, lp_vector);

            // does this DataSpace belong in the DataSample from the previous iteration?
            if (sample_it == data_manager->end() || !(*sample_it)->same_sample(modes[mode], ts))
                sample_it = data_manager->find_data(modes[mode], ts);

            if (sample_it != data_manager->end())
            {
                // TODO need event id for VTS
                (*sample_it)->push_ds_ptr(id, -1, ds);
                //cout << "num ds_ptrs " << (*sample_it)->get_ds_ptr_size() << endl;
            }
            else // couldn't find it
            {
                DataSample s(ts, modes[mode], DataStatus_speculative);
                s.push_ds_ptr(id, -1, ds);
                data_manager->insert_data(std::move(s));
            }

            ++begin;
        }

        auto pes = fbb.CreateVector(pe_vector);
        auto kps = fbb.CreateVector(kp_vector);
        auto lps = fbb.CreateVector(lp_vector);
        auto sample = CreateDamarisDataSample(fbb, vts, rts, last_gvt, modes[mode], pes, kps, lps);
        fbb.Finish(sample);
        int fb_size = static_cast<int>(fbb.GetSize());
        size_t size, offset;
        uint8_t *raw = fbb.ReleaseRaw(size, offset);
        stream_client->enqueue_data(raw, &raw[offset], fb_size);
    }

    // Now set up task(s) to do data aggregation
    // Since we use a FIFO queue along with just one processing ES,
    // things should be done in the correct order.
    // In the future, we'll need to make sure to handle correctly
    //for (auto it = sampling_points.begin(); it != sampling_points.end(); ++it)
    //{
    //    for (auto set_it = (*it).second.begin(); set_it != (*it).second.end(); ++set_it)
    //    {
    //        data_agg_args *agg_args = (data_agg_args*)malloc(sizeof(data_agg_args));
    //        agg_args->mode = static_cast<InstMode>((*it).first);
    //        agg_args->lower_ts = (*set_it);
    //        agg_args->upper_ts = (*set_it);
    //        ABT_task_create(pool, convert_to_flatbuffers, agg_args, NULL);
    //        //ABT_task_create(pool, aggregate_data, agg_args, NULL);
    //    }
    //}

    // TODO if using multiple inst modes, need to make sure to do a tasklet for each?

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

