#include <core/model-c.h>
#include <plugins/util/FlatBufferHelper.h>
#include <plugins/flatbuffers/data_sample_generated.h>
#include <ross.h>
#include <iostream>

using namespace std;

namespace ross_damaris {
namespace util{

using namespace ross_damaris::sample;

void FlatBufferHelper::start_lp_model_sample(int lpid)
{
    cur_lpid_ = lpid;
}

void FlatBufferHelper::finish_lp_model_sample()
{
    auto model_lp = CreateModelLPDirect(fbb_, cur_lpid_, cur_lp_type_.c_str(), &cur_lp_vars_);
    model_lps_.push_back(model_lp);

    // clean up cur lp tracking info
    cur_lp_type_ = "";
    cur_lpid_ = -1;
    cur_lp_vars_.clear();
}


void FlatBufferHelper::pe_sample(tw_pe *pe, tw_statistics *s, tw_statistics *last_pe_stats, int inst_type)
{
    // add in pe_comm data
    auto pe_comm = fbb_.CreateVector(pe->pe_comm, tw_nnodes());
    memset(pe->pe_comm, 0, tw_nnodes()*sizeof(int));

    SimEngineMetricsBuilder metrics(fbb_);
    metrics.add_nevent_processed(s->s_nevent_processed-last_pe_stats->s_nevent_processed);
    metrics.add_nevent_abort(s->s_nevent_abort-last_pe_stats->s_nevent_abort);
    metrics.add_nevent_rb(s->s_e_rbs-last_pe_stats->s_e_rbs);
    metrics.add_rb_total(s->s_rb_total-last_pe_stats->s_rb_total);
    metrics.add_rb_sec(s->s_rb_secondary-last_pe_stats->s_rb_secondary);
    metrics.add_fc_attempts(s->s_fc_attempts-last_pe_stats->s_fc_attempts);
    metrics.add_pq_qsize(tw_pq_get_size(pe->pq));
    metrics.add_network_send(s->s_nsend_network-last_pe_stats->s_nsend_network);
    metrics.add_network_recv(s->s_nread_network-last_pe_stats->s_nread_network);
    metrics.add_num_gvt(g_tw_gvt_done - last_pe_stats->s_ngvts);
    metrics.add_event_ties(s->s_pe_event_ties-last_pe_stats->s_pe_event_ties);

    metrics.add_net_read_time((pe->stats.s_net_read - last_pe_stats->s_net_read) / g_tw_clock_rate);
    metrics.add_net_other_time((pe->stats.s_net_other - last_pe_stats->s_net_other) / g_tw_clock_rate);
    metrics.add_gvt_time((pe->stats.s_gvt - last_pe_stats->s_gvt) / g_tw_clock_rate);
    metrics.add_fc_time((pe->stats.s_fossil_collect - last_pe_stats->s_fossil_collect) / g_tw_clock_rate);
    metrics.add_event_abort_time((pe->stats.s_event_abort - last_pe_stats->s_event_abort) / g_tw_clock_rate);
    metrics.add_event_proc_time((pe->stats.s_event_process - last_pe_stats->s_event_process) / g_tw_clock_rate);
    metrics.add_pq_time((pe->stats.s_pq - last_pe_stats->s_pq) / g_tw_clock_rate);
    metrics.add_rb_time((pe->stats.s_rollback - last_pe_stats->s_rollback) / g_tw_clock_rate);
    metrics.add_cancel_q_time((pe->stats.s_cancel_q - last_pe_stats->s_cancel_q) / g_tw_clock_rate);
    metrics.add_avl_time((pe->stats.s_avl - last_pe_stats->s_avl) / g_tw_clock_rate);
    metrics.add_comm_data(pe_comm);

    auto data = metrics.Finish();
    int peid = static_cast<int>(pe->id);
    auto cur_pe = CreatePEData(fbb_, peid, data);
    pes_.push_back(cur_pe);
}

void FlatBufferHelper::kp_sample(tw_kp *kp, int inst_type)
{
    // add in kp_comm data
    auto kp_comm = fbb_.CreateVector(kp->kp_comm, g_tw_nkp*tw_nnodes());
    memset(kp->kp_comm, 0, g_tw_nkp*tw_nnodes()*sizeof(int));

    SimEngineMetricsBuilder metrics(fbb_);
    metrics.add_nevent_processed(kp->kp_stats->s_nevent_processed - kp->last_stats[inst_type]->s_nevent_processed);
    metrics.add_nevent_abort(kp->kp_stats->s_nevent_abort - kp->last_stats[inst_type]->s_nevent_abort);
    metrics.add_nevent_rb(kp->kp_stats->s_e_rbs - kp->last_stats[inst_type]->s_e_rbs);
    metrics.add_rb_total(kp->kp_stats->s_rb_total - kp->last_stats[inst_type]->s_rb_total);
    metrics.add_rb_sec(kp->kp_stats->s_rb_secondary - kp->last_stats[inst_type]->s_rb_secondary);
    metrics.add_network_send(kp->kp_stats->s_nsend_network - kp->last_stats[inst_type]->s_nsend_network);
    metrics.add_network_recv(kp->kp_stats->s_nread_network - kp->last_stats[inst_type]->s_nread_network);
    metrics.add_virtual_time_diff(kp->last_time - kp->pe->GVT);
    memcpy(kp->last_stats[inst_type], kp->kp_stats, sizeof(st_kp_stats));
    metrics.add_comm_data(kp_comm);

    auto data = metrics.Finish();
    int peid = static_cast<int>(kp->pe->id);
    int kpid = static_cast<int>(kp->id);
    int kp_gid = peid * g_tw_nkp + kpid;
    auto cur_kp = CreateKPData(fbb_, peid, kpid, kp_gid, data);
    kps_.push_back(cur_kp);
}

void FlatBufferHelper::lp_sample(tw_lp *lp, int inst_type)
{
    // add in lp_comm data
    //auto lp_comm = fbb_.CreateVector(lp->lp_comm, g_tw_total_lps);
    //memset(lp->lp_comm, 0, g_tw_total_lps*sizeof(int));

    SimEngineMetricsBuilder metrics(fbb_);
    metrics.add_nevent_processed(lp->lp_stats->s_nevent_processed - lp->last_stats[inst_type]->s_nevent_processed);
    metrics.add_nevent_abort(lp->lp_stats->s_nevent_abort - lp->last_stats[inst_type]->s_nevent_abort);
    metrics.add_nevent_rb(lp->lp_stats->s_e_rbs - lp->last_stats[inst_type]->s_e_rbs);
    metrics.add_network_send(lp->lp_stats->s_nsend_network - lp->last_stats[inst_type]->s_nsend_network);
    metrics.add_network_recv(lp->lp_stats->s_nread_network - lp->last_stats[inst_type]->s_nread_network);
    memcpy(lp->last_stats[inst_type], lp->lp_stats, sizeof(st_lp_stats));
    //metrics.add_comm_data(lp_comm);

    auto data = metrics.Finish();
    int peid = static_cast<int>(lp->pe->id);
    int kpid = static_cast<int>(lp->kp->id);
    int kp_gid = peid * g_tw_nkp + kpid;
    auto cur_lp = CreateLPData(fbb_, peid, kpid, kp_gid, lp->id, lp->gid, data);
    lps_.push_back(cur_lp);
}

void FlatBufferHelper::start_sample(double vts, double rts, double gvt, InstMode mode)
{
    start_sample(vts, rts, gvt, mode, g_tw_mynode, -1);
}

void FlatBufferHelper::start_sample(double vts, double rts, double gvt,
        InstMode mode, int entity_id, int event_id)
{
    // new sample, so just make sure we don't have any lingering data from previous sample
    fbb_.Clear();
    pes_.clear();
    kps_.clear();
    lps_.clear();
    model_lps_.clear();
    cur_lp_vars_.clear();
    virtual_ts_ = vts;
    real_ts_ = rts;
    gvt_ = gvt;
    mode_ = mode;
    entity_id_ = entity_id;
    event_id_ = event_id;
}

uint8_t *FlatBufferHelper::finish_sample(size_t* offset)
{
    int block = 0;
    auto ds = CreateDamarisDataSampleDirect(fbb_, virtual_ts_, real_ts_, gvt_, mode_,
            &pes_, &kps_, &lps_, &model_lps_, entity_id_, event_id_);
    fbb_.Finish(ds);

    // now get the pointer and size and expose to Damaris
    uint8_t *buf;
    uint8_t *raw;
    int size = fbb_.GetSize();
    if (offset == NULL)
    {
        buf = fbb_.GetBufferPointer();
    }
    else
    {
        size_t tmp;
        raw = fbb_.ReleaseRaw(tmp, *offset);
        buf = &raw[*offset];
    }
    int err;
    // I think damaris parameters may not actually use the default value that you can supposedly set in the XML file...
    // OR damaris doesn't like it when the amount we write is not exactly the same size as we tell it
    // but when we use char* with damaris for paths, that isn't the case
    //if (size > max_sample_size_)
    {
        max_sample_size_ = size;
        //if ((err = damaris_parameter_set("sample_size", &max_sample_size_, sizeof(max_sample_size_))) != DAMARIS_OK)
        //    st_damaris_error(TW_LOC, err, "sample_size");
    }

    //cout << "size of model fb " << size << " max_sample_size_ " << max_sample_size_ << endl;
    if (mode_ == InstMode_RT)
    {
        block = rt_block_;
        rt_block_++;
    }
    if (mode_ == InstMode_VT)
    {
        block = vt_block_;
        vt_block_++;
    }

    //if ((err = damaris_write_block("ross/sample_size", block, &size)) != DAMARIS_OK)
    //    st_damaris_error(TW_LOC, err, "ross/sample_size");
    //cout << "PE " << g_tw_mynode << ": writing to damaris ross/sample to block " << block << endl;
    if ((err = damaris_write_block("ross/sample", block, &buf[0])) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, "ross/sample");

    if (offset == NULL)
        return NULL;
    else
        return raw;
}

void FlatBufferHelper::set_rc_data(uint8_t *raw, size_t offset)
{
    cur_rc_raw_ = raw;
    uint8_t *data = &raw[offset];
    cur_rc_fb_ = GetDamarisDataSample(data);
}

void FlatBufferHelper::delete_rc_data()
{
    cur_rc_fb_ = NULL;
    // since we have this data, we know we stole ownership of it, so we need to delete
    delete[] cur_rc_raw_;
}

void FlatBufferHelper::reset_block_counters()
{
    //std::cout << "rt_block_ = " << rt_block_ << "\nvt_block_ = " << vt_block_ << endl;
    rt_block_ = 0;
    vt_block_ = 0;
}

template <>
fb::Offset<void> FlatBufferHelper::get_variable_offset<int>(fb::Offset<fb::Vector<int>> data_vec)
{
    return CreateIntVar(fbb_, data_vec).Union();
}

template <>
fb::Offset<void> FlatBufferHelper::get_variable_offset<long>(fb::Offset<fb::Vector<long>> data_vec)
{
    return CreateLongVar(fbb_, data_vec).Union();
}

template <>
fb::Offset<void> FlatBufferHelper::get_variable_offset<float>(fb::Offset<fb::Vector<float>> data_vec)
{
    return CreateFloatVar(fbb_, data_vec).Union();
}

template <>
fb::Offset<void> FlatBufferHelper::get_variable_offset<double>(fb::Offset<fb::Vector<double>> data_vec)
{
    return CreateDoubleVar(fbb_, data_vec).Union();
}

template <>
VariableType FlatBufferHelper::get_var_value_type<int>()
{
    return VariableType_IntVar;
}

template <>
VariableType FlatBufferHelper::get_var_value_type<long>()
{
    return VariableType_LongVar;
}

template <>
VariableType FlatBufferHelper::get_var_value_type<float>()
{
    return VariableType_FloatVar;
}

template <>
VariableType FlatBufferHelper::get_var_value_type<double>()
{
    return VariableType_DoubleVar;
}

template <>
const int* FlatBufferHelper::get_variable_data<int>(const ModelVariable *v, size_t *num_elements)
{
    auto var_value = v->var_value_as_IntVar()->value();
    *num_elements = var_value->size();
    return var_value->data();
}

template <>
const long* FlatBufferHelper::get_variable_data<long>(const ModelVariable *v, size_t *num_elements)
{
    auto var_value = v->var_value_as_LongVar()->value();
    *num_elements = var_value->size();
    return var_value->data();
}

template <>
const float* FlatBufferHelper::get_variable_data<float>(const ModelVariable *v, size_t *num_elements)
{
    auto var_value = v->var_value_as_FloatVar()->value();
    *num_elements = var_value->size();
    return var_value->data();
}

template <>
const double* FlatBufferHelper::get_variable_data<double>(const ModelVariable *v, size_t *num_elements)
{
    auto var_value = v->var_value_as_DoubleVar()->value();
    *num_elements = var_value->size();
    return var_value->data();
}

} // end namespace util
} // end namespace ross_damaris
