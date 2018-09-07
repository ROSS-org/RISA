#include "damaris/data/VariableManager.hpp"
#include "damaris/data/Variable.hpp"
#include <iostream>
#include <fstream>
#include <boost/asio.hpp>

#include "stream-client.h"
#include "fb-util.h"
#include "damaris-util.h"
#include "config.h"
#include "sim-config.h"

using namespace ross_damaris;
using namespace ross_damaris::sample;
using namespace ross_damaris::streaming;
using namespace ross_damaris::util;


extern "C" {

std::vector<flatbuffers::Offset<PEData>> pe_data_to_fb(
        flatbuffers::FlatBufferBuilder& builder, int32_t step);
std::vector<flatbuffers::Offset<KPData>> kp_data_to_fb(
        flatbuffers::FlatBufferBuilder& builder, int32_t step);
std::vector<flatbuffers::Offset<LPData>> lp_data_to_fb(
        flatbuffers::FlatBufferBuilder& builder, int32_t step);

SimConfig sim_config;
ModelConfig model_config;
ofstream data_file;
boost::asio::io_service service;
tcp::resolver resolver(service);
StreamClient *client;
std::thread *t; // thread is to handle boost async IO operations

/** 
 * @brief Damaris event to set up simulation configuration
 *
 * Should be set in XML file as "group" scope
 */
void setup_simulation_config(const std::string& event, int32_t src, int32_t step, const char* args)
{
    (void) event;
    (void) src;
    (void) step;
    (void) args;
    //cout << "setup_simulation_config() step " << step << endl;

    sim_config.num_pe = damaris::Environment::CountTotalClients();
    for (int i = 0; i < sim_config.num_pe; i++)
    {
        auto val = *(static_cast<int*>(DUtil::get_value_from_damaris("ross/nlp", i, 0, 0)));
        sim_config.num_lp.push_back(val);
    }
    sim_config.kp_per_pe = *(static_cast<int*>(DUtil::get_value_from_damaris("ross/nkp", 1, 0, 0)));

    auto opts = set_options();
    po::variables_map vars;
    string config_file = "/home/rossc3/rv-build/models/phold/test.cfg";
    ifstream ifs(config_file.c_str());
    parse_file(ifs, opts, vars);
    sim_config.set_parameters(vars);
    sim_config.print_parameters();

    if (sim_config.write_data)
        data_file.open("test-fb.bin", ios::out | ios::trunc | ios::binary);

    if (sim_config.stream_data)
    {
        tcp::resolver::query q("localhost", "8000");
        auto it = resolver.resolve(q);
        client = new StreamClient(service, it);
        t = new std::thread([](){ service.run(); });
    }

    model_config.model_setup();
    //model_config.print_model_info();
}

/**
 * @brief Damaris event to either stream data or write to file
 *
 * First converts all data in a given sample into Flatbuffers format.
 * Call with "group" scope.
 */
void handle_data(const std::string& event, int32_t src, int32_t step, const char* args)
{
    (void) event;
    (void) src;
    (void) args;
    step--;
    //cout << "write_data() rank " << src << " step " << step << endl;
    flatbuffers::FlatBufferBuilder *builder = new flatbuffers::FlatBufferBuilder();

    // setup sim engine data tables and model tables as needed
    auto pe_data = pe_data_to_fb(*builder, step);
    auto kp_data = kp_data_to_fb(*builder, step);
    auto lp_data = lp_data_to_fb(*builder, step);

    // then setup the DamarisDataSample table
    double virtual_ts = 0.0; // placeholder for now
    double real_ts =*(static_cast<double*>(DUtil::get_value_from_damaris("ross/real_time", 0, step, 0)));
    double last_gvt = *(static_cast<double*>(DUtil::get_value_from_damaris("ross/last_gvt", 0, step, 0)));
    auto data_sample = CreateDamarisDataSampleDirect(*builder, virtual_ts, real_ts, last_gvt, InstMode_GVT, &pe_data, &kp_data, &lp_data);
    
    builder->Finish(data_sample);

	//auto s = flatbuffers::FlatBufferToString(builder.GetBufferPointer(), DamarisDataSampleTypeTable(), true);
	//cout << "current sample:\n" << s << endl;
    if (sim_config.write_data)
    {
        // Get pointer to the buffer and the size for writing to file
        uint8_t *buf = builder->GetBufferPointer();
        int size = builder->GetSize();
        data_file.write(reinterpret_cast<const char*>(&size), sizeof(size));
        data_file.write(reinterpret_cast<const char*>(buf), size);
        //cout << "wrote " << size << " bytes to file" << endl;
    }

    if (sim_config.stream_data)
        client->write(builder);
}

/**
 * @brief Damaris event to close files, sockets, etc
 *
 * Called just before Damaris is stopped.
 * Called with "group" scope.
 */
void streaming_finalize(const std::string& event, int32_t src, int32_t step, const char* args)
{
    (void) event;
    (void) src;
    (void) step;
    (void) args;
    if (sim_config.write_data)
        data_file.close();

    if (sim_config.stream_data)
    {
        client->close();
        t->join();
    }
}

std::vector<flatbuffers::Offset<SimEngineMetrics>> sim_engine_metrics_to_fb(flatbuffers::FlatBufferBuilder& builder,
        int32_t src, int32_t step, int32_t num_entities, const std::string& var_prefix)
{
    std::vector<flatbuffers::Offset<SimEngineMetrics>> data;
    SimEngineMetricsT *metrics_objects = new SimEngineMetricsT[num_entities];
    FBUtil::collect_metrics(&metrics_objects[0], var_prefix, src, step, 0, num_entities);
    for (int id = 0; id < num_entities; id++)
    {
        auto metrics = CreateSimEngineMetrics(builder, &metrics_objects[id]);
        data.push_back(metrics);
    }
    delete[] metrics_objects;
    return data;
}

std::vector<flatbuffers::Offset<PEData>> pe_data_to_fb(
        flatbuffers::FlatBufferBuilder& builder, int32_t step)
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
        flatbuffers::FlatBufferBuilder& builder, int32_t step)
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
        flatbuffers::FlatBufferBuilder& builder, int32_t step)
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
