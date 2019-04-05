#include <unordered_map>
#include <unordered_set>
#include <plugins/data/analysis-tasks.h>
#include <plugins/flatbuffers/data_sample_generated.h>
#include <plugins/util/damaris-util.h>
#include <plugins/util/SimConfig.h>
#include <plugins/streaming/StreamClient.h>

#include <damaris/buffer/DataSpace.hpp>
#include <damaris/buffer/Buffer.hpp>
#include <damaris/data/VariableManager.hpp>
#include <damaris/data/Variable.hpp>

#include <instrumentation/st-instrumentation-internal.h>
#include <instrumentation/st-model-data.h>

using namespace ross_damaris;
using namespace ross_damaris::util;
using namespace ross_damaris::sample;
//using namespace ross_damaris::data;
using namespace std;
namespace fb = flatbuffers;

config::SimConfig* sim_config = nullptr;
streaming::StreamClient* stream_client = nullptr;

const char * const inst_buffer_names[] = {
    "",
    "ross/inst_sample/gvt_inst",
    "ross/inst_sample/vts_inst",
    "ross/inst_sample/rts_inst"
};

void init_analysis_tasks()
{
    sim_config = config::SimConfig::get_instance();
    stream_client = streaming::StreamClient::get_instance();
}

// TODO add in size checking for errors
void create_flatbuffer(InstMode mode, char* buffer, sample_metadata *sample_md,
        fb::FlatBufferBuilder& fbb, vector<fb::Offset<PEData>>& pe_vector,
        vector<fb::Offset<KPData>>& kp_vector, vector<fb::Offset<LPData>>& lp_vector,
        vector<fb::Offset<ModelLP>>& mlp_vector)
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
        // TODO if VTS, save model data separately from sim engine data
        cout << "analysis-tasks has_model\n";
        for (int i = 0; i < sample_md->num_model_lps; i++)
        {
            vector<fb::Offset<ModelVariable>> cur_lp_vars;
            st_model_data *model_lp = reinterpret_cast<st_model_data*>(buffer);
            buffer += sizeof(*model_lp);
            auto lp_md = sim_config->get_lp_model_md(sample_md->peid, model_lp->kpid, model_lp->lpid);
            for (int j = 0; j < model_lp->num_vars; j++)
            {
                model_var_md *var = reinterpret_cast<model_var_md*>(buffer);
                buffer += sizeof(*var);
                fb::Offset<ModelVariable> variable;
                auto var_name = (*lp_md)->get_var_name(j);
                switch (var->type)
                {
                    case MODEL_INT:
                    {
                        int *data = reinterpret_cast<int*>(buffer);
                        buffer += sizeof(int) * var->num_elems;
                        vector<int> data_vec(data, data + var->num_elems);
                        auto var_union = CreateIntVarDirect(fbb, &data_vec).Union();
                        variable = CreateModelVariableDirect(fbb, var_name.c_str(),
                                VariableType_IntVar, var_union);
                        break;
                    }
                    case MODEL_LONG:
                    {
                        long *data = reinterpret_cast<long*>(buffer);
                        buffer += sizeof(long) * var->num_elems;
                        vector<long> data_vec(data, data + var->num_elems);
                        auto var_union = CreateLongVarDirect(fbb, &data_vec).Union();
                        variable = CreateModelVariableDirect(fbb, var_name.c_str(),
                                VariableType_LongVar, var_union);
                        break;
                    }
                    case MODEL_FLOAT:
                    {
                        float *data = reinterpret_cast<float*>(buffer);
                        buffer += sizeof(float) * var->num_elems;
                        vector<float> data_vec(data, data + var->num_elems);
                        auto var_union = CreateFloatVarDirect(fbb, &data_vec).Union();
                        variable = CreateModelVariableDirect(fbb, var_name.c_str(),
                                VariableType_FloatVar, var_union);
                        break;
                    }
                    case MODEL_DOUBLE:
                    {
                        double *data = reinterpret_cast<double*>(buffer);
                        buffer += sizeof(double) * var->num_elems;
                        vector<double> data_vec(data, data + var->num_elems);
                        auto var_union = CreateDoubleVarDirect(fbb, &data_vec).Union();
                        variable = CreateModelVariableDirect(fbb, var_name.c_str(),
                                VariableType_DoubleVar, var_union);
                        break;
                    }
                    default:
                        break;
                }
                cur_lp_vars.push_back(variable);
            }
            auto mlp_name = (*lp_md)->get_name();
            auto mlp = CreateModelLPDirect(fbb, model_lp->lpid, mlp_name.c_str(), &cur_lp_vars);
            mlp_vector.push_back(mlp);

        }

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
    //cout << "initial data processing: step " << step << endl;
    //printf("initial_data_processing() called at step %d, pool_id %d\n", step, pool_id);

    const InstMode *modes = EnumValuesInstMode();
    //TODO split into multiple argobots tasks?
    for (int mode = InstMode_GVT; mode <= InstMode_RT; mode++)
    {
        if (!sim_config->inst_mode_sim(modes[mode]) && !sim_config->inst_mode_model(modes[mode]))
            continue;

        damaris::BlocksByIteration::iterator begin, end;
        DUtil::get_damaris_iterators(inst_buffer_names[mode], step, begin, end);
        if (begin == end)
        {
            // TODO there's potentially a bug here if this task is not done quickly enough,
            // the task may be too late to access the data
            // might be garbage collection acting too quickly?
            // What we should do is after we process this data,
            // set up garbage collection for this time step
            // we don't need it in damaris format anymore
            // Seems to have been fixed for not, but leaving notes just incase
            cout << "initial_data_processing: begin == end\n";
            continue;
        }

        fb::FlatBufferBuilder fbb;
        vector<fb::Offset<PEData>> pe_vector;
        vector<fb::Offset<KPData>> kp_vector;
        vector<fb::Offset<LPData>> lp_vector;
        vector<fb::Offset<ModelLP>> mlp_vector;
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
            //cout << "PE : " << sample_md->peid << " last_GVT: " << last_gvt << endl;

            int id;
            if (mode == InstMode_VT)
                id = sample_md->kp_gid;
            else
                id = sample_md->peid;
            create_flatbuffer(modes[mode], dbuf_cur, sample_md, fbb, pe_vector, kp_vector, lp_vector,
                    mlp_vector);

            ++begin;
        }

        auto sample = CreateDamarisDataSampleDirect(fbb, vts, rts, last_gvt, modes[mode],
            &pe_vector, &kp_vector, &lp_vector, &mlp_vector);
        fbb.Finish(sample);
        int fb_size = static_cast<int>(fbb.GetSize());
        size_t size, offset;
        uint8_t *raw = fbb.ReleaseRaw(size, offset);
        stream_client->enqueue_data(raw, &raw[offset], fb_size);

        // GC this variable for this time step
        std::shared_ptr<damaris::Variable> v = damaris::VariableManager::Search(inst_buffer_names[mode]);
        v->ClearIteration(step);
    }

    free(args);
}

