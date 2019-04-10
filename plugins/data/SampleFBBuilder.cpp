#include <plugins/data/SampleFBBuilder.h>

using namespace ross_damaris::sample;
using namespace ross_damaris::data;
using namespace std;
namespace fb = flatbuffers;

SampleFBBuilder::SampleFBBuilder(SampleFBBuilder&& s) :
    fbb_(move(s.fbb_)),
    pe_vector_(move(s.pe_vector_)),
    kp_vector_(move(s.kp_vector_)),
    lp_vector_(move(s.lp_vector_)),
    mlp_vector_(move(s.mlp_vector_)),
    sim_config_(s.sim_config_),
    vts_(s.vts_),
    rts_(s.rts_),
    last_gvt_(s.last_gvt_),
    mode_(s.mode_),
    is_finished_(s.is_finished_),
    raw_(s.raw_),
    offset_(s.offset_),
    size_(s.size_),
    fb_size_(s.fb_size_) {  }

SampleFBBuilder& SampleFBBuilder::operator=(SampleFBBuilder&& s)
{
    sim_config_ = s.sim_config_;
    fbb_ = move(s.fbb_);
    vts_ = s.vts_;
    rts_ = s.rts_;
    last_gvt_ = s.last_gvt_;
    mode_ = s.mode_;
    pe_vector_ = move(s.pe_vector_);
    kp_vector_ = move(s.kp_vector_);
    lp_vector_ = move(s.lp_vector_);
    mlp_vector_ = move(s.mlp_vector_);
    is_finished_ = s.is_finished_;
    raw_ = s.raw_;
    offset_ = s.offset_;
    size_ = s.size_;
    fb_size_ = s.fb_size_;
    return *this;
}

void SampleFBBuilder::add_pe(const st_pe_stats* pe_stats)
{
    SimEngineMetricsBuilder sim_builder(fbb_);
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
        sim_builder.add_efficiency((float) 100.0 * (1.0 - ((float) pe_stats->s_e_rbs /
                        (float) net_events)));
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
    pe_vector_.push_back(CreatePEData(fbb_, pe_stats->peid, sim_data));
}

void SampleFBBuilder::add_kp(const st_kp_stats* kp_stats, int peid)
{
    SimEngineMetricsBuilder sim_builder(fbb_);
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
        sim_builder.add_efficiency((float) 100.0 * (1.0 - ((float) kp_stats->s_e_rbs /
                        (float) net_events)));
    else
        sim_builder.add_efficiency(0);

    sim_builder.add_virtual_time_diff(kp_stats->time_ahead_gvt);

    auto sim_data = sim_builder.Finish();
    int kp_gid = peid * sim_config_->num_kp_pe() + kp_stats->kpid;
    //cout << "pe: " << sample_md->peid << " kp: " << kp_stats->kpid << " num_kp " <<
    //    sim_config->num_kp_pe() << " kp_gid " << kp_gid << endl;
    kp_vector_.push_back(CreateKPData(fbb_, peid, kp_stats->kpid, kp_gid, sim_data));
}

void SampleFBBuilder::add_lp(const st_lp_stats* lp_stats, int peid)
{
    SimEngineMetricsBuilder sim_builder(fbb_);
    sim_builder.add_nevent_processed(lp_stats->s_nevent_processed);
    sim_builder.add_nevent_abort(lp_stats->s_nevent_abort);
    sim_builder.add_nevent_rb(lp_stats->s_e_rbs);
    sim_builder.add_network_send(lp_stats->s_nsend_network);
    sim_builder.add_network_recv(lp_stats->s_nread_network);

    int net_events = lp_stats->s_nevent_processed - lp_stats->s_e_rbs;
    if (net_events > 0)
        sim_builder.add_efficiency((float) 100.0 * (1.0 - ((float) lp_stats->s_e_rbs /
                        (float) net_events)));
    else
        sim_builder.add_efficiency(0);

    auto sim_data = sim_builder.Finish();
    int kp_gid = peid * sim_config_->num_kp_pe() + lp_stats->kpid;
    // TODO do we actually need a local LP id?
    // i.e., this is just the index into g_tw_lp
    lp_vector_.push_back(CreateLPData(fbb_, peid, lp_stats->kpid, kp_gid, 0,
                lp_stats->lpid, sim_data));
}

