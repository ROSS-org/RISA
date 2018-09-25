#include <ross.h>
#include <sys/stat.h>
#include <plugins/util/config-c.h>
#include <core/model-c.h>

/**
 * @file damaris.c
 * @brief ROSS integration with Damaris data management
 */
void st_damaris_signal_init(void);

int g_st_ross_rank = 0;

static int damaris_initialized = 0;
static double *pe_id, *kp_id, *lp_id;
static tw_statistics last_pe_stats[3];

static int rt_block_counter = 0;
static int vt_block_counter = 0;
static int max_block_counter = 0;
static int iterations = 0;

int g_st_damaris_enabled = 0;
static char data_xml[4096];

static const tw_optdef damaris_options[] = {
    TWOPT_GROUP("Damaris Integration"),
    TWOPT_UINT("enable-damaris", g_st_damaris_enabled, "Turn on (1) or off (0) Damaris in situ analysis"),
    TWOPT_CHAR("data-xml", data_xml, "Path to XML file for describing data to Damaris"),
    TWOPT_END()
};

/**
 * @brief Returns the damaris_options struct so ROSS can pull in the options
 */
const tw_optdef *st_damaris_opts(void)
{
    return damaris_options;
}

/**
 * @brief parse config file and set parameters
 */
void st_damaris_parse_config(const char *config_file)
{
    parse_file(config_file);
}

/**
 * @brief Sets up the simulation parameters needed by Damaris
 */
static void set_parameters(const char *config_file)
{
    int err;
    int num_pe = tw_nnodes();
    int num_kp = (int) g_tw_nkp;
    int num_lp = (int) g_tw_nlp;
    int model_sample_size = 524288;
    //printf("PE %ld num pe %d, num kp %d, num lp %d\n", g_tw_mynode, num_pe, num_kp, num_lp);

    if ((err = damaris_parameter_set("num_lp", &num_lp, sizeof(num_lp))) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, "num_lp");

    if ((err = damaris_parameter_set("num_pe", &num_pe, sizeof(num_pe))) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, "num_pe");

    if ((err = damaris_parameter_set("num_kp", &num_kp, sizeof(num_kp))) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, "num_kp");

    if ((err = damaris_parameter_set("sample_size", &model_sample_size, sizeof(model_sample_size))) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, "sample_size");

    if ((err = damaris_write("ross/nlp", &num_lp)) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, "num_lp");

    if ((err = damaris_write("ross/nkp", &num_kp)) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, "num_kp");

    if (config_file)
    {
        if ((err = damaris_write("ross/inst_config", &config_file[0])) != DAMARIS_OK)
            st_damaris_error(TW_LOC, err, "ross/inst_config");
    }

    st_damaris_signal_init();
}

/**
 * @brief Sets up ROSS to use Damaris
 *
 * This must be called after MPI is initialized. Damaris splits the MPI
 * communicator and we set MPI_COMM_ROSS to the subcommunicator returned by Damaris.
 * This sets the variable g_st_ross_rank to 1 on ROSS ranks and 0 on Damaris ranks.
 * Need to make sure that Damaris ranks return to model after this point.
 *
 */
void st_damaris_ross_init(void)
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
    }

}

/**
 * @brief Some initialization output to print when using Damaris.
 */
void st_damaris_init_print()
{
    int total_size;
    MPI_Comm_size(MPI_COMM_WORLD, &total_size);

    printf("\nDamaris Configuration: \n");
    printf("\t%-50s %11u\n", "Total Ranks", total_size - tw_nnodes());
}

/**
 * @brief Init for using Instrumentation with Damaris.
 */
void st_damaris_inst_init(const char *config_file)
{
    set_parameters(config_file);
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
    int err;
    if (g_st_ross_rank)
    {
        if ((err = damaris_signal("streaming_finalize")) != DAMARIS_OK)
            st_damaris_error(TW_LOC, err, "streaming_finalize");
        damaris_stop();
    }
    //if (g_st_real_time_samp)
    //    printf("Rank %ld: Max blocks counted for Real Time instrumentation is %d\n", g_tw_mynode, max_block_counter);
    if ((err = damaris_finalize()) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, NULL);
    if (MPI_Finalize() != MPI_SUCCESS)
      tw_error(TW_LOC, "Failed to finalize MPI");
}

/**
 * @brief Expose instrumentation data to Damaris
 */
void st_damaris_expose_data(tw_pe *me, int inst_type)
{
    int err;

    double real_ts = (double)tw_clock_read() / g_tw_clock_rate;
    st_damaris_start_sample(0.0, real_ts, me->GVT, inst_type);

    if (engine_modes[inst_type])
    {
        // collect data for each entity
        if (g_st_pe_data)
            st_damaris_sample_pe_data(me, &last_pe_stats[0], inst_type);
        if (g_st_kp_data)
            st_damaris_sample_kp_data(inst_type);
        if (g_st_lp_data)
            st_damaris_sample_lp_data(inst_type);
    }

    if (model_modes[inst_type])
    {
        st_damaris_sample_model_data();
    }

    st_damaris_finish_sample();

    if (inst_type == RT_INST)
        rt_block_counter++;
    if (inst_type == VT_INST)
        vt_block_counter++;
}

/**
 * @brief Resets block counters when ending a Damaris iteration.
 *
 * A PE may notify Damaris of multiple samples for a given variable within a single iteration.
 * Damaris refers to these as blocks or domains and needs to be passed the domain/block id
 * when there is more than one per iteration. This resets the counter variable to 0 when cleaning
 * up at the end of an iteration.
 */
static void reset_block_counter(int *counter)
{
    if (*counter > max_block_counter)
        max_block_counter = *counter;
    *counter = 0;
}

void st_damaris_signal_init()
{
    int err;
    //printf("rank %ld signaling setup_simulation_config\n", g_tw_mynode);
    if ((err = damaris_signal("setup_simulation_config")) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, "setup_simulation_config");
}

/**
 * @brief Signals to Damaris that the current iteration is over.
 *
 * Iterations should always end at GVT (though it doesn't have to be every GVT), because
 * the call to damaris_end_iteration() contains a collective call.
 */
void st_damaris_end_iteration()
{
    int err;
    if ((err = damaris_signal("damaris_gc")) != DAMARIS_OK)
        st_damaris_error(TW_LOC, err, "damaris_gc");

    if (g_tw_gvt_done % g_st_num_gvt == 0)
    {
        if ((err = damaris_end_iteration()) != DAMARIS_OK)
            st_damaris_error(TW_LOC, err, "end iteration");
        if ((err = damaris_signal("handle_data")) != DAMARIS_OK)
            st_damaris_error(TW_LOC, err, "handle_data");
    }

    iterations++;
    //if (vt_block_counter > 0)
    //    printf("PE %ld end iteration #%d vt_block_counter = %d\n", g_tw_mynode, iterations, vt_block_counter);

    reset_block_counter(&rt_block_counter);
    reset_block_counter(&vt_block_counter);
}

/**
 * @brief Make Damaris error checking easier.
 *
 * Some errors will stop simulation, but some will only warn and keep going.
 */
void st_damaris_error(const char *file, int line, int err, const char *variable)
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

