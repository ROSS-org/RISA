#include <ross.h>
#include <sys/stat.h>
#include<stdlib.h>
#include<unistd.h>
#include<stdio.h>
#include<fcntl.h>

/**
 * @file damaris.c
 * @brief ROSS integration with Damaris data management
 */
MPI_Comm MPI_COMM_ROSS_FULL; // only for use with optimistic debug mode
static int full_ross_rank = -1; // rank within MPI_COMM_ROSS_FULL

int g_st_ross_rank = 0;

static int damaris_initialized = 0;
static double *pe_id, *kp_id, *lp_id;
static tw_statistics last_pe_stats[3];

// TODO update to other instrumentation types
static const char *inst_path[3] = {
    "gvt_inst",
    "rt_inst",
    "vt_inst"};
static int rt_block_counter = 0;
static int vt_block_counter = 0;
static int max_block_counter = 0;
static int iterations = 0;
static tw_stime current_gvt = 0;

static void expose_pe_data(tw_pe *pe, tw_statistics *s, int inst_type);
static void expose_kp_data(tw_pe *pe, int inst_type);
static void expose_lp_data(int inst_type);

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
    DLP_EFF,
    NUM_LP_VARS
} lp_vars;

damaris_variable pe_data[NUM_PE_VARS];
damaris_variable kp_data[NUM_KP_VARS];
damaris_variable lp_data[NUM_LP_VARS];

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
    "efficiency"};

int g_st_damaris_enabled = 0;
int g_st_opt_debug = 0;
static char data_xml[4096];

static const tw_optdef damaris_options[] = {
    TWOPT_GROUP("Damaris Integration"),
    TWOPT_UINT("enable-damaris", g_st_damaris_enabled, "Turn on (1) or off (0) Damaris in situ analysis"),
    TWOPT_CHAR("data-xml", data_xml, "Path to XML file for describing data to Damaris"),
    TWOPT_UINT("opt-debug", g_st_opt_debug, "Turn on (1) or off (0) Optimistic debugging with Damaris"),
    TWOPT_END()
};

static int num_sim_args;
static char **sim_args;
static char arg_string[1024];

const tw_optdef *st_damaris_opts()
{
	return damaris_options;
}

/**
 * @brief Set simulation parameters in Damaris
 */
static void set_parameters()
{
    int err;
    int num_pe = tw_nnodes();
    int num_kp = (int) g_tw_nkp;
    int num_lp = (int) g_tw_nlp;
    //printf("PE %ld num pe %d, num kp %d, num lp %d\n", g_tw_mynode, num_pe, num_kp, num_lp);

    if ((err = damaris_parameter_set("num_lp", &num_lp, sizeof(num_lp))) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, "num_lp");

    if ((err = damaris_parameter_set("num_pe", &num_pe, sizeof(num_pe))) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, "num_pe");

    if ((err = damaris_parameter_set("num_kp", &num_kp, sizeof(num_kp))) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, "num_kp");
}

// take one of our ranks and make it run a seq sim separate from the optimistic run
void opt_debug_init()
{
    int my_rank, current_ross_size, sub_size;
    MPI_Comm new_ross_comm, debug_comm;

    // MPI_COMM_ROSS controls all non-damaris ranks at this point
    MPI_Comm_size(MPI_COMM_ROSS, &current_ross_size);

    // save our full ROSS communicator and ranks
    MPI_COMM_ROSS_FULL = MPI_COMM_ROSS;
    full_ross_rank = g_tw_mynode;
    printf("opt_debug_init full world size is %d\n", current_ross_size);
    
    if (g_tw_mynode < current_ross_size - 1) //optimistic ranks
    {
        MPI_Comm_split(MPI_COMM_ROSS_FULL, 1, g_tw_mynode, &new_ross_comm);
        MPI_Comm_split(MPI_COMM_ROSS_FULL, MPI_UNDEFINED, 0, &debug_comm);
        tw_comm_set(new_ross_comm);
    }
    else // sequential rank
    {
        MPI_Comm_split(MPI_COMM_ROSS_FULL, MPI_UNDEFINED, 0, &new_ross_comm);
        MPI_Comm_split(MPI_COMM_ROSS_FULL, 2, 0, &debug_comm);
        tw_comm_set(debug_comm);
        st_turn_off_instrumentation();

        g_tw_synchronization_protocol = SEQUENTIAL;
        freopen("redir.txt", "w", stdout); // redirect this ranks output to file for now
    }

    // now make sure we have the correct rank number for our version of MPI_COMM_ROSS
    if (MPI_Comm_rank(MPI_COMM_ROSS, &my_rank) != MPI_SUCCESS)
        tw_error(TW_LOC, "Cannot get MPI_Comm_rank(MPI_COMM_ROSS)");
    g_tw_mynode = my_rank;

    MPI_Comm_size(MPI_COMM_ROSS, &sub_size);
    printf("I am rank %ld (full_ross_rank %d) with comm size of %d\n", g_tw_mynode, full_ross_rank, sub_size);
}

