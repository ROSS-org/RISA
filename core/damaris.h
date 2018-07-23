#include <ross.h>
#include <Damaris.h>

extern int g_st_ross_rank;
extern int g_st_damaris_enabled;

/* if a function call starts with damaris, it is a call directly from Damaris library
 * any function here that starts with st_damaris means it is from the ROSS-Damaris library
 */
extern const tw_optdef *st_damaris_opts(void);
extern void st_damaris_init_print();
extern void st_damaris_ross_init();
extern void st_damaris_inst_init();
extern void st_damaris_ross_finalize();
extern void st_damaris_expose_data(tw_pe *me, tw_stime gvt, int inst_type);
extern void st_damaris_end_iteration();
extern void st_damaris_error(const char *file, int line, int err, char *variable);
