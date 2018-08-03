#include <ross.h>
#include <dlfcn.h>
//#include "damaris.h"

MPI_Comm MPI_COMM_ROSS_FULL; // only for use with optimistic debug mode
static int full_ross_rank = -1; // rank within MPI_COMM_ROSS_FULL
static tw_stime current_gvt = 0.0;
static tw_stime prev_gvt = 0.0;
static void opt_debug_pe_data(tw_pe *pe, tw_statistics *s);
static int first_iteration = 1;
static int initialized = 0;
static int event_block = 0;
static char **event_handlers;
static int event_handlers_idx = 0;
static int *event_map = NULL;

#define MAX_EVENT_HANDLERS 10
#define MAX_HANDLER_LEN 256

void opt_debug_expose_data(tw_pe *pe);
static void opt_debug_lp_data();

// take one of our ranks and make it run a seq sim separate from the optimistic run
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
        freopen("redir.txt", "w", stdout); // redirect this ranks output to file for now
    }

    // now make sure we have the correct rank number for our version of MPI_COMM_ROSS
    if (MPI_Comm_rank(MPI_COMM_ROSS, &my_rank) != MPI_SUCCESS)
        tw_error(TW_LOC, "Cannot get MPI_Comm_rank(MPI_COMM_ROSS)");
    g_tw_mynode = my_rank;

    MPI_Comm_size(MPI_COMM_ROSS, &sub_size);
    printf("I am rank %ld (full_ross_rank %d) with comm size of %d\n", g_tw_mynode, full_ross_rank, sub_size);
    st_inst_set_debug();

    event_handlers = tw_calloc(TW_LOC, "damaris", sizeof(char*), MAX_EVENT_HANDLERS);
    for (i = 0; i < MAX_EVENT_HANDLERS; i++)
        event_handlers[i] = tw_calloc(TW_LOC, "damaris", sizeof(char), MAX_HANDLER_LEN);
}

// needs to be called from tw_lp_settype()
void st_damaris_opt_debug_map(event_f handler, tw_lpid id)
{
    if (g_tw_synchronization_protocol != SEQUENTIAL)
        return;

    // TODO need to get event handler names to damaris plugin
    // TODO error checking
    int i, found = -1;
    dlopen("/home/rossc3/cv-build/install/lib/libcodes.so", RTLD_NOW | RTLD_LOCAL);
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

void st_damaris_expose_setup_data()
{
    int err, i, idx = 0;
    char handler_str[MAX_EVENT_HANDLERS * MAX_HANDLER_LEN];

    if ((err = damaris_parameter_set("total_lp", &g_tw_nlp, sizeof(g_tw_nlp))) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, "total_lp");

    if ((err = damaris_write("nlp", &g_tw_nlp)) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, "nlp");

    if ((err = damaris_write("sync", &g_tw_synchronization_protocol)) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, "sync");

    // only need the following data from one rank
    // sequential rank is the easiest
    if (g_tw_synchronization_protocol == SEQUENTIAL)
    {
        printf("about to send event_map data using damaris_write\n");
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
        printf("handler_str: %s\n", handler_str);

        if ((err = damaris_write("num_types", &event_handlers_idx)) != DAMARIS_OK)
            st_damaris_error(TW_LOC, err, "num_types");

        if ((err = damaris_write("lp_types_str", &handler_str[0])) != DAMARIS_OK)
            st_damaris_error(TW_LOC, err, "lp_types_str");
    }
}

int it_no = 0;
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

void opt_debug_expose_data(tw_pe *pe)
{
    int err, i;

    tw_statistics s;
    bzero(&s, sizeof(s));
    tw_get_stats(pe, &s);

    // collect data for each entity
    if (g_st_pe_data)
        opt_debug_pe_data(pe, &s);
    if (g_st_lp_data)
        opt_debug_lp_data();

}

static void opt_debug_pe_data(tw_pe *pe, tw_statistics *s)
{
    int err, i, block = 0;
    int committed_ev = (int)pe->stats.s_committed_events;
    float gvt = (float)pe->GVT;

    // let damaris know we're done updating data
    if ((err = damaris_write_block("ross/pes/gvt_inst/committed_events", block, &committed_ev)) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, "ross/pes/gvt_inst/committed_events");
    if ((err = damaris_write_block("current_gvt", block, &gvt)) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, "current_gvt");
}

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

void st_damaris_expose_event_data(tw_event *e)
{
    int lpid, err;
    float ts;
    char prefix[1024];
    char varname[1024];

    if (g_tw_synchronization_protocol == SEQUENTIAL)
        sprintf(prefix, "ross/seq_event");
    else if (g_tw_synchronization_protocol == OPTIMISTIC)
        sprintf(prefix, "ross/opt_event");

    //printf("pe %ld (sync=%d) src_lp %d dest_lp %d recv_ts %f\n",
    //        g_tw_mynode, g_tw_synchronization_protocol, (int)e->send_lp,
    //        (int)e->dest_lp->gid, (float)e->recv_ts);

    lpid = (int)e->send_lp;
    sprintf(varname, "%s/src_lp", prefix);
    if ((err = damaris_write_block(varname, event_block, &lpid)) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, varname);

    lpid = (int)e->dest_lp->gid;
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

    event_block++;
}

void st_damaris_opt_debug_finalize()
{
    int err;

    if ((err = damaris_signal("opt_debug_finalize")) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, "opt_debug_finalize");
}
