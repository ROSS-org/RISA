/**
 * @file events.cpp
 *
 * This file defines all the possible events that Damaris will call for plugins.
 */
#include <plugins/server/RDServer.h>
#include <plugins/flatbuffers/data_sample_generated.h>
#include <plugins/util/damaris-util.h>

#include <damaris/buffer/DataSpace.hpp>
#include <damaris/buffer/Buffer.hpp>
#include <damaris/data/Variable.hpp>

using namespace ross_damaris::server;
using namespace ross_damaris::sample;
using namespace ross_damaris::util;

extern "C" {

// TODO maybe should be unique_ptr
boost::shared_ptr<RDServer> server;

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
    server.reset(new RDServer());
    server->set_model_metadata();
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
    // want the last step because the data is now complete
    step--;
    //cout << "damaris_rank_end_iteration() rank " << src << " step " << step << endl;

    server->update_gvt(step);

    server->initial_data_tasks(step);

    // TODO this is just for now
    // eventually the data processor will be its own thread
    // and it will make the determination itself when and what data
    // to forward on for streaming/writing
    server->forward_data(step);
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
}

}
