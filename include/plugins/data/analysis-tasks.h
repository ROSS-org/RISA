/**
 * @file analysis-tasks.h
 *
 * Definitions of functions to be used by Argobots work units.
 */

#ifndef ANALYSIS_TASKS_H
#define ANALYSIS_TASKS_H

#include <damaris/buffer/DataSpace.hpp>
#include <damaris/buffer/Buffer.hpp>
#include <plugins/data/ArgobotsManager.h>
#include <abt.h>

#ifdef __cplusplus
extern "C" {
#endif

struct init_args
{
    void* data_manager;
    void* sim_config;
    void* stream_client;
};

struct initial_task_args
{
    int step;
    const void* ds;
};

struct data_agg_args
{
    int mode;
    double lower_ts;
    double upper_ts;
};

void initialize_task(void *arguments);

/**
 * @brief Aggregates data for a given sampling point.
 */
void aggregate_data(void *arg);

/**
 * @brief Removes data from the DataManager up to a given sampling point.
 */
void remove_data_mic(void *arg);

/**
 * @brief Adds Damaris DataSpaces to the DataManager.
 */
void initial_data_processing(void *arguments);

#ifdef __cplusplus
} // end extern "C"
#endif

#endif // end ANALYSIS_TASKS_H
