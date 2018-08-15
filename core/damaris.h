#ifndef DAMARIS_H
#define DAMARIS_H

/**
 * @file damaris.h
 * @brief Describes the API for using Damaris with ROSS.
 *
 * The variables and functions defined here should only be used directly by the 
 * Damaris integration component. ROSS and models should NOT call anything here directly!
 */
#include "damaris-api.h"
#include <ross.h>
#include <Damaris.h>

extern int g_st_opt_debug;
extern int g_st_rng_check;

/* if a function call starts with damaris, it is a call directly from Damaris library
 * any function here that starts with st_damaris means it is from the ROSS-Damaris library
 */
void st_damaris_opt_debug_data(tw_pe *pe);
void st_damaris_opt_debug_finalize();
void st_damaris_expose_setup_data();
void st_damaris_opt_debug_init_print();
void st_damaris_rng_check_end_iteration();


typedef struct {
    void **data_ptr;
    char *var_path;
} damaris_variable;

typedef enum {
    DPE_EVENT_PROC,
    DPE_EVENT_ABORT,
    DPE_E_RBS,
    DPE_RB_TOTAL,
    DPE_RB_PRIM,
    DPE_RB_SEC,
    DPE_FC_ATTEMPT,
    DPE_PQ_SIZE,
    DPE_NET_SEND,
    DPE_NET_RECV,
    DPE_NGVTS,
    DPE_EVENT_TIES,
    DPE_COMMITTED_EV,
    DPE_EFF,
    DPE_NET_READ_TIME,
    DPE_NET_OTHER_TIME,
    DPE_GVT_TIME,
    DPE_FC_TIME,
    DPE_EVENT_ABORT_TIME,
    DPE_EVENT_PROC_TIME,
    DPE_PQ_TIME,
    DPE_RB_TIME,
    DPE_CANQ_TIME,
    DPE_AVL_TIME,
    NUM_PE_VARS
} pe_vars;

typedef enum {
    DKP_EVENT_PROC,
    DKP_EVENT_ABORT,
    DKP_E_RBS,
    DKP_RB_TOTAL,
    DKP_RB_PRIM,
    DKP_RB_SEC,
    DKP_NET_SEND,
    DKP_NET_RECV,
    DKP_VT_DIFF,
    DKP_EFF,
    NUM_KP_VARS
} kp_vars;

typedef enum {
    DLP_EVENT_PROC,
    DLP_EVENT_ABORT,
    DLP_E_RBS,
    DLP_NET_SEND,
    DLP_NET_RECV,
    DLP_COMMITTED_EV,
    DLP_EFF,
    NUM_LP_VARS
} lp_vars;

extern damaris_variable pe_data[NUM_PE_VARS];
extern damaris_variable kp_data[NUM_KP_VARS];
extern damaris_variable lp_data[NUM_LP_VARS];

// set up variable path names based on setup in Damaris XML file
static const char *pe_var_names[NUM_PE_VARS] = {
    "nevent_processed",
    "nevent_abort",
    "nevent_rb",
    "rb_total",
    "rb_prim",
    "rb_sec",
    "fc_attempts", 
    "pq_qsize", 
    "network_send",
    "network_recv",
    "num_gvt",
    "event_ties",
    "committed_events",
    "efficiency",
    "net_read_time",
    "net_other_time",
    "gvt_time",
    "fc_time",
    "event_abort_time",
    "event_proc_time",
    "pq_time",
    "rb_time",
    "cancel_q_time",
    "avl_time"};

static const char *kp_var_names[NUM_KP_VARS] = {
    "nevent_processed",
    "nevent_abort",
    "nevent_rb",
    "rb_total",
    "rb_prim",
    "rb_sec",
    "network_send",
    "network_recv",
    "virtual_time_diff",
    "efficiency"};

static const char *lp_var_names[NUM_LP_VARS] = {
    "nevent_processed",
    "nevent_abort",
    "nevent_rb",
    "network_send",
    "network_recv",
    "committed_events",
    "efficiency"};


void opt_debug_init();

#endif // DAMARIS_H
