#ifndef MODEL_C_H
#define MODEL_C_H

#ifdef __cplusplus
#define EXTERNC extern "C"
#include <cstddef>
#else
#define EXTERNC
#endif

#include <ross.h>

EXTERNC void st_damaris_start_sample(double vts, double rts, double gvt, int inst_type);
EXTERNC void st_damaris_start_sample_vt(double vts, double rts, double gvt, int inst_type,
            int entity_id, int event_id);
EXTERNC void *st_damaris_finish_sample(size_t* offset);
EXTERNC void st_damaris_sample_pe_data(tw_pe *pe, tw_statistics *last_pe_stats,
            int inst_type);
EXTERNC void st_damaris_sample_kp_data(int inst_type, tw_kpid *id);
EXTERNC void st_damaris_sample_lp_data(int inst_type, tw_lpid *lpids, int nlps);
EXTERNC void st_damaris_invalidate_sample(double ts, int kp_gid, int event_id);
EXTERNC void st_damaris_sample_model_data(tw_lpid *lpids, int nlps);
EXTERNC void st_damaris_reset_block_counters();
EXTERNC void st_damaris_set_rc_data(uint8_t *raw, size_t offset);
EXTERNC void st_damaris_delete_rc_data();
EXTERNC void st_damaris_save_model_variable_int(int lpid, const char* lp_type,
            const char* var_name, int *data, size_t num_elements);
EXTERNC void st_damaris_save_model_variable_long(int lpid, const char* lp_type,
            const char* var_name, long *data, size_t num_elements);
EXTERNC void st_damaris_save_model_variable_float(int lpid, const char* lp_type,
            const char* var_name, float *data, size_t num_elements);
EXTERNC void st_damaris_save_model_variable_double(int lpid, const char* lp_type,
            const char* var_name, double *data, size_t num_elements);
EXTERNC const int* st_damaris_get_model_variable_int(int lpid, const char* lp_type,
            const char* var_name, size_t *num_elements);
EXTERNC const long* st_damaris_get_model_variable_long(int lpid, const char* lp_type,
            const char* var_name, size_t *num_elements);
EXTERNC const float* st_damaris_get_model_variable_float(int lpid, const char* lp_type,
            const char* var_name, size_t *num_elements);
EXTERNC const double* st_damaris_get_model_variable_double(int lpid, const char* lp_type,
            const char* var_name, size_t *num_elements);

#undef EXTERNC

#endif // MODEL_C_H
