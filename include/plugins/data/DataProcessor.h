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

namespace ross_damaris {
namespace data {

class DataProcessor {
public:
    DataProcessor() :
        last_processed_gvt_(0.0),
        current_gvt_(0.0) {  }

    void aggregate_data();
    void forward_model_data();

    // delete data from the multi-index with sampling time <= ts
    void delete_data(double ts);

private:
    double last_processed_gvt_;
    double current_gvt_; // most recent GVT we know of, so we can update data status
};

} // end namespace data
} // end namespace ross_damaris

#endif // DATA_PROCESSOR_H
