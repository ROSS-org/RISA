#include <ross.h>
#include <Damaris.h>

extern int g_st_ross_rank;
extern int g_st_damaris_enabled;

/* if a function call starts with damaris, it is a call directly from Damaris library
 * any function here that starts with st_damaris means it is from the ROSS-Damaris library
 */
const tw_optdef *st_damaris_opts(void);
void st_damaris_init_print();
void st_damaris_ross_init();
void st_damaris_inst_init(const char* config_file);
void st_damaris_signal_init();
void st_damaris_ross_finalize();
void st_damaris_expose_data(tw_pe *me, int inst_type);
void st_damaris_end_iteration();
void st_damaris_error(const char *file, int line, int err, const char *variable);
void st_damaris_parse_config(const char *config_file);
