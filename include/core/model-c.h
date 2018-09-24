#ifndef MODEL_C_H
#define MODEL_C_H

#ifdef __cplusplus
#define EXTERNC extern "C"
#include <cstddef>
#else
#define EXTERNC
#endif


//EXTERNC void st_damaris_save_model_variable();
EXTERNC void st_damaris_sample_model_data(double vts, double rts, double gvt, int inst_type);
EXTERNC void st_damaris_save_model_variable_int(int lpid, const char* lp_type, const char* var_name, int *data, size_t num_elements);
EXTERNC void st_damaris_save_model_variable_long(int lpid, const char* lp_type, const char* var_name, long *data, size_t num_elements);
EXTERNC void st_damaris_save_model_variable_float(int lpid, const char* lp_type, const char* var_name, float *data, size_t num_elements);
EXTERNC void st_damaris_save_model_variable_double(int lpid, const char* lp_type, const char* var_name, double *data, size_t num_elements);

#undef EXTERNC

#endif // MODEL_C_H
