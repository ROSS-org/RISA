#include <ross.h>
#include "damaris.h"
#include <dlfcn.h>

/**
 * @file opt-debug.c
 * @brief Handles the ROSS side of the optimistic debugger plugin for Damaris
 */

MPI_Comm MPI_COMM_ROSS_FULL; // only for use with optimistic debug mode
static int full_ross_rank = -1; // rank within MPI_COMM_ROSS_FULL
static tw_stime current_gvt = 0.0;
static tw_stime prev_gvt = 0.0;
static void opt_debug_pe_data(tw_pe *pe);
static int first_iteration = 1;
static int initialized = 0;
static int event_block = 0;
static char **event_handlers;
static int event_handlers_idx = 0;
static int *event_map = NULL;
int g_st_debug_enabled = 0;

#define MAX_EVENT_HANDLERS 10
#define MAX_HANDLER_LEN 256

void opt_debug_expose_data(tw_pe *pe);
static void opt_debug_lp_data();

/**
 * @brief Initialize the optimistic debugger
 *
 * To be called by st_damaris_ross_init() if debugger is turned on.
 * Once Damaris has pulled its necessary ranks, we set the returned communicator to be
 * MPI_COMM_ROSS_FULL, so we can continue to do collectives with all ROSS ranks.
 * But now we split ROSS ranks further for optimistic debugging. One rank will
 * run a sequential version, while the remaining ranks will run the optimistic version.
 */
void opt_debug_init()
{
    int my_rank, current_ross_size, sub_size, i;
    MPI_Comm new_ross_comm, debug_comm;

    // MPI_COMM_ROSS controls all non-damaris ranks at this point
    MPI_Comm_size(MPI_COMM_ROSS, &current_ross_size);

    // save our full ROSS communicator and ranks
    MPI_COMM_ROSS_FULL = MPI_COMM_ROSS;
    full_ross_rank = g_tw_mynode;
    //printf("opt_debug_init full world size is %d\n", current_ross_size);
    
    if (!g_st_rng_check)
    {
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

            g_tw_synchronization_protocol = SEQUENTIAL;
            //freopen("redir.txt", "w", stdout); // redirect this ranks output to file for now
            freopen("/dev/null", "w", stdout); // redirect this ranks output to file for now
        }
    }

    // now make sure we have the correct rank number for our version of MPI_COMM_ROSS
    if (MPI_Comm_rank(MPI_COMM_ROSS, &my_rank) != MPI_SUCCESS)
        tw_error(TW_LOC, "Cannot get MPI_Comm_rank(MPI_COMM_ROSS)");
    g_tw_mynode = my_rank;

    MPI_Comm_size(MPI_COMM_ROSS, &sub_size);
    printf("I am rank %ld (full_ross_rank %d) with comm size of %d\n", g_tw_mynode, full_ross_rank, sub_size);
    st_inst_set_debug();
    st_set_gvt_print_interval(1.0); // turn off normal ROSS mid-simulation output

    event_handlers = tw_calloc(TW_LOC, "damaris", sizeof(char*), MAX_EVENT_HANDLERS);
    for (i = 0; i < MAX_EVENT_HANDLERS; i++)
        event_handlers[i] = tw_calloc(TW_LOC, "damaris", sizeof(char), MAX_HANDLER_LEN);

    g_st_debug_enabled = 1;
    printf("opt_debug_init() end\n");
}

/**
 * @brief This collects event handlers for providing detailed debugging information.
 *
 * This function needs to be called for each LP from the tw_lp_settype() function.
 */
void st_damaris_opt_debug_map(event_f handler, tw_lpid id)
{
    if (g_tw_synchronization_protocol != SEQUENTIAL)
        return;

    // TODO error checking
    int i, found = -1;
    //dlopen("/home/rossc3/cv-build/install/lib/libcodes.so", RTLD_NOW | RTLD_LOCAL);
    Dl_info info;
    dladdr(handler, &info);
    //printf("test %s \n", info.dli_sname);

    if (event_map == NULL)
        event_map = tw_calloc(TW_LOC, "damaris", sizeof(int), g_tw_nlp);

    if (info.dli_sname)
    {
        for (i = 0; i < event_handlers_idx; i++)
        {
            if (strcmp(info.dli_sname, event_handlers[i]) == 0)
            {
                found = i;
                break;
            }
        }

        if (found == -1)
        {
            strcpy(event_handlers[event_handlers_idx], info.dli_sname);
            found = event_handlers_idx;
            event_handlers_idx++;
        }
    }

    // found == position in event_handlers
    // unless we couldn't find the symbol, then it's a -1
    event_map[id] = found;
    //printf("lpid %ld event_map[id] = %d\n", id, event_map[id]);

}

