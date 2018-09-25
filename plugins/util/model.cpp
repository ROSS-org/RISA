#include <plugins/flatbuffers/data_sample_generated.h>
#include <core/model-c.h>
#include <plugins/util/model-fb.h>
#include <ross.h>
#include <iostream>

// may not need flatbuffers stuff in this file
using namespace ross_damaris::sample;
using namespace ross_damaris::util;

// setup model's flatbuffers table
FlatBufferHelper mfb;

void st_damaris_start_sample(double vts, double rts, double gvt, int inst_type)
{
    InstMode mode;
    if (inst_type == GVT_INST)
        mode = InstMode_GVT;
    else if (inst_type == RT_INST)
        mode = InstMode_RT;
    else if (inst_type == VT_INST)
        mode = InstMode_VT;

    mfb.start_sample(vts, rts, gvt, mode);
}

void st_damaris_finish_sample()
{
    mfb.finish_sample();
}

void st_damaris_sample_pe_data(tw_pe *pe, tw_statistics *last_pe_stats, int inst_type)
{
    tw_statistics s;
    bzero(&s, sizeof(s));
    tw_get_stats(pe, &s);
    mfb.pe_sample(pe, &s, last_pe_stats, inst_type);
    memcpy(&last_pe_stats[inst_type], &s, sizeof(tw_statistics));
}

void st_damaris_sample_kp_data(int inst_type)
{
    tw_kp *kp;
    for (unsigned int kpid = 0; kpid < g_tw_nkp; kpid++)
    {
        kp = tw_getkp(kpid);
        mfb.kp_sample(kp, inst_type);
    }
}

void st_damaris_sample_lp_data(int inst_type)
{
    tw_lp *lp;
    for (unsigned int lpid = 0; lpid < g_tw_nlp; lpid++)
    {
        lp = tw_getlp(lpid);
        mfb.lp_sample(lp, inst_type);
    }
}

// call from ROSS-damaris interface to save in flatbuffers format and expose to Damaris
void st_damaris_sample_model_data()
{
    //std::cout << "Reached st_damaris_sample_model_data()\n";
    tw_lpid lpid;
    tw_lp *lp;

    for (lpid = 0; lpid < g_tw_nlp; lpid++)
    {
        lp = tw_getlp(lpid);
        // start the building of the ModelLP
        mfb.start_lp_model_sample(lp->gid);

        // call model LP's instrumentation sampling function
        // the implemented function should utilize one of st_damaris_save_model_variable_*() below
        if (lp->model_types == NULL || lp->model_types->sample_struct_sz == 0)
            continue;

        // we're making use of the LP callback used for virtual time sampling
        // but using it a bit differently
        // no bitfield needed because this isn't associated with any event
        // or idk maybe we need it for virtual time sampling actually, but not GVT or RT
        // TODO figure this out
        lp->model_types->sample_event_fn(lp->cur_state, NULL, lp, NULL);

        // now finish up flatbuffer stuff for this LP
        mfb.finish_lp_model_sample();
    }
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

