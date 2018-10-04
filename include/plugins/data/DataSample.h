#ifndef DATA_SAMPLE_H
#define DATA_SAMPLE_H

#include <plugins/flatbuffers/data_sample_generated.h>
#include <damaris/buffer/DataSpace.hpp>
#include <damaris/buffer/Buffer.hpp>

namespace ross_damaris {
namespace data {

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

    // move constructor
    DataSample(DataSample&& s) = default;
    ~DataSample() = default;

    virtual int get_mode() const { return mode_; }
    virtual double get_sampling_time() const { return sampling_time_; }
    virtual int get_status() const { return status_; }

    //void push_data_ptr(const sample::DamarisDataSample& sample);
    void push_ds_ptr(const damaris::DataSpace<damaris::Buffer> ds);
    damaris::DataSpace<damaris::Buffer> get_data_at(int i) { return ds_ptrs_[i]; }
    int get_ds_ptr_size() { return ds_ptrs_.size(); }

private:
    // prevent use of copy constructor
    DataSample(DataSample& s) = default;

    sample::InstMode mode_;
    double sampling_time_;
    sample::DataStatus status_;
    //std::vector<sample::DamarisDataSample> data_ptrs_;
    // to ensure we can keep access to the data pointers without Damaris deleting them automatically
    // maybe, idk
    std::vector<damaris::DataSpace<damaris::Buffer>> ds_ptrs_;
};

} // end namespace data
} // end namespace ross_damaris

#endif // DATA_SAMPLE_H