/**
 * @brief Pass all setup data to Damaris
 */
void st_damaris_expose_setup_data()
{
    int err, i, idx = 0;
    char handler_str[MAX_EVENT_HANDLERS * MAX_HANDLER_LEN];

    if ((err = damaris_parameter_set("total_lp", &g_tw_nlp, sizeof(g_tw_nlp))) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, "total_lp");

    if ((err = damaris_parameter_set("num_rng", &g_tw_nRNG_per_lp, sizeof(g_tw_nRNG_per_lp))) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, "num_rng");

    if ((err = damaris_write("nlp", &g_tw_nlp)) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, "nlp");

    if ((err = damaris_write("sync", &g_tw_synchronization_protocol)) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, "sync");

    if ((err = damaris_write("rng_check", &g_st_rng_check)) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, "rng_check");

    if ((err = damaris_write("num_rngs", &g_tw_nRNG_per_lp)) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, "num_rngs");

    // only need the following data from one rank
    // sequential rank is the easiest
    if (g_tw_synchronization_protocol == SEQUENTIAL)
    {
        if ((err = damaris_write("lp_types", event_map)) != DAMARIS_OK)
            st_damaris_error(TW_LOC, err, "lp_types");

        for (i = 0; i < event_handlers_idx; i++)
        {
            printf("event handlers[%d] %s\n", i, event_handlers[i]);
            sprintf(&handler_str[idx], "%s", event_handlers[i]);
            idx += strlen(event_handlers[i]);
            handler_str[idx] = ' ';
            idx++;
        }
        handler_str[idx] = '\0';

        if ((err = damaris_write("num_types", &event_handlers_idx)) != DAMARIS_OK)
            st_damaris_error(TW_LOC, err, "num_types");

        if ((err = damaris_write("lp_types_str", &handler_str[0])) != DAMARIS_OK)
            st_damaris_error(TW_LOC, err, "lp_types_str");

    }
    if (g_tw_mynode == g_tw_masternode)
    {
        if (g_st_opt_debug)
            printf("***** STARTING OPTIMISTIC DEBUGGER WITH DAMARIS *****\n");
        if (g_st_rng_check)
            printf("***** STARTING OPTIMISTIC RNG DEBUGGER WITH DAMARIS *****\n");
    }
}

int it_no = 0;

void st_damaris_debug_end_iteration(tw_pe *pe)
{
    if (g_st_opt_debug)
        st_damaris_opt_debug_sync(pe);
    else if (g_st_rng_check)
        st_damaris_rng_check_end_iteration(pe);
}

/**
 * @brief Used to synchronize optimistic and sequential simulation runs
 */
tw_stime st_damaris_opt_debug_sync(tw_pe *pe)
{
    int err;

    if (g_tw_synchronization_protocol == SEQUENTIAL && !first_iteration
            && current_gvt != prev_gvt)
        st_damaris_opt_debug_data(pe);

    first_iteration = 0;
    prev_gvt = current_gvt;

    if (MPI_Barrier(MPI_COMM_ROSS_FULL) != MPI_SUCCESS)
        tw_error(TW_LOC, "Failed to wait for MPI_Barrier");

    // now let seq rank know the GVT of the opt run
    current_gvt = pe->GVT;
    MPI_Bcast(&current_gvt, 1, MPI_DOUBLE, 0, MPI_COMM_ROSS_FULL);

    //printf("[GVT: %f, it: %d] rank %ld (sync=%d) finished Barrier and Bcast\n", 
    //        current_gvt, it_no, g_tw_mynode, g_tw_synchronization_protocol);
    if (g_tw_synchronization_protocol == OPTIMISTIC &&
            current_gvt != prev_gvt && current_gvt < DBL_MAX)
        st_damaris_opt_debug_data(pe);

    return current_gvt;
}

/**
 * @brief Wrap up a Damaris iteration for the RNG Check component
 */
