/**
 * @file events.cpp
 *
 * This file defines all the possible events that Damaris will call for plugins.
 */
#include <iostream>
#include <plugins/server/RDServer.h>
#include <plugins/flatbuffers/data_sample_generated.h>
#include <plugins/util/damaris-util.h>
#include <plugins/util/SimConfig.h>
#include <plugins/data/analysis-tasks.h>

#include <damaris/buffer/DataSpace.hpp>
#include <damaris/buffer/Buffer.hpp>
#include <damaris/data/Variable.hpp>

using namespace ross_damaris::server;
using namespace ross_damaris::sample;
using namespace ross_damaris::util;
using namespace std;

extern "C" {

// TODO maybe should be unique_ptr
//boost::shared_ptr<RDServer> server;
RDServer *server;

/**
 * @brief Damaris event to set up simulation configuration
 *
 * Should be set in XML file as "group" scope
 */
void damaris_rank_init(const std::string& event, int32_t src, int32_t step, const char* args)
{
    (void) event;
    (void) src;
    (void) step;
    (void) args;
    //cout << "damaris_rank_init() step " << step << endl;

    //server.reset(new RDServer());
    server = RDServer::get_instance();
    auto sim_config = ross_damaris::config::SimConfig::get_instance();
    sim_config->set_model_metadata();
    //cout << "end of damaris_rank_init()\n";
}

/**
 * @brief Damaris event called at the end of each iteration
 *
 * Should be set in XML file as "group" scope.
 * Removes data from Damaris control and stores it in our own boost multi-index.
 */
void damaris_rank_end_iteration(const std::string& event, int32_t src, int32_t step, const char* args)
{
    (void)event;
    (void)src;
    (void)args;

    // want the last step because the data is now complete
    step--;
    //cout << "damaris_rank_end_iteration() rank " << src << " step " << step << endl;

    server->initial_data_tasks(step);
    server->update_gvt(step);
}

/**
 * @brief Damaris event to close files, sockets, etc
 *
 * Called just before Damaris is stopped.
 * Called with "group" scope.
 */
void damaris_rank_finalize(const std::string& event, int32_t src, int32_t step, const char* args)
{
    (void) event;
    (void) src;
    (void) step;
    (void) args;
    //cout << "damaris_rank_finalize() step " << step << endl;
    server->finalize();
    //cout << "end of damaris_rank_finalize()\n";
    size_t raw, reduced;
    get_reduction_sizes(&raw, &reduced);
    double red_percent = 100.0 * (raw - reduced) / raw;
    cout << "RISA End\nRaw data size: " << raw << "\nReduced data size: " << reduced <<
        "\nPercent reduced: " << red_percent << "%" << endl;
}

}
