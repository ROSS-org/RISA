#include "sim-config.h"
#include "flatbuffers/minireflect.h"
#include "damaris/data/VariableManager.hpp"
#include <iostream>
#include <fstream>

using namespace ross_damaris;
using namespace ross_damaris::sample;
using namespace damaris;

// return the pointer to the variable, because it may be a single value or an array
template <typename T>
T *get_value_from_damaris(const std::string& varname, int32_t src, int32_t step, int32_t block_id)
{
    boost::shared_ptr<Variable> v = VariableManager::Search(varname);
    if (v && v->GetBlock(src, step, block_id))
        return (T*)v->GetBlock(src, step, block_id)->GetDataSpace().GetData();
    else
    {
        cout << "ERROR in get_value_from_damaris() for variable " << varname << endl;
        return nullptr;
    }
}

template <typename T>
void add_metric(SimEngineMetricsBuilder& metric_builder, const std::string& var_name, T value)
{
    if (var_name.compare("nevent_processed") == 0)
        metric_builder.add_nevent_processed(value);
    else if (var_name.compare("nevent_abort") == 0)
        metric_builder.add_nevent_abort(value);
    else if (var_name.compare("nevent_rb") == 0)
        metric_builder.add_nevent_rb(value);
    else if (var_name.compare("rb_total") == 0)
        metric_builder.add_rb_total(value);
    else if (var_name.compare("rb_prim") == 0)
        metric_builder.add_rb_prim(value);
    else if (var_name.compare("rb_sec") == 0)
        metric_builder.add_rb_sec(value);
    else if (var_name.compare("fc_attempts") == 0)
        metric_builder.add_fc_attempts(value);
    else if (var_name.compare("pq_qsize") == 0)
        metric_builder.add_pq_qsize(value);
    else if (var_name.compare("network_send") == 0)
        metric_builder.add_network_send(value);
    else if (var_name.compare("network_recv") == 0)
        metric_builder.add_network_recv(value);
    else if (var_name.compare("num_gvt") == 0)
        metric_builder.add_num_gvt(value);
    else if (var_name.compare("event_ties") == 0)
        metric_builder.add_event_ties(value);
    else if (var_name.compare("efficiency") == 0)
        metric_builder.add_efficiency(value);
    else if (var_name.compare("net_read_time") == 0)
        metric_builder.add_net_read_time(value);
    else if (var_name.compare("net_other_time") == 0)
        metric_builder.add_net_other_time(value);
    else if (var_name.compare("gvt_time") == 0)
        metric_builder.add_gvt_time(value);
    else if (var_name.compare("fc_time") == 0)
        metric_builder.add_fc_time(value);
    else if (var_name.compare("event_abort_time") == 0)
        metric_builder.add_event_abort_time(value);
    else if (var_name.compare("event_proc_time") == 0)
        metric_builder.add_event_proc_time(value);
    else if (var_name.compare("pq_time") == 0)
        metric_builder.add_pq_time(value);
    else if (var_name.compare("rb_time") == 0)
        metric_builder.add_rb_time(value);
    else if (var_name.compare("cancel_q_time") == 0)
        metric_builder.add_cancel_q_time(value);
    else if (var_name.compare("avl_time") == 0)
        metric_builder.add_avl_time(value);
    else if (var_name.compare("virtual_time_diff") == 0)
        metric_builder.add_virtual_time_diff(value);
    else
        cout << "ERROR in get_metric_fn_ptr() var " << var_name << "not found!" << endl;
}

void put_metric_in_buffer(SimEngineMetricsBuilder& metric_builder, flatbuffers::ElementaryType type,
        const std::string& var_name, const std::string& var_prefix, int32_t src, int32_t step, int32_t block_id)
{
    switch (type)
    {
        case flatbuffers::ET_INT:
        {
            auto *val = get_value_from_damaris<int>(var_prefix + var_name, src, step, 0);
            if (val)
                add_metric(metric_builder, var_name, *val);
            break;
        }
        case flatbuffers::ET_FLOAT:
        {
            auto *val = get_value_from_damaris<float>(var_prefix + var_name, src, step, 0);
            if (val)
                add_metric(metric_builder, var_name, *val);
            break;
        }
        case flatbuffers::ET_DOUBLE:
        {
            auto *val = get_value_from_damaris<double>(var_prefix + var_name, src, step, 0);
            if (val)
                add_metric(metric_builder, var_name, *val);
            break;
        }
        default:
        {
            cout << "put_metric_in_buffer(): this type hasn't been implemented!" << endl;
            break;
        }
    }

}

// TODO need setup event to get sim configuration options
SimConfig sim_config;

