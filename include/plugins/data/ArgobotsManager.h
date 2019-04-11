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
    static ArgobotsManager* get_instance();

    void create_initial_data_task(int32_t step);
    void check_for_rb_data(int32_t step);
    void forward_vts_data(double gvt);
    void finalize();


private:
    ArgobotsManager();
    ~ArgobotsManager();

    // no copying
    ArgobotsManager(const ArgobotsManager&) = delete;
    ArgobotsManager& operator=(const ArgobotsManager&) = delete;

    double last_processed_gvt_;
    double last_processed_rts_;
    double last_processed_vts_;

    // This ends up being Main Damaris thread, so do not assign any
    // actual argobots tasks to this.
    ABT_xstream *primary_xstream_;
    int *primary_rank_;
    ABT_xstream *proc_xstream_;
    int *proc_rank_;
    ABT_pool *pool_;
    ABT_sched *scheduler_;
};

} // end namespace data
} // end namespace ross_damaris

#endif // ANALYSIS_THREAD_H
