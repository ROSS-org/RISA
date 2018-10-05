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
#include <plugins/streaming/stream-client.h>

namespace ross_damaris {
namespace data {

class DataProcessor {
public:
    DataProcessor() :
        last_processed_gvt_(0.0),
        current_gvt_(0.0) {  }

    void aggregate_data();
    void forward_model_data();

    // for non-threaded version
    // just aggregating into a single flatbuffer
    // and putting into StreamClient buffer
    void forward_data(sample::InstMode mode, double ts);

    // delete data from the multi-index with sampling time <= ts
    void delete_data(double ts);

    void set_manager_ptr(boost::shared_ptr<DataManager>&& ptr);
    void set_stream_ptr(boost::shared_ptr<streaming::StreamClient>&& ptr);

private:
    double last_processed_gvt_;
    double current_gvt_; // most recent GVT we know of, so we can update data status
    boost::shared_ptr<DataManager> data_manager_;
    boost::shared_ptr<streaming::StreamClient> stream_client_;
};

} // end namespace data
} // end namespace ross_damaris

#endif // DATA_PROCESSOR_H