/**
 * @brief Sets up ROSS to use Damaris
 *
 * This must be called after MPI is initialized. Damaris splits the MPI 
 * communicator and we set MPI_COMM_ROSS to the subcommunicator returned by Damaris.
 * This sets the g_st_ross_rank.  Need to make sure that Damaris ranks
 * (g_st_ross_rank == 0) return to model after this point. 
 *
 */
void st_damaris_ross_init()
{
    int err, my_rank;
    MPI_Comm ross_comm;

    if (!g_st_damaris_enabled)
    {
        g_st_ross_rank = 1;
        return;
    }

    struct stat buffer;
    int file_check = stat(data_xml, &buffer);
    if (file_check != 0)
        tw_error(TW_LOC, "Need to provide the appropriate XML metadata file for Damaris!");

    if ((err = damaris_initialize(data_xml, MPI_COMM_WORLD)) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, NULL);
    damaris_initialized = 1;

    // g_st_ross_rank > 0: ROSS MPI rank
    // g_st_ross_rank == 0: Damaris MPI rank
    if ((err = damaris_start(&g_st_ross_rank)) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, NULL);
    if(g_st_ross_rank) 
    {
        //get ROSS sub comm, sets to MPI_COMM_ROSS
        if ((err = damaris_client_comm_get(&ross_comm)) != DAMARIS_OK) 
            st_damaris_error(TW_LOC, err, NULL);
        tw_comm_set(ross_comm);
        
        // now make sure we have the correct rank number for ROSS ranks
        if (MPI_Comm_rank(MPI_COMM_ROSS, &my_rank) != MPI_SUCCESS)
            tw_error(TW_LOC, "Cannot get MPI_Comm_rank(MPI_COMM_ROSS)");

        g_tw_mynode = my_rank;

        if (g_st_opt_debug)
            opt_debug_init();

        //if (g_st_opt_debug)
        //{
        //    // need to first write our arg data to damaris
        //    if ((err = damaris_write("opt_debug/num_sim_args", &num_sim_args)) != DAMARIS_OK)
        //        st_damaris_error(TW_LOC, err, "opt_debug/num_sim_args");
        //    
        //    if ((err = damaris_write("opt_debug/sim_args", &arg_string[0])) != DAMARIS_OK)
        //        st_damaris_error(TW_LOC, err, "opt_debug/sim_args");

        //    // now send our signal to init debug mode plugin
        //    if ((err = damaris_signal("opt_debug_init")) != DAMARIS_OK)
        //        st_damaris_error(TW_LOC, err, "opt_debug_init");

        //    //if ((err = damaris_end_iteration()) != DAMARIS_OK)
        //    //    st_damaris_error(TW_LOC, err, "end iteration");
        //    
        //}
    }

}

void st_damaris_init_print()
{
    int total_size;
    MPI_Comm_size(MPI_COMM_WORLD, &total_size);

    printf("\nDamaris Configuration: \n");
    printf("\t%-50s %11u\n", "Total Ranks", total_size - tw_nnodes());
}