void st_damaris_rng_check_end_iteration(tw_pe *pe)
{
    int err;
    opt_debug_pe_data(pe);
    st_damaris_end_iteration();
    event_block = 0;
    if (!initialized)
    { // here in order to ensure all ROSS ranks have gotten their values in
        if ((err = damaris_signal("rng_check_setup")) != DAMARIS_OK)
            st_damaris_error(TW_LOC, err, "rng_check_setup");
        initialized = 1;
    }
    if ((err = damaris_signal("rng_check_event")) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, "rng_check_event");
    
}

/**
 * @brief Collecting optimistic debug data
 */
void st_damaris_opt_debug_data(tw_pe *pe)
{
    int err;

    //printf("[GVT: %f, it: %d] rank %ld (sync=%d) has committed %llu events\n", 
    //        pe->GVT, it_no, g_tw_mynode, g_tw_synchronization_protocol, pe->stats.s_committed_events);

    opt_debug_expose_data(pe);
    st_damaris_end_iteration();
    event_block = 0;
    if (!initialized)
    { // here in order to ensure all ROSS ranks have gotten their values in
        if ((err = damaris_signal("opt_debug_setup")) != DAMARIS_OK)
            st_damaris_error(TW_LOC, err, "opt_debug_setup");
        initialized = 1;
    }
    if ((err = damaris_signal("opt_debug")) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, "opt_debug");
    it_no++;
}

/**
 * @brief wrapper for exposing the appropriate data to Damaris
 */
void opt_debug_expose_data(tw_pe *pe)
{
    int err, i;

    tw_statistics s;
    bzero(&s, sizeof(s));
    tw_get_stats(pe, &s);

    // collect data for each entity
    if (g_st_pe_data)
        opt_debug_pe_data(pe);
    if (g_st_lp_data)
        opt_debug_lp_data();

}

/**
 * @brief Expose PE opt debugging data
 */
static void opt_debug_pe_data(tw_pe *pe)
{
    int err, block = 0;
    float gvt = (float)pe->GVT;

    // let damaris know we're done updating data
    if ((err = damaris_write_block("current_gvt", block, &gvt)) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, "current_gvt");
}

/**
 * @brief Expose LP opt debugging data
 */
static void opt_debug_lp_data()
{
    int err, i, block = 0;
    int committed_ev[g_tw_nlp];
    
    for (i = 0; i < g_tw_nlp; i++)
        committed_ev[i] = (int)g_tw_lp[i]->lp_stats->committed_events;

    // let damaris know we're done updating data
    if ((err = damaris_write_block("ross/lps/gvt_inst/committed_events", block, &committed_ev[0])) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, "ross/lps/gvt_inst/committed_events");
}

/**
 * @brief Collects forward event data and calls event
 *
 * for optimistic debugging, do our event/commit calls here so we can do some
 * processing before and after the call, without having to worry about multiple
 * function calls in ROSS
 */
void st_damaris_call_event(tw_event *cev, tw_lp *clp)
{
    //if (!cev->rng_count_before)
    //    cev->rng_count_before = tw_calloc(TW_LOC, "damaris", sizeof(unsigned long), g_tw_nRNG_per_lp);
    //if (!cev->rng_count_end)
    //    cev->rng_count_end = tw_calloc(TW_LOC, "damaris", sizeof(unsigned long), g_tw_nRNG_per_lp);
    
    int i;
    for (i = 0; i < g_tw_nRNG_per_lp; i++)
        cev->rng_count_before[i] = clp->rng[i].count;

    (*clp->type->event)(clp->cur_state, &cev->cv, tw_event_data(cev), clp);

    for (i = 0; i < g_tw_nRNG_per_lp; i++)
        cev->rng_count_end[i] = clp->rng[i].count;

    if (g_tw_synchronization_protocol == OPTIMISTIC)
        st_damaris_expose_event_data(cev, "ross/fwd_event");
    else
        st_damaris_expose_event_data(cev, "ross/seq_event");

}

/**
 * @brief Collects reverse event data and calls event
 */
void st_damaris_call_rev_event(tw_event *cev, tw_lp *clp)
{
    int i;
    for (i = 0; i < g_tw_nRNG_per_lp; i++)
        cev->rng_count_before[i] = clp->rng[i].count;

    (*clp->type->revent)(clp->cur_state, &cev->cv, tw_event_data(cev), clp);

    for (i = 0; i < g_tw_nRNG_per_lp; i++)
        cev->rng_count_end[i] = clp->rng[i].count;

    st_damaris_expose_event_data(cev, "ross/rev_event");

}

