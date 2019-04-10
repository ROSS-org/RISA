#ifndef RISA_API_H
#define RISA_API_H

#include <ross.h>
#include <Damaris.h>

/**
 * @file risa-api.h
 * @brief API for RISA integration with ROSS
 */

extern int g_st_ross_rank;       /**< @brief 1 if ROSS PE, 0 if Damaris rank */
extern int g_st_risa_enabled;    /**< @brief 1 if Damaris enabled, 0 otherwise */

/**
 * @brief Returns the damaris_options struct so ROSS can pull in the options
 */
const tw_optdef *risa_opts(void);

/**
 * @brief Sets up ROSS to use Damaris
 *
 * This must be called after MPI is initialized. Damaris splits the MPI
 * communicator and we set MPI_COMM_ROSS to the subcommunicator returned by Damaris.
 * This sets the variable g_st_ross_rank to 1 on ROSS ranks and 0 on Damaris ranks.
 * Need to make sure that RISA/Damaris ranks return to model after this point.
 *
 */
void risa_init(void);

/**
 * @brief parse config file and set parameters for ROSS ranks
 * @param[in] config_file path to configuration file
 *
 * Allows for instrumentation options to be specified via config file
 */
void risa_parse_config(const char *config_file);

/**
 * @brief Init for using Instrumentation with Damaris.
 * @param[in] config_file path to configuration file
 */
void risa_inst_init(const char* config_file);

/**
 * @brief Print some basic info on the RISA/Damaris Configuration during simulation init
 */
void risa_init_print(void);

/**
 * @brief Shuts down Damaris and calls MPI_Finalize
 *
 * ROSS ranks (But not damaris ranks) have to first call damaris_stop()
 */
void risa_finalize(void);

/**
 * @brief Expose instrumentation data to Damaris
 * @param[in] me PE pointer
 * @param[in] inst_type Instrumentation mode
 * @param[in] lp Analysis LP pointer (only for VTS)
 * @param[in] vts_commit true if called from a commit function, otherwise false
 *
 * Can use the sampling functions provided by ROSS instrumentation layer, instead of
 * having to create new ones.
 */
void risa_expose_data_gvt(tw_pe *me, int inst_type);
void risa_expose_data_rts(tw_pe *me, int inst_type);
void risa_expose_data_vts(tw_pe *me, int inst_type, tw_lp* lp, int vts_commit);

/**
 * @brief Signals to Damaris that the current iteration is over.
 * @param[in] gvt Current GVT value
 *
 * Iterations should always end at GVT (though it doesn't have to be every GVT), because
 * the call to damaris_end_iteration() contains a collective call.
 */
void risa_end_iteration(tw_stime gvt);

/**
 * @brief Send some data to RISA/Damaris ranks so it knows a data sample has been invalidated
 * by rollback (only for model data).
 * @param[in] vts virtual time stamp
 * @param[in] rts real time stamp which is used to identify multiple samples with same vts
 * @param[in] kp_gid global id of KPs over full simulation
 */
void risa_invalidate_sample(double vts, double rts, int kp_gid);

/**
 * @brief Send some data to RISA/Damaris ranks so it knows a data sample has been validated
 * due to commit (only for model data).
 * @param[in] vts virtual time stamp
 * @param[in] rts real time stamp which is used to identify multiple samples with same vts
 * @param[in] kp_gid global id of KPs over full simulation
 */
void risa_validate_sample(double vts, double rts, int kp_gid);

/**
 * @brief Make Damaris error checking easier.
 * @param[in] file file where error occurred
 * @param[in] line line where error occurred
 * @param[in] err error number from Damaris
 * @param[in] variable variable name
 *
 * Some errors will stop simulation, but some will only warn and keep going.
 * TW_LOC can be used for file and line as in tw_error
 */
void risa_damaris_error(const char *file, int line, int err, const char *variable);

#endif // RISA_API_H