void st_damaris_inst_init()
{
    int err, i, j;
    double *dummy;

    if (g_st_opt_debug)
        return;

    set_parameters();

    // each pe needs to only write the coordinates for which it will be 
    // writing variables.
    // PE coordinates - just need pe_id and pe_id + 1 to allow for zonal coloring
    // on the mesh
    pe_id = tw_calloc(TW_LOC, "damaris", sizeof(double), 2);
    pe_id[0] = (double) g_tw_mynode;
    pe_id[1] = (double) g_tw_mynode + 1;
    if ((err = damaris_write("ross/pe_id", pe_id)) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, "ross/pe_id");

    // just a dummy variable to allow for creating a mesh of PE stats
    dummy = tw_calloc(TW_LOC, "damaris", sizeof(double), 2);
    dummy[0] = 0;
    dummy[1] = 1;
    if ((err = damaris_write("ross/dummy", dummy)) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, "ross/dummy");

    // LP coordinates in local ids
    lp_id = tw_calloc(TW_LOC, "damaris", sizeof(double), g_tw_nlp + 1);
    for (i = 0; i < g_tw_nlp + 1; i++)
        lp_id[i] = (double) i;
    if ((err = damaris_write("ross/lp_id", lp_id)) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, "ross/lp_id");

    // KP coordinates in local ids
    kp_id = tw_calloc(TW_LOC, "damaris", sizeof(double), g_tw_nkp + 1);
    for (i = 0; i < g_tw_nkp + 1; i++)
        kp_id[i] = (double) i;
    if ((err = damaris_write("ross/kp_id", kp_id)) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, "ross/kp_id");

    // calloc the space for the variables we want to track
    if (g_st_pe_data)
    {
        for (i = 0; i <  NUM_PE_VARS; i++)
        {
            pe_data[i].var_path = tw_calloc(TW_LOC, "damaris", sizeof(char), 1024);
            pe_data[i].data_ptr = tw_calloc(TW_LOC, "damaris", sizeof(void *), 1);
            if (i < DPE_EFF)
                pe_data[i].data_ptr[0] = tw_calloc(TW_LOC, "damaris", sizeof(int), 1);
            else
                pe_data[i].data_ptr[0] = tw_calloc(TW_LOC, "damaris", sizeof(float), 1);
        }
    }

    if (g_st_kp_data)
    {
        for (i = 0; i <  NUM_KP_VARS; i++)
        {
            kp_data[i].var_path = tw_calloc(TW_LOC, "damaris", sizeof(char), 1024);
            kp_data[i].data_ptr = tw_calloc(TW_LOC, "damaris", sizeof(void *), g_tw_nkp);
            for (j = 0; j < g_tw_nkp; j++)
            {
                if (i < DKP_VT_DIFF)
                    kp_data[i].data_ptr[j] = tw_calloc(TW_LOC, "damaris", sizeof(int), 1);
                else
                    kp_data[i].data_ptr[j] = tw_calloc(TW_LOC, "damaris", sizeof(float), 1);
            }
        }
    }

    if (g_st_lp_data)
    {
        for (i = 0; i <  NUM_LP_VARS; i++)
        {
            lp_data[i].var_path = tw_calloc(TW_LOC, "damaris", sizeof(char), 1024);
            lp_data[i].data_ptr = tw_calloc(TW_LOC, "damaris", sizeof(void *), g_tw_nlp);
            for (j = 0; j < g_tw_nlp; j++)
            {
                if (i < DLP_EFF)
                    lp_data[i].data_ptr[j] = tw_calloc(TW_LOC, "damaris", sizeof(int), 1);
                else
                    lp_data[i].data_ptr[j] = tw_calloc(TW_LOC, "damaris", sizeof(float), 1);
            }
        }
    }
    //printf ("PE %ld finished writing and callocing initial mesh and pe data\n", g_tw_mynode);
}

/**
 * @brief Shuts down Damaris and calls MPI_Finalize
 *
 * ROSS ranks (But not damaris ranks) have to first call damaris_stop()
 */
