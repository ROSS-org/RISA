#include <ross.h>
#include <Damaris.h>

/**
 * @file damaris.h
 * @brief API for ROSS-Damaris integration
 */

typedef struct model_lp_metadata model_lp_metadata;
typedef struct mlp_var_metadata mlp_var_metadata;

struct model_lp_metadata
{
    int peid;
    int kpid;
    int lpid;
    int num_vars;
    size_t name_sz;
};

struct mlp_var_metadata
{
    size_t name_sz;
    int id;
};

extern int g_st_ross_rank;          /**< @brief 1 if ROSS PE, 0 if Damaris rank */
extern int g_st_damaris_enabled;    /**< @brief 1 if Damaris enabled, 0 otherwise */

/**
 * @brief Returns the damaris_options struct so ROSS can pull in the options
 */
const tw_optdef *st_damaris_opts(void);

/**
 * @brief Print some basic info on Damaris Configuration during simulation init
 */
void st_damaris_init_print(void);

/**
 * @brief Sets up ROSS to use Damaris
 *
 * This must be called after MPI is initialized. Damaris splits the MPI
 * communicator and we set MPI_COMM_ROSS to the subcommunicator returned by Damaris.
 * This sets the variable g_st_ross_rank to 1 on ROSS ranks and 0 on Damaris ranks.
 * Need to make sure that Damaris ranks return to model after this point.
 *
 */
void st_damaris_ross_init(void);

/**
 * @brief Init for using Instrumentation with Damaris.
 */
void st_damaris_inst_init(const char* config_file);

/**
 * @brief Shuts down Damaris and calls MPI_Finalize
 *
 * ROSS ranks (But not damaris ranks) have to first call damaris_stop()
 */
void st_damaris_ross_finalize(void);

/**
 * @brief Expose instrumentation data to Damaris
 */
void st_damaris_expose_data(tw_pe *me, int inst_type, tw_lp* lp, int vts_commit);

/**
 * @brief Signals to Damaris that the current iteration is over.
 *
 * Iterations should always end at GVT (though it doesn't have to be every GVT), because
 * the call to damaris_end_iteration() contains a collective call.
 */
void st_damaris_end_iteration(tw_stime gvt);

/**
 * @brief Make Damaris error checking easier.
 *
 * Some errors will stop simulation, but some will only warn and keep going.
 */
void st_damaris_error(const char *file, int line, int err, const char *variable);

/**
 * @brief parse config file and set parameters for ROSS ranks
 *
 * Allows for instrumentation options to be specified via config file
 */
void st_damaris_parse_config(const char *config_file);
