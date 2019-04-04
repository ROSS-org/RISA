#ifndef ANALYSIS_THREAD_H
#define ANALYSIS_THREAD_H

#include <iostream>
#include <plugins/flatbuffers/data_sample_generated.h>
#include <plugins/streaming/StreamClient.h>
#include <plugins/util/SimConfig.h>
#include <abt.h>

namespace ross_damaris {
namespace data {

class ArgobotsManager
{
public:
    ArgobotsManager();
    ArgobotsManager(ArgobotsManager&& m);
    ~ArgobotsManager();

    void set_shared_ptrs(
            std::shared_ptr<streaming::StreamClient>& sc_ptr,
            std::shared_ptr<config::SimConfig>& conf_ptr);
    void create_insert_data_mic_task(int32_t step);
    void finalize();


private:
    // no copying
    ArgobotsManager(const ArgobotsManager&) = delete;
    ArgobotsManager& operator=(const ArgobotsManager&) = delete;
    // shouldn't need move assignment either
    ArgobotsManager& operator=(ArgobotsManager&& m) = delete;

    double last_processed_gvt_;
    double last_processed_rts_;
    double last_processed_vts_;

    std::shared_ptr<streaming::StreamClient> stream_client_;
    std::shared_ptr<config::SimConfig> sim_config_;

    // This ends up being Main Damaris thread, so do not assign any
    // actual argobots tasks to this.
    ABT_xstream *primary_xstream_;
    ABT_xstream *proc_xstream_;
    ABT_pool *pool_;
    ABT_sched *scheduler_;

    void set_last_processed(sample::InstMode mode, double ts)
    {
        if (mode == sample::InstMode_GVT)
            last_processed_gvt_ = ts;
        else if (mode == sample::InstMode_RT)
            last_processed_rts_ = ts;
        else if (mode == sample::InstMode_VT)
            last_processed_vts_ = ts;
    }
};

} // end namespace data
} // end namespace ross_damaris

#endif // ANALYSIS_THREAD_H
