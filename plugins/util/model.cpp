#include <plugins/flatbuffers/data_sample_generated.h>
#include <core/model-c.h>
#include <plugins/util/model-fb.h>
#include <ross.h>
#include <iostream>

// may not need flatbuffers stuff in this file
using namespace ross_damaris::sample;
using namespace ross_damaris::util;
namespace fb = flatbuffers;

// setup model's flatbuffers table
ModelFlatBuffer mfb;
// call from ROSS-damaris interface to save in flatbuffers format and expose to Damaris
void st_damaris_sample_model_data(double vts, double rts, double gvt, int inst_type)
{
    //std::cout << "Reached st_damaris_sample_model_data()\n";
    tw_lpid lpid;
    tw_lp *lp;
    InstMode mode;

    if (inst_type == GVT_INST)
        mode = InstMode_GVT;
    else if (inst_type == RT_INST)
        mode = InstMode_RT;
    else if (inst_type == VT_INST)
        mode = InstMode_VT;

    mfb.start_sample(vts, rts, gvt, mode);
    for (lpid = 0; lpid < g_tw_nlp; lpid++)
    {
        // start the building of the ModelLP
        mfb.start_lp_sample(lpid);

        // call model LP's instrumentation sampling function
        // the implemented function should utilize one of st_damaris_save_model_variable_*() below
        lp = tw_getlp(lpid);
        if (lp->model_types == NULL || lp->model_types->sample_struct_sz == 0)
            continue;

        // we're making use of the LP callback used for virtual time sampling
        // but using it a bit differently
        // no bitfield needed because this isn't associated with any event
        // or idk maybe we need it for virtual time sampling actually, but not GVT or RT
        // TODO figure this out
        lp->model_types->sample_event_fn(lp->cur_state, NULL, lp, NULL);

        // now finish up flatbuffer stuff for this LP
        mfb.finish_lp_sample();
    }

    mfb.finish_sample();

}

// have C wrapper functions specific to data type
// main function will use templates
// this function can be called by a model to save a specific model variable
void st_damaris_save_model_variable_int(int lpid, const char* lp_type, const char* var_name, int *data, size_t num_elements)
{
    mfb.save_model_variable(lpid, lp_type, var_name, data, num_elements);
}

void st_damaris_save_model_variable_long(int lpid, const char* lp_type, const char* var_name, long *data, size_t num_elements)
{
    mfb.save_model_variable(lpid, lp_type, var_name, data, num_elements);
}

void st_damaris_save_model_variable_float(int lpid, const char* lp_type, const char* var_name, float *data, size_t num_elements)
{
    mfb.save_model_variable(lpid, lp_type, var_name, data, num_elements);
}

void st_damaris_save_model_variable_double(int lpid, const char* lp_type, const char* var_name, double *data, size_t num_elements)
{
    mfb.save_model_variable(lpid, lp_type, var_name, data, num_elements);
}

