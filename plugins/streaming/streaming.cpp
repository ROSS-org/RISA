#include "damaris/data/VariableManager.hpp"
#include "damaris/data/Variable.hpp"
#include <iostream>
#include <fstream>
#include <boost/asio.hpp>

#include <plugins/streaming/stream-client.h>
#include <plugins/util/fb-util.h>
#include <plugins/util/damaris-util.h>
#include <plugins/util/sim-config.h>

using namespace ross_damaris;
using namespace ross_damaris::sample;
using namespace ross_damaris::streaming;
using namespace ross_damaris::util;


extern "C" {

void pe_data_to_fb(flatbuffers::FlatBufferBuilder& builder, int32_t step,
        std::vector<flatbuffers::Offset<PEData>>& pe_data);
void kp_data_to_fb(flatbuffers::FlatBufferBuilder& builder, int32_t step,
        std::vector<flatbuffers::Offset<KPData>>& kp_data);
void lp_data_to_fb(flatbuffers::FlatBufferBuilder& builder, int32_t step,
        std::vector<flatbuffers::Offset<LPData>>& lp_data);

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
    cout << "setup_simulation_config() step " << step << endl;

    sim_config.num_pe = damaris::Environment::CountTotalClients();
    for (int i = 0; i < sim_config.num_pe; i++)
    {
        auto val = *(static_cast<int*>(DUtil::get_value_from_damaris("ross/nlp", i, 0, 0)));
        sim_config.num_lp.push_back(val);
    }
    sim_config.kp_per_pe = *(static_cast<int*>(DUtil::get_value_from_damaris("ross/nkp", 1, 0, 0)));

    auto opts = set_options();
    po::variables_map vars;
    string config_file = &((char *)DUtil::get_value_from_damaris("ross/inst_config", 0, 0, 0))[0];
    ifstream ifs(config_file.c_str());
    parse_file(ifs, opts, vars);
    sim_config.set_parameters(vars);
    sim_config.print_parameters();

    if (sim_config.write_data)
        data_file.open("test-fb.bin", ios::out | ios::trunc | ios::binary);

    if (sim_config.stream_data)
    {
        cout << "Attempting to setup connection to " << sim_config.stream_addr << ":" << sim_config.stream_port << endl;
        tcp::resolver::query q(sim_config.stream_addr, sim_config.stream_port);
        auto it = resolver.resolve(q);
        client = new StreamClient(service, it);
        t = new std::thread([](){ service.run(); });
    }

    //model_config.model_setup();
    //model_config.print_model_info();
    cout << "end of setup_simulation_config()\n";
}

/**
 * @brief Damaris event to either stream data or write to file
 *
 * First converts all data in a given sample into Flatbuffers format.
 * Call with "group" scope.
 */
// TODO need to appropriately check for data depending on the type of instrumentation being performed
void handle_data(const std::string& event, int32_t src, int32_t step, const char* args)
{
    (void) event;
    (void) src;
    (void) args;
    step--;
    cout << "handle_data() rank " << src << " step " << step << endl;
    flatbuffers::FlatBufferBuilder *builder = new flatbuffers::FlatBufferBuilder();

    // setup sim engine data tables and model tables as needed
    std::vector<flatbuffers::Offset<PEData>> pe_data;
    std::vector<flatbuffers::Offset<KPData>> kp_data;
    std::vector<flatbuffers::Offset<LPData>> lp_data;
    if (sim_config.pe_data)
        pe_data_to_fb(*builder, step, pe_data);
    if (sim_config.kp_data)
        kp_data_to_fb(*builder, step, kp_data);
    if (sim_config.lp_data)
        lp_data_to_fb(*builder, step, lp_data);

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
        flatbuffers::uoffset_t size = builder->GetSize();
        data_file.write(reinterpret_cast<const char*>(&size), sizeof(size));
        data_file.write(reinterpret_cast<const char*>(buf), size);
        //cout << "wrote " << size << " bytes to file" << endl;
    }

    if (sim_config.stream_data)
        client->write(builder);
}

