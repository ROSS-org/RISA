#ifndef VTSAMPLE_H
#define VTSAMPLE_H

#include <plugins/flatbuffers/data_sample_generated.h>
#include <plugins/data/SampleFBBuilder.h>
#include <instrumentation/st-instrumentation-internal.h>
#include <iostream>

namespace ross_damaris {
namespace data {

class TSSample {
public:
    TSSample(sample_metadata* sample_md, sample::InstMode mode,
            sample::DataStatus status = sample::DataStatus_speculative) :
        peid_(sample_md->peid),
        kp_gid_(sample_md->kp_gid),
        vts_(sample_md->vts),
        rts_(sample_md->rts),
        gvt_(sample_md->last_gvt),
        mode_(mode),
        status_(status),
        sample_fbb_(vts_, rts_, gvt_, mode_) {  }

    TSSample(TSSample&& s) :
        peid_(s.peid_),
        kp_gid_(s.kp_gid_),
        vts_(s.vts_),
        rts_(s.rts_),
        gvt_(s.gvt_),
        mode_(s.mode_),
        status_(s.status_),
        sample_fbb_(std::move(s.sample_fbb_)) {  }

    TSSample& operator=(TSSample&& s)
    {
        peid_ = s.peid_;
        kp_gid_ = s.kp_gid_;
        vts_ = s.vts_;
        rts_ = s.rts_;
        gvt_ = s.gvt_;
        mode_ = s.mode_;
        status_ = s.status_;
        sample_fbb_ = std::move(s.sample_fbb_);
        return *this;
    }

    virtual int get_peid() const { return peid_; }
    virtual int get_kp_gid() const { return kp_gid_; }
    virtual double get_vts() const { return vts_; }
    virtual double get_rts() const { return rts_; }
    virtual double get_gvt() const { return gvt_; }

    SampleFBBuilder* get_sample_fbb() { return &sample_fbb_; }

    void invalidate_sample()
    {
        status_ = sample::DataStatus_invalid;
    }

    void validate_sample()
    {
        status_ = sample::DataStatus_committed;
    }

    void print_sample()
    {
        std::cout << "peid " << peid_ << "\nkp_gid " << kp_gid_ <<
            "\nvts " << vts_ << "\nrts " << rts_ << "\ngvt " << gvt_ <<
            "\nmode " << mode_ << std::endl;
    }

private:
    TSSample(const TSSample&) = delete;
    TSSample& operator=(const TSSample&) = delete;

    int peid_;
    int kp_gid_;
    double vts_;
    double rts_;
    double gvt_;
    sample::InstMode mode_;
    sample::DataStatus status_;
    SampleFBBuilder sample_fbb_;
};

} // end namespace data
} // end namespace ross_damaris

#endif // VTSAMPLE_H
