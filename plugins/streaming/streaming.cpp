#include "sim-config.h"
#include "flatbuffers/minireflect.h"
#include "damaris/data/VariableManager.hpp"
#include <iostream>
#include <fstream>
#include <boost/asio.hpp>
#include "stream-client.h"

using namespace ross_damaris;
using namespace ross_damaris::sample;
using namespace damaris_streaming;
using namespace damaris;

// return the pointer to the variable, because it may be a single value or an array
void *get_value_from_damaris(const std::string& varname, int32_t src, int32_t step, int32_t block_id)
{
    boost::shared_ptr<Variable> v = VariableManager::Search(varname);
    if (v && v->GetBlock(src, step, block_id))
        return v->GetBlock(src, step, block_id)->GetDataSpace().GetData();
    else
    {
        //cout << "ERROR in get_value_from_damaris() for variable " << varname << endl;
        return nullptr;
    }
}

template <typename T>
void add_metric(SimEngineMetricsT *obj, const std::string& var_name, T value)
{
    if (var_name.compare("nevent_processed") == 0)
        obj->nevent_processed = value;
    else if (var_name.compare("nevent_abort") == 0)
        obj->nevent_abort = value;
    else if (var_name.compare("nevent_rb") == 0)
        obj->nevent_rb = value;
    else if (var_name.compare("rb_total") == 0)
        obj->rb_total = value;
    else if (var_name.compare("rb_prim") == 0)
        obj->rb_prim = value;
    else if (var_name.compare("rb_sec") == 0)
        obj->rb_sec = value;
    else if (var_name.compare("fc_attempts") == 0)
        obj->fc_attempts = value;
    else if (var_name.compare("pq_qsize") == 0)
        obj->pq_qsize = value;
    else if (var_name.compare("network_send") == 0)
        obj->network_send = value;
    else if (var_name.compare("network_recv") == 0)
        obj->network_recv = value;
    else if (var_name.compare("num_gvt") == 0)
        obj->num_gvt = value;
    else if (var_name.compare("event_ties") == 0)
        obj->event_ties = value;
    else if (var_name.compare("efficiency") == 0)
        obj->efficiency = value;
    else if (var_name.compare("net_read_time") == 0)
        obj->net_read_time = value;
    else if (var_name.compare("net_other_time") == 0)
        obj->net_other_time = value;
    else if (var_name.compare("gvt_time") == 0)
        obj->gvt_time = value;
    else if (var_name.compare("fc_time") == 0)
        obj->fc_time = value;
    else if (var_name.compare("event_abort_time") == 0)
        obj->event_abort_time = value;
    else if (var_name.compare("event_proc_time") == 0)
        obj->event_proc_time = value;
    else if (var_name.compare("pq_time") == 0)
        obj->pq_time = value;
    else if (var_name.compare("rb_time") == 0)
        obj->rb_time = value;
    else if (var_name.compare("cancel_q_time") == 0)
        obj->cancel_q_time = value;
    else if (var_name.compare("avl_time") == 0)
        obj->avl_time = value;
    else if (var_name.compare("virtual_time_diff") == 0)
        obj->virtual_time_diff = value;
    else
        cout << "ERROR in get_metric_fn_ptr() var " << var_name << "not found!" << endl;
}