// just testing out the flatbuffers model data in ross and transferring to damaris as a binary buffer
// TODO need to handle multiple samples per damaris iteration (for RT and VT sampling modes)
void handle_model_data(const std::string& event, int32_t src, int32_t step, const char* args)
{
    step--;
    cout << "handle_data() rank " << src << " step " << step << endl;

    DamarisDataSampleT combined_sample;
    DamarisDataSampleT ds;
    bool data_found = false;
    for (int peid = 0; peid < sim_config.num_pe; peid++ )
    {
        char *binary_data = static_cast<char*>(DUtil::get_value_from_damaris("ross/sample", peid, step, 0));
        if (binary_data)
        {
            data_found = true;
            auto data_sample = GetDamarisDataSample(binary_data);
            if (peid == 0)
            {
                data_sample->UnPackTo(&combined_sample);
                continue;
            }

            data_sample->UnPackTo(&ds);
            //combined_sample.model_data.insert(combined_sample.model_data.end(), ds.model_data.begin(), ds.model_data.end());
            std::move(ds.pe_data.begin(), ds.pe_data.end(), std::back_inserter(combined_sample.pe_data));
            std::move(ds.kp_data.begin(), ds.kp_data.end(), std::back_inserter(combined_sample.kp_data));
            std::move(ds.lp_data.begin(), ds.lp_data.end(), std::back_inserter(combined_sample.lp_data));
            std::move(ds.model_data.begin(), ds.model_data.end(), std::back_inserter(combined_sample.model_data));
        }
    }

    if (data_found)
    {
        // TODO you can use your own memory allocator with flatbufferbuilder
        // as is, it will dynamically allocate memory
        // Since we already have the data in a flatbuffer compatible format, let
        // the flatbufferbuilder become the owner of this buffer?
        // actually no, we're having to combine flatbuffers anyway
        flatbuffers::FlatBufferBuilder fbb;
        auto new_samp = DamarisDataSample::Pack(fbb, &combined_sample);
        fbb.Finish(new_samp);
        //auto s = flatbuffers::FlatBufferToString(fbb.GetBufferPointer(), DamarisDataSampleTypeTable(), true);
        //cout << s << endl;
        if (sim_config.write_data)
        {
            uint8_t *buf = fbb.GetBufferPointer();
            flatbuffers::uoffset_t size = fbb.GetSize();
            data_file.write(reinterpret_cast<const char*>(&size), sizeof(size));
            data_file.write(reinterpret_cast<const char*>(buf), size);
            //cout << "wrote " << size << " bytes to file" << endl;
        }
    }
}

/**
 * @brief Damaris event to close files, sockets, etc
 *
 * Called just before Damaris is stopped.
 * Called with "group" scope.
 */
void streaming_finalize(const std::string& event, int32_t src, int32_t step, const char* args)
{
    cout << "streaming_finalize()\n";
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

void pe_data_to_fb(flatbuffers::FlatBufferBuilder& builder, int32_t step,
        std::vector<flatbuffers::Offset<PEData>>& pe_data)
{
    std::string var_prefix = "ross/pes/gvt_inst/";
    for (int peid = 0; peid < sim_config.num_pe; peid++)
    {
        auto metrics = sim_engine_metrics_to_fb(builder, peid, step, 1, var_prefix);
        pe_data.push_back(CreatePEData(builder, peid, metrics[0])); // for PEs, it will always be metrics[0]
    }
}

void kp_data_to_fb(flatbuffers::FlatBufferBuilder& builder, int32_t step,
        std::vector<flatbuffers::Offset<KPData>>& kp_data)
{
    std::string var_prefix = "ross/kps/gvt_inst/";
    for (int peid = 0; peid < sim_config.num_pe; peid++)
    {
        auto metrics = sim_engine_metrics_to_fb(builder, peid, step, sim_config.kp_per_pe, var_prefix);
        for (int kpid = 0; kpid < sim_config.kp_per_pe; kpid++)
        {
            int kp_gid = (peid * sim_config.kp_per_pe) + kpid;
            kp_data.push_back(CreateKPData(builder, peid, kpid, kp_gid, metrics[kpid]));

        }
    }
}

void lp_data_to_fb(flatbuffers::FlatBufferBuilder& builder, int32_t step,
        std::vector<flatbuffers::Offset<LPData>>& lp_data)
{
    std::string var_prefix = "ross/lps/gvt_inst/";
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
}

}