void st_damaris_ross_finalize()
{
    if (!damaris_initialized)
        return;
    if (g_st_ross_rank)
        damaris_stop();
    //if (g_st_real_time_samp)
    //    printf("Rank %ld: Max blocks counted for Real Time instrumentation is %d\n", g_tw_mynode, max_block_counter);
    int err;
    if ((err = damaris_finalize()) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, NULL);
    if (MPI_Finalize() != MPI_SUCCESS)
      tw_error(TW_LOC, "Failed to finalize MPI");
}

/**
 * @brief Expose GVT-based instrumentation data to Damaris
 */
void st_damaris_expose_data(tw_pe *me, tw_stime gvt, int inst_type)
{
    int err, i;

    tw_statistics s;
    bzero(&s, sizeof(s));
    tw_get_stats(me, &s);

    // collect data for each entity
    if (g_st_pe_data)
        expose_pe_data(me, &s, inst_type);
    if (g_st_kp_data)
        expose_kp_data(me, inst_type);
    if (g_st_lp_data)
        expose_lp_data(inst_type);

    if (inst_type == RT_COL)
        rt_block_counter++;
    if (inst_type == ANALYSIS_LP)
        vt_block_counter++;

    memcpy(&last_pe_stats[inst_type], &s, sizeof(tw_statistics));

    //if ((err = damaris_signal("test")) != DAMARIS_OK)
    //    st_damaris_error(TW_LOC, err, "test");
}

/**
 * @brief expose PE data to Damaris
 */