extern "C" {

std::vector<flatbuffers::Offset<PEData>> pe_data_to_fb(
        flatbuffers::FlatBufferBuilder& builder, int32_t src, int32_t step, const std::string& var_prefix);

ofstream data_file;
// only call once, not per source
void setup_simulation_config(const std::string& event, int32_t src, int32_t step, const char* args)
{
    cout << "setup_simulation_config() step " << step << endl;

    sim_config.num_pe = Environment::CountTotalClients();
    for (int i = 0; i < sim_config.num_pe; i++)
    {
        auto val = *get_value_from_damaris<int>("ross/nlp", i, 0, 0);
        sim_config.num_lp.push_back(val);
    }
    sim_config.kp_per_pe = *get_value_from_damaris<int>("ross/nkp", 0, 0, 0);

    int *inst_modes_sim = get_value_from_damaris<int>("ross/inst_modes_sim", 0, 0, 0);
    int *inst_modes_model = get_value_from_damaris<int>("ross/inst_modes_model", 0, 0, 0);
    for (int i = 0; i < InstMode_MAX; i++)
    {
        sim_config.inst_mode_sim[i] = inst_modes_sim[i];
        sim_config.inst_mode_model[i] = inst_modes_model[i];
    }
    data_file.open("test-fb.bin", ios::out | ios::trunc | ios::binary);
}

void write_data(const std::string& event, int32_t src, int32_t step, const char* args)
{
    step--;
    cout << "write_data() rank " << src << " step " << step << endl;
    flatbuffers::FlatBufferBuilder builder;
    std::string var_prefix = "ross/pes/gvt_inst/";

    // setup sim engine data tables and model tables as needed
    auto pe_data = pe_data_to_fb(builder, src, step, var_prefix);

    // then setup the DamarisDataSample table
    //DamarisDataSampleT data_sample;
    //double virtual_ts = *get_value_from_damaris<double>("ross/virtual_ts", 0, step, 0);
    double virtual_ts = 0.0; // placeholder for now
    double real_ts = *get_value_from_damaris<double>("ross/real_time", 0, step, 0);
    double last_gvt = *get_value_from_damaris<double>("ross/last_gvt", 0, step, 0);
    auto data_sample = CreateDamarisDataSampleDirect(builder, virtual_ts, real_ts, last_gvt, InstMode_GVT, &pe_data);
    
    builder.Finish(data_sample);

	//auto s = flatbuffers::FlatBufferToString(builder.GetBufferPointer(), DamarisDataSampleTypeTable());
	//cout << "current sample:\n" << s << endl;
    // Get pointer to the buffer and the size for writing to file
    uint8_t *buf = builder.GetBufferPointer();
    int size = builder.GetSize();
	data_file.write(reinterpret_cast<const char*>(&size), sizeof(size));
    data_file.write(reinterpret_cast<const char*>(buf), size);
	//cout << "wrote " << size + sizeof(size) << " bytes to file" << endl;
}

void streaming_finalize(const std::string& event, int32_t src, int32_t step, const char* args)
{
    data_file.close();
}

flatbuffers::Offset<SimEngineMetrics> sim_engine_metrics_to_fb(flatbuffers::FlatBufferBuilder& builder,
        int32_t src, int32_t step, const std::string& var_prefix)
{
    SimEngineMetricsBuilder metric_builder(builder);
    const flatbuffers::TypeTable *tt = SimEngineMetricsTypeTable();
    for (int i = 0; i < tt->num_elems; i++)
        put_metric_in_buffer(metric_builder, static_cast<flatbuffers::ElementaryType>(tt->type_codes[i].base_type),
                tt->names[i], var_prefix, src, step, 0);
    return metric_builder.Finish();
}

std::vector<flatbuffers::Offset<PEData>> pe_data_to_fb(
        flatbuffers::FlatBufferBuilder& builder, int32_t src, int32_t step, const std::string& var_prefix)
{
    std::vector<flatbuffers::Offset<PEData>> pe_data;
    std::vector<flatbuffers::Offset<SimEngineMetrics>> engine_data;
    for (int i = 0; i < sim_config.num_pe; i++)
    {
        auto metrics = sim_engine_metrics_to_fb(builder, src, step, var_prefix);
        pe_data.push_back(CreatePEData(builder, i, metrics));
    }

    return pe_data;
}

//flatbuffers::Offset<SimEngineKP> kp_data_to_fb(flatbuffers::FlatBufferBuilder& builder, int32_t src, int32_t step, const std::string& var_prefix)
//{
//
//}
//
//flatbuffers::Offset<SimEngineLP> lp_data_to_fb(flatbuffers::FlatBufferBuilder& builder, int32_t src, int32_t step, const std::string& var_prefix)
//{
//
//}

}
