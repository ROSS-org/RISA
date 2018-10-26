#ifndef ANALYSIS_THREAD_H
#define ANALYSIS_THREAD_H

#include <iostream>
#include <plugins/flatbuffers/data_sample_generated.h>
#include <plugins/data/DataManager.h>
#include <plugins/streaming/StreamClient.h>
#include <plugins/util/SimConfig.h>
#include <abt.h>

namespace ross_damaris {
namespace data {

class ArgobotsManager
{
public:
    ArgobotsManager(boost::shared_ptr<DataManager>&& dm_ptr,
            boost::shared_ptr<streaming::StreamClient>&& sc_ptr,
            boost::shared_ptr<config::SimConfig>&& conf_ptr) :
        last_processed_gvt_(0.0),
        last_processed_rts_(0.0),
        last_processed_vts_(0.0),
        data_manager_(boost::shared_ptr<DataManager>(dm_ptr)),
        stream_client_(boost::shared_ptr<streaming::StreamClient>(sc_ptr)),
        sim_config_(boost::shared_ptr<config::SimConfig>(conf_ptr))
    {
        my_xstream_ = (ABT_xstream*)malloc(sizeof(ABT_xstream));
    }

    ArgobotsManager() :
        last_processed_gvt_(0.0),
        last_processed_rts_(0.0),
        last_processed_vts_(0.0),
        data_manager_(nullptr),
        stream_client_(nullptr),
        sim_config_(nullptr) {  }

    void start_processing();


private:
    double last_processed_gvt_;
    double last_processed_rts_;
    double last_processed_vts_;

    boost::shared_ptr<DataManager> data_manager_;
    boost::shared_ptr<streaming::StreamClient> stream_client_;
    boost::shared_ptr<config::SimConfig> sim_config_;

    ABT_xstream *my_xstream_;

    void set_last_processed(sample::InstMode mode, double ts)
    {
        if (mode == sample::InstMode_GVT)
            last_processed_gvt_ = ts;
        else if (mode == sample::InstMode_RT)
            last_processed_rts_ = ts;
        else if (mode == sample::InstMode_VT)
            last_processed_vts_ = ts;
    }

    void aggregate_data(sample::InstMode mode, double lower_ts, double upper_ts);
    void combine_pe_flatbuffers(sample::InstMode mode, double lower_ts, double upper_ts);
    void combine_kp_flatbuffers(sample::InstMode mode, double lower_ts, double upper_ts);
    void combine_lp_flatbuffers(sample::InstMode mode, double lower_ts, double upper_ts);

    void forward_model_data();
};

} // end namespace data
} // end namespace ross_damaris

#endif // ANALYSIS_THREAD_H