static void expose_pe_data(tw_pe *pe, tw_statistics *s, int inst_type)
{
    int err, i, block = 0;

    *((int*)pe_data[DPE_EVENT_PROC].data_ptr) = (int)( s->s_nevent_processed-last_pe_stats[inst_type].s_nevent_processed);
    *((int*)pe_data[DPE_EVENT_ABORT].data_ptr) = (int)(s->s_nevent_abort-last_pe_stats[inst_type].s_nevent_abort);
    *((int*)pe_data[DPE_E_RBS].data_ptr) = (int)(s->s_e_rbs-last_pe_stats[inst_type].s_e_rbs);
    *((int*)pe_data[DPE_RB_TOTAL].data_ptr) = (int)( s->s_rb_total-last_pe_stats[inst_type].s_rb_total);
    *((int*)pe_data[DPE_RB_SEC].data_ptr) = (int)(s->s_rb_secondary-last_pe_stats[inst_type].s_rb_secondary);
    *((int*)pe_data[DPE_RB_PRIM].data_ptr) = *((int*)pe_data[DPE_RB_TOTAL].data_ptr) - *((int*)pe_data[DPE_RB_SEC].data_ptr);
    *((int*)pe_data[DPE_FC_ATTEMPT].data_ptr) = (int)(s->s_fc_attempts-last_pe_stats[inst_type].s_fc_attempts);
    *((int*)pe_data[DPE_PQ_SIZE].data_ptr) = tw_pq_get_size(pe->pq);
    *((int*)pe_data[DPE_NET_SEND].data_ptr) = (int)(s->s_nsend_network-last_pe_stats[inst_type].s_nsend_network);
    *((int*)pe_data[DPE_NET_RECV].data_ptr) = (int)(s->s_nread_network-last_pe_stats[inst_type].s_nread_network);
    *((int*)pe_data[DPE_NGVTS].data_ptr) = (int)(g_tw_gvt_done - last_pe_stats[inst_type].s_ngvts);
    *((int*)pe_data[DPE_EVENT_TIES].data_ptr) = (int)(s->s_pe_event_ties-last_pe_stats[inst_type].s_pe_event_ties);

    int net_events = *((int*)pe_data[DPE_EVENT_PROC].data_ptr) - *((int*)pe_data[DPE_E_RBS].data_ptr);
    if (net_events > 0)
        *((float*)pe_data[DPE_EFF].data_ptr) = (float) 100.0 * (1.0 - ((float)*((int*)pe_data[DPE_E_RBS].data_ptr) /
                    (float)net_events));
    else
        *((float*)pe_data[DPE_EFF].data_ptr) = 0;

    *((float*)pe_data[DPE_NET_READ_TIME].data_ptr) = (float)(pe->stats.s_net_read - last_pe_stats[inst_type].s_net_read) / g_tw_clock_rate;
    *((float*)pe_data[DPE_GVT_TIME].data_ptr) = (float)(pe->stats.s_gvt - last_pe_stats[inst_type].s_gvt) / g_tw_clock_rate;
    *((float*)pe_data[DPE_FC_TIME].data_ptr) = (float)(pe->stats.s_fossil_collect - last_pe_stats[inst_type].s_fossil_collect) / g_tw_clock_rate;
    *((float*)pe_data[DPE_EVENT_ABORT_TIME].data_ptr) = (float)(pe->stats.s_event_abort - last_pe_stats[inst_type].s_event_abort) / g_tw_clock_rate;
    *((float*)pe_data[DPE_EVENT_PROC_TIME].data_ptr) = (float)(pe->stats.s_event_process - last_pe_stats[inst_type].s_event_process) / g_tw_clock_rate;
    *((float*)pe_data[DPE_PQ_TIME].data_ptr) = (float)(pe->stats.s_pq - last_pe_stats[inst_type].s_pq) / g_tw_clock_rate;
    *((float*)pe_data[DPE_RB_TIME].data_ptr) = (float)(pe->stats.s_rollback - last_pe_stats[inst_type].s_rollback) / g_tw_clock_rate;
    *((float*)pe_data[DPE_CANQ_TIME].data_ptr) = (float)(pe->stats.s_cancel_q - last_pe_stats[inst_type].s_cancel_q) / g_tw_clock_rate;
    *((float*)pe_data[DPE_AVL_TIME].data_ptr) = (float)(pe->stats.s_avl - last_pe_stats[inst_type].s_avl) / g_tw_clock_rate;

    if (inst_type == RT_COL)
        block = rt_block_counter;
    if (inst_type == ANALYSIS_LP)
        block = vt_block_counter;
    // let damaris know we're done updating data
    for (i = 0; i < NUM_PE_VARS; i++)
    {
        sprintf(pe_data[i].var_path, "ross/pes/%s/%s", inst_path[inst_type], pe_var_names[i]);
        if ((err = damaris_write_block(pe_data[i].var_path, block, pe_data[i].data_ptr)) != DAMARIS_OK)
        {
            printf("error: vt_block_counter %d\n", vt_block_counter);
            st_damaris_error(TW_LOC, err, pe_data[i].var_path);

        }
        //damaris_commit(pe_data[i].var_path);
        //damaris_clear(pe_data[i].var_path);
    }
    //printf("PE %ld damaris_write finished\n", g_tw_mynode);
}

/**
 * @brief expose KP data to Damaris
 */
