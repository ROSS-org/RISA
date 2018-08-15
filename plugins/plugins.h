#ifndef PLUGINS_H
#define PLUGINS_H

#include <cstdlib>
#include <iostream>

using namespace opt_debug;

extern "C" {

void variable_gc(int32_t step, const char *varname);
shared_ptr<Event> get_event(EventIndex& events, int src_lp, int dest_lp, float send_ts, float recv_ts);
shared_ptr<Event> get_spec_event(EventIndex& events, int dest_lp, float recv_ts);
void save_events(int32_t step, const std::string& var_prefix, int num_rng, float current_gvt, EventIndex& events);

}

#endif // PLUGINS_H
