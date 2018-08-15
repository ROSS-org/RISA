#include <iostream>
#include "damaris/data/VariableManager.hpp"
#include "damaris/data/ParameterManager.hpp"
#include "damaris/env/Environment.hpp"
#include "ross.h"
#include "Event.h"
#include "EventIndex.h"

#include "plugins.h"

using namespace damaris;
using namespace opt_debug;

extern "C" {

void variable_gc(int32_t step, const char *varname)
{
	shared_ptr<Variable> v = VariableManager::Search(varname);

	// Non-time-varying data are not removed 
	if (v->IsTimeVarying()) {
		v->ClearUpToIteration(step);
	}

}

shared_ptr<Event> get_event(EventIndex& events, int src_lp, int dest_lp, float send_ts, float recv_ts)
{
	const Events::iterator& end = events.get<by_full_event>().end();
	Events::iterator begin = events.get<by_full_event>().find(
			boost::make_tuple(src_lp, dest_lp, send_ts, recv_ts));
	if(begin == end) return shared_ptr<Event>();
	return *begin;
}

shared_ptr<Event> get_spec_event(EventIndex& events, int dest_lp, float recv_ts)
{
	const EventsPartial::iterator& end = events.get<by_partial>().end();
	EventsPartial::iterator begin = events.get<by_partial>().find(
			boost::make_tuple(dest_lp, recv_ts));
	if(begin == end) return shared_ptr<Event>();
	return *begin;
}

}