static void expose_kp_data(tw_pe *pe, int inst_type)
{
    int i, err, block = 0;
    tw_kp *kp;

    for (i = 0; i < g_tw_nkp; i++)
    {
        kp = tw_getkp(i);

        *((int*)kp_data[DKP_EVENT_PROC].data_ptr[i]) = (int)(kp->kp_stats->s_nevent_processed - kp->last_stats[inst_type]->s_nevent_processed);
        *((int*)kp_data[DKP_EVENT_ABORT].data_ptr[i]) = (int)(kp->kp_stats->s_nevent_abort - kp->last_stats[inst_type]->s_nevent_abort);
        *((int*)kp_data[DKP_E_RBS].data_ptr[i]) = (int)(kp->kp_stats->s_e_rbs - kp->last_stats[inst_type]->s_e_rbs);
        *((int*)kp_data[DKP_RB_TOTAL].data_ptr[i]) = (int)(kp->kp_stats->s_rb_total - kp->last_stats[inst_type]->s_rb_total);
        *((int*)kp_data[DKP_RB_SEC].data_ptr[i]) = (int)(kp->kp_stats->s_rb_secondary - kp->last_stats[inst_type]->s_rb_secondary);
        *((int*)kp_data[DKP_RB_PRIM].data_ptr[i]) = *((int*)kp_data[DKP_RB_TOTAL].data_ptr[i]) - *((int*)kp_data[DKP_RB_SEC].data_ptr[i]);
        *((int*)kp_data[DKP_NET_SEND].data_ptr[i]) = (int)(kp->kp_stats->s_nsend_network - kp->last_stats[inst_type]->s_nsend_network);
        *((int*)kp_data[DKP_NET_RECV].data_ptr[i]) = (int)(kp->kp_stats->s_nread_network - kp->last_stats[inst_type]->s_nread_network);
        *((float*)kp_data[DKP_VT_DIFF].data_ptr[i]) = (float)(kp->last_time - pe->GVT);

        int net_events = *((int*)kp_data[DKP_EVENT_PROC].data_ptr[i]) - *((int*)kp_data[DKP_E_RBS].data_ptr[i]);
        if (net_events > 0)
            *((float*)kp_data[DKP_EFF].data_ptr[i]) = (float) 100.0 * (1.0 - ((float) *((int*)kp_data[DKP_E_RBS].data_ptr[i]) / (float) net_events));
        else
            *((float*)kp_data[DKP_EFF].data_ptr[i]) = 0;

        memcpy(kp->last_stats[inst_type], kp->kp_stats, sizeof(st_kp_stats));
    }

    if (inst_type == RT_COL)
        block = rt_block_counter;
    if (inst_type == ANALYSIS_LP)
        block = vt_block_counter;
    // let damaris know we're done updating data
    for (i = 0; i < NUM_KP_VARS; i++)
    {
        sprintf(kp_data[i].var_path, "ross/kps/%s/%s", inst_path[inst_type], kp_var_names[i]);
        if ((err = damaris_write_block(kp_data[i].var_path, block, kp_data[i].data_ptr[0])) != DAMARIS_OK)
            st_damaris_error(TW_LOC, err, kp_data[i].var_path);
    }
}

/**
 * @brief expose LP data to Damaris
 */
static void expose_lp_data(int inst_type)
{
    tw_lp *lp;
    int i, err, block = 0;

    for (i = 0; i < g_tw_nlp; i++)
    {
        lp = tw_getlp(i);

        *((int*)lp_data[DLP_EVENT_PROC].data_ptr[i]) = (int)(lp->lp_stats->s_nevent_processed - lp->last_stats[inst_type]->s_nevent_processed);
        *((int*)lp_data[DLP_EVENT_ABORT].data_ptr[i]) = (int)(lp->lp_stats->s_nevent_abort - lp->last_stats[inst_type]->s_nevent_abort);
        *((int*)lp_data[DLP_E_RBS].data_ptr[i]) = (int)(lp->lp_stats->s_e_rbs - lp->last_stats[inst_type]->s_e_rbs);
        *((int*)lp_data[DLP_NET_SEND].data_ptr[i]) = (int)(lp->lp_stats->s_nsend_network - lp->last_stats[inst_type]->s_nsend_network);
        *((int*)lp_data[DLP_NET_RECV].data_ptr[i]) = (int)(lp->lp_stats->s_nread_network - lp->last_stats[inst_type]->s_nread_network);

        int net_events = *((int*)lp_data[DLP_EVENT_PROC].data_ptr[i]) - *((int*)lp_data[DLP_E_RBS].data_ptr[i]);
        if (net_events > 0)
            *((float*)lp_data[DLP_EFF].data_ptr[i]) = (float) 100.0 * (1.0 - ((float) *((int*)lp_data[DLP_E_RBS].data_ptr[i]) / (float) net_events));
        else
            *((float*)lp_data[DLP_EFF].data_ptr[i]) = 0;

        memcpy(lp->last_stats[inst_type], lp->lp_stats, sizeof(st_lp_stats));
    }

    if (inst_type == RT_COL)
        block = rt_block_counter;
    if (inst_type == ANALYSIS_LP)
        block = vt_block_counter;
    // let damaris know we're done updating data
    for (i = 0; i < NUM_LP_VARS; i++)
    {
        sprintf(lp_data[i].var_path, "ross/lps/%s/%s", inst_path[inst_type], lp_var_names[i]);
        if ((err = damaris_write_block(lp_data[i].var_path, block, lp_data[i].data_ptr[0])) != DAMARIS_OK)
            st_damaris_error(TW_LOC, err, lp_data[i].var_path);
    }
}

