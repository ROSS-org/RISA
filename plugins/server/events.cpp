/**
 * @file events.cpp
 *
 * This file defines all the possible events that Damaris will call for plugins.
 */
#include <plugins/server/server.h>
#include <plugins/flatbuffers/data_sample_generated.h>
#include <plugins/util/damaris-util.h>

#include <damaris/buffer/DataSpace.hpp>
#include <damaris/buffer/Buffer.hpp>

using namespace ross_damaris::server;
using namespace ross_damaris::sample;
using namespace ross_damaris::util;

extern "C" {

RDServer server;

/**
 * @brief Damaris event to set up simulation configuration
 *
 * Should be set in XML file as "group" scope
 */
void damaris_rank_init(const std::string& event, int32_t src, int32_t step, const char* args)
{
    (void) event;
    (void) src;
    (void) step;
    (void) args;
    cout << "damaris_rank_init() step " << step << endl;
    server.setup_simulation_config();
    cout << "end of damaris_rank_init()\n";
}

/**
 * @brief Damaris event called at the end of each iteration
 *
 * Should be set in XML file as "group" scope.
 * Removes data from Damaris control and stores it in our own boost multi-index.
 */
void damaris_rank_end_iteration(const std::string& event, int32_t src, int32_t step, const char* args)
{
    step--;
    cout << "damaris_rank_end_iteration() rank " << src << " step " << step << endl;

    damaris::BlocksByIteration::iterator begin, end;
    DUtil::get_damaris_iterators("ross/sample", step, begin, end);

    while (begin != end)
    {
        server.process_sample(*begin);

        begin++;
    }
    server.forward_data();

    //for (int peid = 0; peid < sim_config.num_pe; peid++ )
    //{
    //    char *binary_data = static_cast<char*>(DUtil::get_value_from_damaris("ross/sample", peid, step, 0));
    //}

    //if (data_found)
    //{
    //    // TODO you can use your own memory allocator with flatbufferbuilder
    //    // as is, it will dynamically allocate memory
    //    // Since we already have the data in a flatbuffer compatible format, let
    //    // the flatbufferbuilder become the owner of this buffer?
    //    // actually no, we're having to combine flatbuffers anyway
    //    flatbuffers::FlatBufferBuilder fbb;
    //    auto new_samp = DamarisDataSample::Pack(fbb, &combined_sample);
    //    fbb.Finish(new_samp);
    //    //auto s = flatbuffers::FlatBufferToString(fbb.GetBufferPointer(), DamarisDataSampleTypeTable(), true);
    //    //cout << s << endl;
    //    if (sim_config.write_data)
    //    {
    //        uint8_t *buf = fbb.GetBufferPointer();
    //        flatbuffers::uoffset_t size = fbb.GetSize();
    //        data_file.write(reinterpret_cast<const char*>(&size), sizeof(size));
    //        data_file.write(reinterpret_cast<const char*>(buf), size);
    //        //cout << "wrote " << size << " bytes to file" << endl;
    //    }
    //}
}

/**
 * @brief Damaris event to close files, sockets, etc
 *
 * Called just before Damaris is stopped.
 * Called with "group" scope.
 */
void damaris_rank_finalize(const std::string& event, int32_t src, int32_t step, const char* args)
{
    (void) event;
    (void) src;
    (void) step;
    (void) args;
    cout << "damaris_rank_finalize() step " << step << endl;
    server.finalize();
    cout << "end of damaris_rank_finalize()\n";
}


/**
 * @brief Damaris event to either stream data or write to file
 *
 * First converts all data in a given sample into Flatbuffers format.
 * Call with "group" scope.
 */
// TODO need to appropriately check for data depending on the type of instrumentation being performed
// This is old version, where data is not already in flatbuffers format
//void handle_data(const std::string& event, int32_t src, int32_t step, const char* args)
//{
//    (void) event;
//    (void) src;
//    (void) args;
//    step--;
//    cout << "handle_data() rank " << src << " step " << step << endl;
//    flatbuffers::FlatBufferBuilder *builder = new flatbuffers::FlatBufferBuilder();
//
//    // setup sim engine data tables and model tables as needed
//    std::vector<flatbuffers::Offset<PEData>> pe_data;
//    std::vector<flatbuffers::Offset<KPData>> kp_data;
//    std::vector<flatbuffers::Offset<LPData>> lp_data;
//    if (sim_config.pe_data)
//        pe_data_to_fb(*builder, step, pe_data);
//    if (sim_config.kp_data)
//        kp_data_to_fb(*builder, step, kp_data);
//    if (sim_config.lp_data)
//        lp_data_to_fb(*builder, step, lp_data);
//
//    // then setup the DamarisDataSample table
//    double virtual_ts = 0.0; // placeholder for now
//    double real_ts =*(static_cast<double*>(DUtil::get_value_from_damaris("ross/real_time", 0, step, 0)));
//    double last_gvt = *(static_cast<double*>(DUtil::get_value_from_damaris("ross/last_gvt", 0, step, 0)));
//    auto data_sample = CreateDamarisDataSampleDirect(*builder, virtual_ts, real_ts, last_gvt, InstMode_GVT, &pe_data, &kp_data, &lp_data);
//
//    builder->Finish(data_sample);
//
//	//auto s = flatbuffers::FlatBufferToString(builder.GetBufferPointer(), DamarisDataSampleTypeTable(), true);
//	//cout << "current sample:\n" << s << endl;
//    if (sim_config.write_data)
//    {
//        // Get pointer to the buffer and the size for writing to file
//        uint8_t *buf = builder->GetBufferPointer();
//        flatbuffers::uoffset_t size = builder->GetSize();
//        data_file.write(reinterpret_cast<const char*>(&size), sizeof(size));
//        data_file.write(reinterpret_cast<const char*>(buf), size);
//        //cout << "wrote " << size << " bytes to file" << endl;
//    }
//
//    if (sim_config.stream_data)
//        client->write(builder);
//}

// just testing out the flatbuffers model data in ross and transferring to damaris as a binary buffer
// TODO need to handle multiple samples per damaris iteration (for RT and VT sampling modes)
//void handle_model_data(const std::string& event, int32_t src, int32_t step, const char* args)
//{
//    step--;
//    cout << "handle_data() rank " << src << " step " << step << endl;
//
//    DamarisDataSampleT combined_sample;
//    DamarisDataSampleT ds;
//    bool data_found = false;
//    for (int peid = 0; peid < sim_config.num_pe; peid++ )
//    {
//        char *binary_data = static_cast<char*>(DUtil::get_value_from_damaris("ross/sample", peid, step, 0));
//        if (binary_data)
//        {
//            data_found = true;
//            auto data_sample = GetDamarisDataSample(binary_data);
//            if (peid == 0)
//            {
//                data_sample->UnPackTo(&combined_sample);
//                continue;
//            }
//
//            data_sample->UnPackTo(&ds);
//            //combined_sample.model_data.insert(combined_sample.model_data.end(), ds.model_data.begin(), ds.model_data.end());
//            std::move(ds.pe_data.begin(), ds.pe_data.end(), std::back_inserter(combined_sample.pe_data));
//            std::move(ds.kp_data.begin(), ds.kp_data.end(), std::back_inserter(combined_sample.kp_data));
//            std::move(ds.lp_data.begin(), ds.lp_data.end(), std::back_inserter(combined_sample.lp_data));
//            std::move(ds.model_data.begin(), ds.model_data.end(), std::back_inserter(combined_sample.model_data));
//        }
//    }
//
//    if (data_found)
//    {
//        // TODO you can use your own memory allocator with flatbufferbuilder
//        // as is, it will dynamically allocate memory
//        // Since we already have the data in a flatbuffer compatible format, let
//        // the flatbufferbuilder become the owner of this buffer?
//        // actually no, we're having to combine flatbuffers anyway
//        flatbuffers::FlatBufferBuilder fbb;
//        auto new_samp = DamarisDataSample::Pack(fbb, &combined_sample);
//        fbb.Finish(new_samp);
//        //auto s = flatbuffers::FlatBufferToString(fbb.GetBufferPointer(), DamarisDataSampleTypeTable(), true);
//        //cout << s << endl;
//        if (sim_config.write_data)
//        {
//            uint8_t *buf = fbb.GetBufferPointer();
//            flatbuffers::uoffset_t size = fbb.GetSize();
//            data_file.write(reinterpret_cast<const char*>(&size), sizeof(size));
//            data_file.write(reinterpret_cast<const char*>(buf), size);
//            //cout << "wrote " << size << " bytes to file" << endl;
//        }
//    }
//}

}
