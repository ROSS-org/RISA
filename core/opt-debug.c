#include <ross.h>
#include "damaris.h"

MPI_Comm MPI_COMM_ROSS_FULL; // only for use with optimistic debug mode
static int full_ross_rank = -1; // rank within MPI_COMM_ROSS_FULL
static tw_stime current_gvt = 0;

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

