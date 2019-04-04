#ifndef RISA_INTERFACE_H
#define RISA_INTERFACE_H

#include <ross.h>
#include <Damaris.h>

/**
 * @file risa-interface.h
 * @brief RISA integration with ROSS
 */

typedef struct model_lp_metadata model_lp_metadata;
typedef struct mlp_var_metadata mlp_var_metadata;

/**
 * @brief Used for getting user metadata from LPs to Damaris Ranks
 */
struct model_lp_metadata
{
    int peid;
    int kpid;
    int lpid;
    int num_vars;
    size_t name_sz;
};

/**
 * @brief Used for getting user metadata from LPs to Damaris Ranks
 */
struct mlp_var_metadata
{
    size_t name_sz;
    int id;
};

/**
 * @brief Sets up the simulation parameters needed by Damaris
 * @param[in] config_file path to configuration file
 */
void set_parameters(const char *config_file);

/**
 * @brief Let Damaris ranks know to do init
 */
void signal_init(void);

#endif // RISA_INTERFACE_H
