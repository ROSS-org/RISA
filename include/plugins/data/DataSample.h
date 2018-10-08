#ifndef DATA_SAMPLE_H
#define DATA_SAMPLE_H

#include <plugins/flatbuffers/data_sample_generated.h>
#include <damaris/buffer/DataSpace.hpp>
#include <damaris/buffer/Buffer.hpp>
#include <unordered_map>

namespace ross_damaris {
namespace data {

/**
 * @brief Represents a data sample for a given time point and instrumentation mode.
 *
 * A DataSample may have more than one Damaris DataSpace associated with it.
 * This typically happens with data when it is first received from ROSS.
 */
class DataSample {
public:
    DataSample() :
        mode_(sample::InstMode_none),
        sampling_time_(0.0),
        status_(sample::DataStatus_none) {  }

    DataSample(double ts, sample::InstMode mode, sample::DataStatus status) :
        mode_(mode),
        sampling_time_(ts),
        status_(status) {  }

    virtual int get_mode() const { return mode_; }
    virtual double get_sampling_time() const { return sampling_time_; }
    virtual int get_status() const { return status_; }

    /**
     * @brief add a DataSpace associated with this sampling point
     * @param id The id of the entity (KP or PE) associated with this sample.
     * @param ds A Damaris DataSpace (essentially a shared_ptr).
     */
    void push_ds_ptr(int id, const damaris::DataSpace<damaris::Buffer> ds);

    /**
     * @brief get the requested DataSpace
     * @param id Id of the entity to search for.
     * @return A Damaris DataSpace (essentially a shared_ptr).
     */
    damaris::DataSpace<damaris::Buffer> get_data_at(int id) { return ds_ptrs_[id]; }

    /**
     * @brief get number of DataSpaces associated with this sampling point
     * @return The number of dataspaces as an integer
     */
    int get_ds_ptr_size() { return ds_ptrs_.size(); }

private:
    /** Instrumentation mode used to collect this sample */
    sample::InstMode mode_;

    /** Time stamp (real or virtual) associated with this sample. */
    double sampling_time_;

    /** is the Data speculative, committed, or invalid? */
    sample::DataStatus status_;

    /** map of DataSpaces to their entity id (KP or PE) */
    std::unordered_map<int, damaris::DataSpace<damaris::Buffer>> ds_ptrs_;
};

} // end namespace data
} // end namespace ross_damaris

#endif // DATA_SAMPLE_H
