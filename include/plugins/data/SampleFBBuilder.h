#ifndef SAMPLE_FB_BUILDER_H
#define SAMPLE_FB_BUILDER_H

#include <iostream>
#include <vector>
#include <plugins/flatbuffers/data_sample_generated.h>
#include <plugins/util/SimConfig.h>
#include <plugins/data/Features.h>
#include <instrumentation/st-instrumentation-internal.h>
#include <instrumentation/st-model-data.h>
#include <vtkFloatArray.h>
#include <vtkTable.h>

namespace ross_damaris {
namespace data {

class SampleFBBuilder
{
public:
    SampleFBBuilder(double vts, double rts, double gvt, sample::InstMode mode) :
        sim_config_(config::SimConfig::get_instance()),
        vts_(vts),
        rts_(rts),
        last_gvt_(gvt),
        mode_(mode),
        is_finished_(false),
        raw_(nullptr) {  }

    SampleFBBuilder(SampleFBBuilder&& s);
    SampleFBBuilder& operator=(SampleFBBuilder&& s);

    void add_pe(const st_pe_stats* pe_stats);
    void add_kp(const st_kp_stats* kp_stats, int peid);
    void add_lp(const st_lp_stats* lp_stats, int peid);
    void add_lp(vtkTable* lp_table, int row, int peid, int kpid, int lpid);
    void add_feature(Port type, vtkFloatArray* arr, sample::FeatureType ft, sample::MetricType mt);
    char* add_model_lp(st_model_data* model_lp, char* buffer, size_t& buf_size, int peid);
    void finish();
    uint8_t* release_raw(size_t& fb_size, size_t& offset);
    flatbuffers::FlatBufferBuilder* get_fbb();

private:
    SampleFBBuilder(const SampleFBBuilder&) = delete;
    SampleFBBuilder& operator=(const SampleFBBuilder&&) = delete;

    flatbuffers::FlatBufferBuilder fbb_;
    std::vector<flatbuffers::Offset<sample::PEData>> pe_vector_;
    std::vector<flatbuffers::Offset<sample::KPData>> kp_vector_;
    std::vector<flatbuffers::Offset<sample::LPData>> lp_vector_;
    std::vector<flatbuffers::Offset<sample::FeatureData>> pe_feature_vector_;
    std::vector<flatbuffers::Offset<sample::FeatureData>> kp_feature_vector_;
    std::vector<flatbuffers::Offset<sample::ModelLP>> mlp_vector_;

    config::SimConfig* sim_config_;
    double vts_;
    double rts_;
    double last_gvt_;
    sample::InstMode mode_;
    bool is_finished_;
    uint8_t *raw_;
    size_t offset_;
    size_t size_;
    size_t fb_size_;

};

} // end namespace data
} // end namespace ross_damaris

#endif // SAMPLE_FB_BUILDER_H
