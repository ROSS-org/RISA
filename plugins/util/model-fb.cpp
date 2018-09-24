#include <core/model-c.h>
#include <plugins/util/model-fb.h>
#include <plugins/flatbuffers/data_sample_generated.h>
#include <ross.h>
#include <iostream>

using namespace std;

namespace ross_damaris {
namespace util{

using namespace ross_damaris::sample;

void ModelFlatBuffer::start_lp_sample(int lpid)
{
    cur_lpid_ = lpid;
}

void ModelFlatBuffer::finish_lp_sample()
{
    auto model_lp = CreateModelLPDirect(fbb_, cur_lpid_, cur_lp_type_.c_str(), &cur_lp_vars_);
    model_lps_.push_back(model_lp);

    // cleanup cur lp tracking info
    cur_lp_type_ = "";
    cur_lpid_ = -1;
    cur_lp_vars_.clear();
}

void ModelFlatBuffer::start_sample(double vts, double rts, double gvt, InstMode mode)
{
    fbb_.Clear();
    model_lps_.clear();
    cur_lp_vars_.clear();
    virtual_ts_ = vts;
    real_ts_ = rts;
    gvt_ = gvt;
    mode_ = mode;
}

void ModelFlatBuffer::finish_sample()
{
    auto ds = CreateDamarisDataSampleDirect(fbb_, virtual_ts_, real_ts_, gvt_, mode_, nullptr, nullptr, nullptr, &model_lps_);
    fbb_.Finish(ds);

    // now get the pointer and size and expose to Damaris
    uint8_t *buf = fbb_.GetBufferPointer();
    int size = fbb_.GetSize();
    int err;
    // I think damaris parameters may not actually use the default value that you can supposedly set in the XML file...
    // OR damaris doesn't like it when the amount we write is not exactly the same size as we tell it
    // but when we use char* with damaris for paths, that isn't the case
    //if (size > max_sample_size_)
    {
        max_sample_size_ = size;
        if ((err = damaris_parameter_set("model_sample_size", &max_sample_size_, sizeof(max_sample_size_))) != DAMARIS_OK)
            st_damaris_error(TW_LOC, err, "model_sample_size");
    }

    cout << "size of model fb " << size << " max_sample_size_ " << max_sample_size_ << endl;
    if ((err = damaris_write_block("model/sample_size", 0, &size)) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, "model/sample_size");
    if ((err = damaris_write_block("model/sample", 0, &buf[0])) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, "model/sample");
}

template <>
fb::Offset<void> ModelFlatBuffer::create_data_type_holder<int>(fb::Offset<fb::Vector<int>> data_vec)
{
    //v.Set(IntVarT());
    //IntVarT *vt = v.AsIntVar();
    //for (int i = 0; i < num_elements; i++)
    //    vt->value.push_back(data[i]);

    return CreateIntVar(fbb_, data_vec).Union();
}

template <>
fb::Offset<void> ModelFlatBuffer::create_data_type_holder<long>(fb::Offset<fb::Vector<long>> data_vec)
{
    return CreateLongVar(fbb_, data_vec).Union();
}

template <>
fb::Offset<void> ModelFlatBuffer::create_data_type_holder<float>(fb::Offset<fb::Vector<float>> data_vec)
{
    return CreateFloatVar(fbb_, data_vec).Union();
}

template <>
fb::Offset<void> ModelFlatBuffer::create_data_type_holder<double>(fb::Offset<fb::Vector<double>> data_vec)
{
    return CreateDoubleVar(fbb_, data_vec).Union();
}

template <>
VariableType ModelFlatBuffer::get_var_value_type<int>()
{
    return VariableType_IntVar;
}

template <>
VariableType ModelFlatBuffer::get_var_value_type<long>()
{
    return VariableType_LongVar;
}

template <>
VariableType ModelFlatBuffer::get_var_value_type<float>()
{
    return VariableType_FloatVar;
}

template <>
VariableType ModelFlatBuffer::get_var_value_type<double>()
{
    return VariableType_DoubleVar;
}

} // end namespace util
} // end namespace ross_damaris