/** 
 * @brief
 */
static void reset_block_counter(int *counter)
{
    if (*counter > max_block_counter)
        max_block_counter = *counter;
    *counter = 0;
}

/**
 * @brief
 */
void st_damaris_end_iteration()
{
    int err;

    //if ((err = damaris_signal("damaris_gc")) != DAMARIS_OK)
    //    st_damaris_error(TW_LOC, err, "damaris_gc");

    if ((err = damaris_end_iteration()) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, "end iteration");

    iterations++;
    //if (vt_block_counter > 0)
    //    printf("PE %ld end iteration #%d vt_block_counter = %d\n", g_tw_mynode, iterations, vt_block_counter);
    
    reset_block_counter(&rt_block_counter);
    reset_block_counter(&vt_block_counter);
}

tw_stime st_damaris_opt_debug_sync(tw_pe *pe)
{
    //printf("ross full rank %d reached barrier\n", full_ross_rank);
    if (MPI_Barrier(MPI_COMM_ROSS_FULL) != MPI_SUCCESS)
        tw_error(TW_LOC, "Failed to wait for MPI_Barrier");

    // now let seq rank know the GVT of the opt run
    current_gvt = pe->GVT;
    MPI_Bcast(&current_gvt, 1, MPI_DOUBLE, 0, MPI_COMM_ROSS_FULL);
    return current_gvt;
}

/**
 * @brief Make Damaris error checking easier.
 *
 * Some errors will stop simulation, but some will only warn and keep going. 
 */
void st_damaris_error(const char *file, int line, int err, char *variable)
{
    switch(err)
    {
        case DAMARIS_ALLOCATION_ERROR:
            tw_warning(file, line, "Damaris allocation error for variable %s at GVT %f\n", variable, g_tw_pe[0]->GVT);
            break;
        case DAMARIS_ALREADY_INITIALIZED:
            tw_warning(file, line, "Damaris was already initialized\n");
            break;
        case DAMARIS_BIND_ERROR:
            tw_error(file, line, "Damaris bind error for Damaris event:  %s\n", variable);
            break;
        case DAMARIS_BLOCK_NOT_FOUND:
            tw_error(file, line, "Damaris not able to find block for variable:  %s\n", variable);
            break;
        case DAMARIS_CORE_IS_SERVER:
            tw_error(file, line, "This node is not a client.\n");
            break;
        case DAMARIS_DATASPACE_ERROR:
            tw_error(file, line, "Damaris dataspace error for variable:  %s\n", variable);
            break;
        case DAMARIS_INIT_ERROR:
            tw_error(file, line, "Error calling damaris_init().\n");
            break;
        case DAMARIS_FINALIZE_ERROR:
            tw_error(file, line, "Error calling damaris_finalize().\n");
            break;
        case DAMARIS_INVALID_BLOCK:
            tw_error(file, line, "Damaris invalid block for variable:  %s\n", variable);
            break;
        case DAMARIS_NOT_INITIALIZED:
            tw_warning(file, line, "Damaris has not been initialized\n");
            break;
        case DAMARIS_UNDEFINED_VARIABLE:
            tw_error(file, line, "Damaris variable %s is undefined\n", variable);
            break;
        case DAMARIS_UNDEFINED_ACTION:
            tw_error(file, line, "Damaris action %s is undefined\n", variable);
            break;
        case DAMARIS_UNDEFINED_PARAMETER:
            tw_error(file, line, "Damaris parameter %s is undefined\n", variable);
            break;
        default:
            tw_error(file, line, "Damaris error unknown. %s \n", variable);
    }
}

