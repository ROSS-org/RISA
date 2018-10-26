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
 * @brief Handles the bulk of the data processing for ROSS-Damaris.
 *
 * This class is responsible for starting a new thread for data
 * analysis tasks.
 */
class DataProcessor {
public:
    DataProcessor(boost::shared_ptr<DataManager>&& dm_ptr,
            boost::shared_ptr<streaming::StreamClient>&& sc_ptr,
            boost::shared_ptr<config::SimConfig>&& conf_ptr,
            bool use_threads = false) :
        last_processed_ts_(0.0),
        data_manager_(boost::shared_ptr<DataManager>(dm_ptr)),
        stream_client_(boost::shared_ptr<streaming::StreamClient>(sc_ptr)),
        sim_config_(boost::shared_ptr<config::SimConfig>(conf_ptr)),
        analysis_thread_(std::move(dm_ptr), std::move(sc_ptr), std::move(conf_ptr)),
        use_threads_(use_threads),
        t_(nullptr)
    {
        if (use_threads_)
            t_ = new std::thread(&ArgobotsManager::start_processing,
                        &analysis_thread_);
    }

    DataProcessor() :
        last_processed_ts_(0.0),
        data_manager_(nullptr),
        stream_client_(nullptr),
        t_(nullptr)
        {  }

    ~DataProcessor()
    {
        if (t_ && t_->joinable())
        {
            t_->join();
            delete t_;
        }
    }

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
    bool use_threads_;
    std::thread *t_;
    ArgobotsManager analysis_thread_;

};

} // end namespace data
} // end namespace ross_damaris

#endif // DATA_PROCESSOR_H
