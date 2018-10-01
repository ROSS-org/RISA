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


//std::vector<flatbuffers::Offset<SimEngineMetrics>> sim_engine_metrics_to_fb(flatbuffers::FlatBufferBuilder& builder,
//        int32_t src, int32_t step, int32_t num_entities, const std::string& var_prefix)
//{
//    std::vector<flatbuffers::Offset<SimEngineMetrics>> data;
//    SimEngineMetricsT *metrics_objects = new SimEngineMetricsT[num_entities];
//    FBUtil::collect_metrics(&metrics_objects[0], var_prefix, src, step, 0, num_entities);
//    for (int id = 0; id < num_entities; id++)
//    {
//        auto metrics = CreateSimEngineMetrics(builder, &metrics_objects[id]);
//        data.push_back(metrics);
//    }
//    delete[] metrics_objects;
//    return data;
//}
//
//void pe_data_to_fb(flatbuffers::FlatBufferBuilder& builder, int32_t step,
//        std::vector<flatbuffers::Offset<PEData>>& pe_data)
//{
//    std::string var_prefix = "ross/pes/gvt_inst/";
//    for (int peid = 0; peid < sim_config.num_pe; peid++)
//    {
//        auto metrics = sim_engine_metrics_to_fb(builder, peid, step, 1, var_prefix);
//        pe_data.push_back(CreatePEData(builder, peid, metrics[0])); // for PEs, it will always be metrics[0]
//    }
//}
//
//void kp_data_to_fb(flatbuffers::FlatBufferBuilder& builder, int32_t step,
//        std::vector<flatbuffers::Offset<KPData>>& kp_data)
//{
//    std::string var_prefix = "ross/kps/gvt_inst/";
//    for (int peid = 0; peid < sim_config.num_pe; peid++)
//    {
//        auto metrics = sim_engine_metrics_to_fb(builder, peid, step, sim_config.kp_per_pe, var_prefix);
//        for (int kpid = 0; kpid < sim_config.kp_per_pe; kpid++)
//        {
//            int kp_gid = (peid * sim_config.kp_per_pe) + kpid;
//            kp_data.push_back(CreateKPData(builder, peid, kpid, kp_gid, metrics[kpid]));
//
//        }
//    }
//}
//
//void lp_data_to_fb(flatbuffers::FlatBufferBuilder& builder, int32_t step,
//        std::vector<flatbuffers::Offset<LPData>>& lp_data)
//{
//    std::string var_prefix = "ross/lps/gvt_inst/";
//    int total_lp = 0;
//    for (int peid = 0; peid < sim_config.num_pe; peid++)
//    {
//        auto metrics = sim_engine_metrics_to_fb(builder, peid, step, sim_config.num_lp[peid], var_prefix);
//        for (int lpid = 0; lpid < sim_config.num_lp[peid]; lpid++)
//        {
//            int kpid = lpid % sim_config.kp_per_pe;
//            int kp_gid = (peid * sim_config.kp_per_pe) + kpid;
//            int lp_gid = total_lp + lpid;
//            lp_data.push_back(CreateLPData(builder, peid, kpid, kp_gid, lpid, lp_gid, metrics[lpid]));
//
//        }
//        total_lp += sim_config.num_lp[peid];
//    }
//}

}