void collect_metrics(SimEngineMetricsT *metrics_objects, const std::string& var_prefix,
        int32_t src, int32_t step, int32_t block_id, int32_t num_entities)
{
    const flatbuffers::TypeTable *tt = SimEngineMetricsTypeTable();
    for (int i = 0; i < tt->num_elems; i++)
    {
        flatbuffers::ElementaryType type = static_cast<flatbuffers::ElementaryType>(tt->type_codes[i].base_type);
        switch (type)
        {
            case flatbuffers::ET_INT:
            {
                auto *val = static_cast<int*>(get_value_from_damaris(var_prefix + tt->names[i], src, step, 0));
                if (val)
                {
                    for (int j = 0; j < num_entities; j++)
                        add_metric(&metrics_objects[j], tt->names[i], val[j]);
                }
                break;
            }
            case flatbuffers::ET_FLOAT:
            {
                auto *val = static_cast<float*>(get_value_from_damaris(var_prefix + tt->names[i], src, step, 0));
                if (val)
                {
                    for (int j = 0; j < num_entities; j++)
                        add_metric(&metrics_objects[j], tt->names[i], val[j]);
                }
                break;
            }
            case flatbuffers::ET_DOUBLE:
            {
                auto *val = static_cast<double*>(get_value_from_damaris(var_prefix + tt->names[i], src, step, 0));
                if (val)
                {
                    for (int j = 0; j < num_entities; j++)
                        add_metric(&metrics_objects[j], tt->names[i], val[j]);
                }
                break;
            }
            default:
            {
                cout << "collect_metrics(): this type hasn't been implemented!" << endl;
                break;
            }
        }
    }
}

// TODO need setup event to get sim configuration options
SimConfig sim_config;