char* SampleFBBuilder::add_model_lp(st_model_data* model_lp, char* buffer, size_t& buf_size, int peid)
{
    auto lp_md = sim_config_->get_lp_model_md(peid, model_lp->kpid, model_lp->lpid);
    if (lp_md == sim_config_->end())
    {
        cout << "lp_md not found for pe " << peid << " kp " << model_lp->kpid <<
            " lp " << model_lp->lpid << endl;
    }
    vector<fb::Offset<ModelVariable>> cur_lp_vars;

    for (int j = 0; j < model_lp->num_vars; j++)
    {
        model_var_md *var = reinterpret_cast<model_var_md*>(buffer);
        buffer += sizeof(*var);
        buf_size -= sizeof(*var);
        fb::Offset<ModelVariable> variable;
        auto var_name = (*lp_md)->get_var_name(j);
        switch (var->type)
        {
            case MODEL_INT:
            {
                int *data = reinterpret_cast<int*>(buffer);
                buffer += sizeof(int) * var->num_elems;
                buf_size -= sizeof(int) * var->num_elems;
                vector<int> data_vec(data, data + var->num_elems);
                auto var_union = CreateIntVarDirect(fbb_, &data_vec).Union();
                variable = CreateModelVariableDirect(fbb_, var_name.c_str(),
                        VariableType_IntVar, var_union);
                break;
            }
            case MODEL_LONG:
            {
                long *data = reinterpret_cast<long*>(buffer);
                buffer += sizeof(long) * var->num_elems;
                buf_size -= sizeof(long) * var->num_elems;
                vector<long> data_vec(data, data + var->num_elems);
                auto var_union = CreateLongVarDirect(fbb_, &data_vec).Union();
                variable = CreateModelVariableDirect(fbb_, var_name.c_str(),
                        VariableType_LongVar, var_union);
                break;
            }
            case MODEL_FLOAT:
            {
                float *data = reinterpret_cast<float*>(buffer);
                buffer += sizeof(float) * var->num_elems;
                buf_size -= sizeof(float) * var->num_elems;
                vector<float> data_vec(data, data + var->num_elems);
                auto var_union = CreateFloatVarDirect(fbb_, &data_vec).Union();
                variable = CreateModelVariableDirect(fbb_, var_name.c_str(),
                        VariableType_FloatVar, var_union);
                break;
            }
            case MODEL_DOUBLE:
            {
                double *data = reinterpret_cast<double*>(buffer);
                buffer += sizeof(double) * var->num_elems;
                buf_size -= sizeof(double) * var->num_elems;
                vector<double> data_vec(data, data + var->num_elems);
                auto var_union = CreateDoubleVarDirect(fbb_, &data_vec).Union();
                variable = CreateModelVariableDirect(fbb_, var_name.c_str(),
                        VariableType_DoubleVar, var_union);
                break;
            }
            default:
                break;
        }
        cur_lp_vars.push_back(variable);
    }

    auto mlp_name = (*lp_md)->get_name();
    auto mlp = CreateModelLPDirect(fbb_, model_lp->lpid, mlp_name.c_str(), &cur_lp_vars);
    mlp_vector_.push_back(mlp);
    return buffer;
}

void SampleFBBuilder::finish()
{
    if (is_finished_)
        return;

    auto sample = CreateDamarisDataSampleDirect(fbb_, vts_, rts_, last_gvt_, mode_,
        &pe_vector_, &kp_vector_, &lp_vector_, &mlp_vector_);
    fbb_.Finish(sample);
    is_finished_ = true;
}

flatbuffers::FlatBufferBuilder* SampleFBBuilder::get_fbb()
{
    return &fbb_;
}

uint8_t* SampleFBBuilder::release_raw(size_t& fb_size, size_t& offset)
{
    if (!is_finished_)
        return nullptr;

    if (!raw_)
    {
        fb_size_ = fbb_.GetSize();
        raw_ = fbb_.ReleaseRaw(size_, offset_);
    }

    offset = offset_;
    fb_size = fb_size_;
    return raw_;
}

