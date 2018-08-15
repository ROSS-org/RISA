#ifndef DAMARIS_API_H
#define DAMARIS_API_H

/**
 * @file damaris-api.h
 * @brief Describes the public API for using Damaris with ROSS
 *
 * This file should only define variables and functions that can be called directly by
 * ROSS or models.
 */

#include <ross.h>
#include <Damaris.h>


extern int g_st_ross_rank;
extern int g_st_damaris_enabled;
extern int g_st_debug_enabled;

const tw_optdef *st_damaris_opts();
void st_damaris_ross_init();
void st_damaris_init_print();
void st_damaris_inst_init();
void st_damaris_expose_data(tw_pe *me, tw_stime gvt, int inst_type);
void st_damaris_expose_event_data(tw_event *e, const char *prefix);
void st_damaris_end_iteration();
void st_damaris_ross_finalize();
void st_damaris_error(const char *file, int line, int err, char *variable);


void st_damaris_opt_debug_map(event_f handler, tw_lpid id);
tw_stime st_damaris_opt_debug_sync(tw_pe *pe);
void st_damaris_call_event(tw_event *cev, tw_lp *clp);
#endif // DAMARIS_API_H
