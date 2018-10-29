/**
 * @file analysis-tasks.h
 *
 * Definitions of functions to be used by Argobots work units.
 */

#ifndef ANALYSIS_TASKS_H
#define ANALYSIS_TASKS_H

#include <plugins/data/ArgobotsManager.h>
#include <abt.h>

#ifdef __cplusplus
extern "C" {
#endif

//extern ABT_pool g_pool;

/**
 * @brief Aggregates data for a given sampling point.
 */
void aggregate_data(void *arg);

/**
 * @brief Removes data from the DataManager up to a given sampling point.
 */
void remove_data_mic(void *arg);

/**
 * @brief Adds data for a given sampling point to the DataManager.
 */
void insert_data_mic(void *arg);

#ifdef __cplusplus
} // end extern "C"
#endif

#endif // end ANALYSIS_TASKS_H