extern "C" {

std::vector<flatbuffers::Offset<PEData>> pe_data_to_fb(
        flatbuffers::FlatBufferBuilder& builder, int32_t src, int32_t step);
std::vector<flatbuffers::Offset<KPData>> kp_data_to_fb(
        flatbuffers::FlatBufferBuilder& builder, int32_t src, int32_t step);
std::vector<flatbuffers::Offset<LPData>> lp_data_to_fb(
        flatbuffers::FlatBufferBuilder& builder, int32_t src, int32_t step);

ofstream data_file;
boost::asio::io_service service;
tcp::resolver resolver(service);
StreamClient *client;
std::thread *t;

// only call once, not per source
void setup_simulation_config(const std::string& event, int32_t src, int32_t step, const char* args)
{
    //cout << "setup_simulation_config() step " << step << endl;

    sim_config.num_pe = Environment::CountTotalClients();
    for (int i = 0; i < sim_config.num_pe; i++)
    {
        auto val = *(static_cast<int*>(get_value_from_damaris("ross/nlp", i, 0, 0)));
        sim_config.num_lp.push_back(val);
    }
    sim_config.kp_per_pe = *(static_cast<int*>(get_value_from_damaris("ross/nkp", 0, 0, 0)));

    int *inst_modes_sim = static_cast<int*>(get_value_from_damaris("ross/inst_modes_sim", 0, 0, 0));
    int *inst_modes_model = static_cast<int*>(get_value_from_damaris("ross/inst_modes_model", 0, 0, 0));
    for (int i = 0; i < InstMode_MAX; i++)
    {
        sim_config.inst_mode_sim[i] = inst_modes_sim[i];
        sim_config.inst_mode_model[i] = inst_modes_model[i];
    }
    data_file.open("test-fb.bin", ios::out | ios::trunc | ios::binary);

    tcp::resolver::query q("localhost", "8000");
    auto it = resolver.resolve(q);
    client = new StreamClient(service, it);
    service.run();
    //t = new std::thread([](){ service.run(); });
}

void write_data(const std::string& event, int32_t src, int32_t step, const char* args)
{
    step--;
    //cout << "write_data() rank " << src << " step " << step << endl;
    flatbuffers::FlatBufferBuilder builder;

    // setup sim engine data tables and model tables as needed
    auto pe_data = pe_data_to_fb(builder, src, step);
    auto kp_data = kp_data_to_fb(builder, src, step);
    auto lp_data = lp_data_to_fb(builder, src, step);

    // then setup the DamarisDataSample table
    //DamarisDataSampleT data_sample;
    //double virtual_ts = *get_value_from_damaris("ross/virtual_ts", 0, step, 0);
    double virtual_ts = 0.0; // placeholder for now
    double real_ts =*(static_cast<double*>(get_value_from_damaris("ross/real_time", 0, step, 0)));
    double last_gvt = *(static_cast<double*>(get_value_from_damaris("ross/last_gvt", 0, step, 0)));
    auto data_sample = CreateDamarisDataSampleDirect(builder, virtual_ts, real_ts, last_gvt, InstMode_GVT, &pe_data, &kp_data, &lp_data);
    
    builder.Finish(data_sample);

	//auto s = flatbuffers::FlatBufferToString(builder.GetBufferPointer(), DamarisDataSampleTypeTable(), true);
	//cout << "current sample:\n" << s << endl;
    // Get pointer to the buffer and the size for writing to file
    uint8_t *buf = builder.GetBufferPointer();
    int size = builder.GetSize();
	data_file.write(reinterpret_cast<const char*>(&size), sizeof(size));
    data_file.write(reinterpret_cast<const char*>(buf), size);
    client->write(builder);
	//cout << "wrote " << size << " bytes to file" << endl;
}

void streaming_finalize(const std::string& event, int32_t src, int32_t step, const char* args)
{
    data_file.close();
    client->close();
    //t->join();
}

std::vector<flatbuffers::Offset<SimEngineMetrics>> sim_engine_metrics_to_fb(flatbuffers::FlatBufferBuilder& builder,
        int32_t src, int32_t step, int32_t num_entities, const std::string& var_prefix)
{
    std::vector<flatbuffers::Offset<SimEngineMetrics>> data;
    SimEngineMetricsT metrics_objects[num_entities];
    collect_metrics(&metrics_objects[0], var_prefix, src, step, 0, num_entities);
    for (int id = 0; id < num_entities; id++)
    {
        auto metrics = CreateSimEngineMetrics(builder, &metrics_objects[id]);
        data.push_back(metrics);
    }
    return data;
}

std::vector<flatbuffers::Offset<PEData>> pe_data_to_fb(
        flatbuffers::FlatBufferBuilder& builder, int32_t src, int32_t step)
{
    std::string var_prefix = "ross/pes/gvt_inst/";
    std::vector<flatbuffers::Offset<PEData>> pe_data;
    for (int peid = 0; peid < sim_config.num_pe; peid++)
    {
        auto metrics = sim_engine_metrics_to_fb(builder, peid, step, 1, var_prefix);
        pe_data.push_back(CreatePEData(builder, peid, metrics[0])); // for PEs, it will always be metrics[0]
    }

    return pe_data;
}

std::vector<flatbuffers::Offset<KPData>> kp_data_to_fb(
        flatbuffers::FlatBufferBuilder& builder, int32_t src, int32_t step)
{
    std::string var_prefix = "ross/kps/gvt_inst/";
    std::vector<flatbuffers::Offset<KPData>> kp_data;
    for (int peid = 0; peid < sim_config.num_pe; peid++)
    {
        auto metrics = sim_engine_metrics_to_fb(builder, peid, step, sim_config.kp_per_pe, var_prefix);
        for (int kpid = 0; kpid < sim_config.kp_per_pe; kpid++)
        {
            int kp_gid = (peid * sim_config.kp_per_pe) + kpid;
            kp_data.push_back(CreateKPData(builder, peid, kpid, kp_gid, metrics[kpid]));

        }
    }
    return kp_data;
}

std::vector<flatbuffers::Offset<LPData>> lp_data_to_fb(
        flatbuffers::FlatBufferBuilder& builder, int32_t src, int32_t step)
{
    std::string var_prefix = "ross/lps/gvt_inst/";
    std::vector<flatbuffers::Offset<LPData>> lp_data;
    int total_lp = 0;
    for (int peid = 0; peid < sim_config.num_pe; peid++)
    {
        auto metrics = sim_engine_metrics_to_fb(builder, peid, step, sim_config.num_lp[peid], var_prefix);
        for (int lpid = 0; lpid < sim_config.num_lp[peid]; lpid++)
        {
            int kpid = lpid % sim_config.kp_per_pe;
            int kp_gid = (peid * sim_config.kp_per_pe) + kpid;
            int lp_gid = total_lp + lpid;
            lp_data.push_back(CreateLPData(builder, peid, kpid, kp_gid, lpid, lp_gid, metrics[lpid]));

        }
        total_lp += sim_config.num_lp[peid];
    }
    return lp_data;

}

}
