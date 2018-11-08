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
 * This typically happens with data when it is first received from ROSS, since
 * a full sample will be built up from multiple flatbuffers.
 */
class DataSample {
public:
    DataSample() :
        mode_(sample::InstMode_none),
        sampling_time_(0.0),
        status_(sample::DataStatus_speculative) {  }

    DataSample(double ts, sample::InstMode mode, sample::DataStatus status) :
        mode_(mode),
        sampling_time_(ts),
        status_(status) {  }

    virtual int get_mode() const { return mode_; }
    virtual double get_sampling_time() const { return sampling_time_; }
    virtual int get_status() const { return status_; }

    bool same_sample(DataSample& other)
    {
        if (mode_ == other.get_mode() && sampling_time_ == other.get_sampling_time())
            return true;
        return false;
    }

    bool same_sample(sample::InstMode mode, double ts)
    {
        if (mode_ == mode && sampling_time_ == ts)
            return true;
        return false;
    }

    /**
     * @brief add a DataSpace associated with this sampling point
     * @param id The id of the entity (KP or PE) associated with this sample.
     * @param ds A Damaris DataSpace (essentially a shared_ptr).
     */
    void push_ds_ptr(int id, const damaris::DataSpace<damaris::Buffer> ds);

    /**
     * @brief add a DataSpace associated with this sampling point
     * @param id The id of the entity (KP or PE) associated with this sample.
     * @param event_id the event_id for this sample
     * @param ds A Damaris DataSpace (essentially a shared_ptr).
     */
    void push_ds_ptr(int id, int event_id, const damaris::DataSpace<damaris::Buffer> ds);

    bool remove_ds_ptr(int id);
    bool remove_ds_ptr(int id, int event_id);

    /**
     * @brief get the requested DataSpace
     * @param id Id of the entity to search for.
     * @return A Damaris DataSpace (essentially a shared_ptr).
     */
    damaris::DataSpace<damaris::Buffer> get_dataspace(int id);
    damaris::DataSpace<damaris::Buffer> get_dataspace(int id, int event_id);

    std::unordered_map<int, std::unordered_map<int,
        damaris::DataSpace<damaris::Buffer>>>::iterator ds_ptrs_begin();
    std::unordered_map<int, std::unordered_map<int,
        damaris::DataSpace<damaris::Buffer>>>::iterator ds_ptrs_end();

    std::unordered_map<int, damaris::DataSpace<damaris::Buffer>>::iterator start_iteration();
    std::unordered_map<int, damaris::DataSpace<damaris::Buffer>>::iterator get_next_iterator();
    std::unordered_map<int, damaris::DataSpace<damaris::Buffer>>::iterator end_iterator();

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

    /** map of DataSpaces to their entity id (PE or KP) */
    // if mapping to PE, the key for the inner map is always -1
    // if mapping to KP gid, the key for the inner map is based on event_id
    std::unordered_map<int,
        std::unordered_map<int, damaris::DataSpace<damaris::Buffer>>> ds_ptrs_;
    std::unordered_map<int, std::unordered_map<
        int, damaris::DataSpace<damaris::Buffer>>> outer_it_;
    std::unordered_map<int, damaris::DataSpace<damaris::Buffer>> inner_it_;
};

} // end namespace data
} // end namespace ross_damaris

#endif // DATA_SAMPLE_H
