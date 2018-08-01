#include <ross.h>
#include <Damaris.h>

extern int g_st_ross_rank;
extern int g_st_damaris_enabled;
extern int g_st_opt_debug;

/* if a function call starts with damaris, it is a call directly from Damaris library
 * any function here that starts with st_damaris means it is from the ROSS-Damaris library
 */
extern const tw_optdef *st_damaris_opts();
extern void st_damaris_init_print();
extern void st_damaris_ross_init();
extern void st_damaris_inst_init();
extern void st_damaris_ross_finalize();
extern void st_damaris_expose_data(tw_pe *me, tw_stime gvt, int inst_type);
extern void st_damaris_end_iteration();
extern tw_stime st_damaris_opt_debug_sync(tw_pe *pe);
extern void st_damaris_error(const char *file, int line, int err, char *variable);
void st_damaris_opt_debug_data(tw_pe *pe);
void st_damaris_expose_event_data(tw_event *e);


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
