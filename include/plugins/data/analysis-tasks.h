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

struct initial_task_args
{
    int step;
    void* dm;
    //damaris::DataSpace<damaris::Buffer> ds;
    const void* ds;
};

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
void insert_data_mic(void *arguments);
void initial_data_processing(void *arguments);

#ifdef __cplusplus
} // end extern "C"
#endif

#endif // end ANALYSIS_TASKS_H
