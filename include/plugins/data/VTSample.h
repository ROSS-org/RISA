#ifndef VTSAMPLE_H
#define VTSAMPLE_H

#include <plugins/flatbuffers/data_sample_generated.h>

namespace ross_damaris {
namespace data {

class VTSample {
public:
    VTSample(int kp_gid, double vts, double rts, double gvt,
            sample::DataStatus status = sample::DataStatus_speculative) :
        kp_gid_(kp_gid),
        vts_(vts),
        rts_(rts),
        status_(status),
        sample_fbb_(vts, rts, gvt, sample::InstMode_VT) {  }

    VTSample(VTSample&& s) :
        kp_gid_(s.kp_gid_),
        vts_(s.vts_),
        rts_(s.rts_),
        status_(s.status_),
        sample_fbb_(std::move(s.sample_fbb_)) {  }

    VTSample& operator=(VTSample&& s)
    {
        kp_gid_ = s.kp_gid_;
        vts_ = s.vts_;
        rts_ = s.rts_;
        status_ = s.status_;
        sample_fbb_ = std::move(s.sample_fbb_);
        return *this;
    }

    virtual int get_kp_gid() const { return kp_gid_; }
    virtual double get_vts() const { return vts_; }
    virtual double get_rts() const { return rts_; }

    SampleFBBuilder* get_sample_fbb() { return &sample_fbb_; }

    void invalidate_sample()
    {
        status_ = sample::DataStatus_invalid;
    }

    void validate_sample()
    {
        status_ = sample::DataStatus_committed;
    }

private:
    VTSample(const VTSample&) = delete;
    VTSample& operator=(const VTSample&) = delete;

    int kp_gid_;
    double vts_;
    double rts_;
    sample::DataStatus status_;
    SampleFBBuilder sample_fbb_;
};

} // end namespace data
} // end namespace ross_damaris

#endif // VTSAMPLE_H
