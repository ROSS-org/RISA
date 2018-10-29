#ifndef DATA_PROCESSOR_H
#define DATA_PROCESSOR_H

/**
 * @file DataProcessor.h
 *
 * The DataProcessor handles the bulk of the data processing.
 * This includes data reduction, as well as garbage collection
 * for the multi-index.
 * This functionality will likely run on its own thread
 * that gets started when initializing the Damaris rank.
 */

#include <plugins/data/DataManager.h>
#include <plugins/streaming/StreamClient.h>
#include <plugins/util/SimConfig.h>
#include <plugins/data/ArgobotsManager.h>

namespace ross_damaris {
namespace data {

/**
 * @brief Initial single threaded version of data processing for ROSS-Damaris.
 */
class DataProcessor {
public:
    DataProcessor(boost::shared_ptr<DataManager>&& dm_ptr,
            boost::shared_ptr<streaming::StreamClient>&& sc_ptr,
            boost::shared_ptr<config::SimConfig>&& conf_ptr) :
        last_processed_ts_(0.0),
        data_manager_(boost::shared_ptr<DataManager>(dm_ptr)),
        stream_client_(boost::shared_ptr<streaming::StreamClient>(sc_ptr)),
        sim_config_(boost::shared_ptr<config::SimConfig>(conf_ptr))
    {
    }

    DataProcessor() :
        last_processed_ts_(0.0),
        data_manager_(nullptr),
        stream_client_(nullptr)
        {  }

    /**
     * @brief NOT IMPLEMENTED YET
     *
     * Will aggregate perf data on a PE or node basis
     */
    void aggregate_data();

    /**
     * @brief NOT IMPLEMENTED YET
     *
     * Checks for committed model data that needs to be streamed
     */
    void forward_model_data();

    /**
     * @brief Forwards data onto StreamClient buffer.
     *
     * Probably Temporary. Combines flatbuffers for a given
     * sample and puts in StreamClient buffer.
     */
    void forward_data(sample::InstMode mode, double ts, int32_t step);
    void invalidate_data(double ts, int32_t step);

    /**
     * @brief get shared ownership to the DataManager
     */
    void set_manager_ptr(boost::shared_ptr<DataManager>&& ptr);

    /**
     * @brief get shared ownership to the StreamClient
     */
    void set_stream_ptr(boost::shared_ptr<streaming::StreamClient>&& ptr);

    void last_gvt(double gvt) { last_gvt_ = gvt; }

private:
    double last_processed_ts_;
    double last_gvt_;
    boost::shared_ptr<DataManager> data_manager_;
    boost::shared_ptr<streaming::StreamClient> stream_client_;
    boost::shared_ptr<config::SimConfig> sim_config_;

};

} // end namespace data
} // end namespace ross_damaris

#endif // DATA_PROCESSOR_H