/**
 * @brief exposing event data to Damaris
 */
void st_damaris_expose_event_data(tw_event *e, const char *prefix)
{
    if (g_st_rng_check && strcmp(prefix, "ross/opt_event") == 0)
        return;
    int lpid, err, i;
    long event_id;
    float ts;
    char varname[1024];
    long rng_count_before[g_tw_nRNG_per_lp];
    long rng_count_end[g_tw_nRNG_per_lp];

    //printf("pe %ld (sync=%d) src_lp %d dest_lp %d recv_ts %f\n",
    //        g_tw_mynode, g_tw_synchronization_protocol, (int)e->send_lp,
    //        (int)e->dest_lp->gid, (float)e->recv_ts);
    //printf("%s send pe %ld, event_id %u: %d, %d, %f, %f, %ld\n", prefix, e->send_pe, e->event_id, 
    //        (int)e->send_lp, (int)e->dest_lpid, e->send_ts, e->recv_ts, (long)e->rng_count[0]);

    lpid = (int)e->send_lp;
    sprintf(varname, "%s/src_lp", prefix);
    if ((err = damaris_write_block(varname, event_block, &lpid)) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, varname);

    lpid = (int)e->dest_lpid;
    sprintf(varname, "%s/dest_lp", prefix);
    if ((err = damaris_write_block(varname, event_block, &lpid)) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, varname);

    ts = (float)e->recv_ts;
    sprintf(varname, "%s/recv_ts", prefix);
    if ((err = damaris_write_block(varname, event_block, &ts)) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, varname);

    ts = (float)e->send_ts;
    sprintf(varname, "%s/send_ts", prefix);
    if ((err = damaris_write_block(varname, event_block, &ts)) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, varname);

    event_id = (long)e->event_id;
    sprintf(varname, "%s/event_id", prefix);
    if ((err = damaris_write_block(varname, event_block, &event_id)) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, varname);

    lpid = (int)e->send_pe;
    sprintf(varname, "%s/src_pe", prefix);
    if ((err = damaris_write_block(varname, event_block, &lpid)) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, varname);

    for (i = 0; i < g_tw_nRNG_per_lp; i++)
    {
        rng_count_before[i] = (long)(e->rng_count_before[i]);
        rng_count_end[i] = (long)(e->rng_count_end[i]);
    }
    sprintf(varname, "%s/rng_count_before", prefix);
    if ((err = damaris_write_block(varname, event_block, &rng_count_before[0])) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, varname);
    sprintf(varname, "%s/rng_count_end", prefix);
    if ((err = damaris_write_block(varname, event_block, &rng_count_end[0])) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, varname);
    
    char ev_buffer[128];
    int collect_flag = 1;
    sprintf(varname, "%s/ev_name", prefix);

    // try to get model event data
    if (e->dest_lp->model_types && e->dest_lp->model_types->ev_trace)
    {
        (*e->dest_lp->model_types->ev_trace)(tw_event_data(e), e->dest_lp, &ev_buffer[0], &collect_flag);
        if ((err = damaris_write_block(varname, event_block, &ev_buffer[0])) != DAMARIS_OK)
            st_damaris_error(TW_LOC, err, varname);
    }

    event_block++;
}

/**
 * @brief Signals to Damaris to call opt debug finalize event
 */
void st_damaris_opt_debug_finalize()
{
    int err;

    if (g_st_opt_debug)
    {
        if ((err = damaris_signal("opt_debug_finalize")) != DAMARIS_OK)
            st_damaris_error(TW_LOC, err, "opt_debug_finalize");
    }
    if (g_st_rng_check)
    {
        if ((err = damaris_signal("rng_check_finalize")) != DAMARIS_OK)
            st_damaris_error(TW_LOC, err, "rng_check_finalize");
    }
}

/**
 * @brief optimistic debugging specific output
 */
void st_damaris_opt_debug_init_print()
{
    int total_size;
    MPI_Comm_size(MPI_COMM_WORLD, &total_size);


    printf("\nDamaris Optimistic Debug Configuration: \n");
    printf("\t%-50s %11u\n", "Total Optimistic Ranks", tw_nnodes());
    printf("\t%-50s %11u\n", "Sequential Rank", 1);
    printf("\t%-50s %11u\n", "Damaris Ranks", total_size - tw_nnodes() - 1);
}
