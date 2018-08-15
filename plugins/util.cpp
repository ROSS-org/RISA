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

shared_ptr<Event> get_event_by_id(EventIndex& events, int peid, long event_id)
{
	const EventsByID::iterator& end = events.get<by_event_id>().end();
	EventsByID::iterator begin = events.get<by_event_id>().find(
			boost::make_tuple(peid, event_id));
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

void save_events(int32_t step, const std::string& var_prefix, int num_rng, float current_gvt, EventIndex& events)
{
	cout << "plugin: save events var_prefix: " << var_prefix << endl;
	shared_ptr<Variable> seq_src_lps = VariableManager::Search(var_prefix + "/src_lp");
	shared_ptr<Variable> seq_dest_lps = VariableManager::Search(var_prefix + "/dest_lp");
	shared_ptr<Variable> seq_send_ts = VariableManager::Search(var_prefix + "/send_ts");
	shared_ptr<Variable> seq_recv_ts = VariableManager::Search(var_prefix + "/recv_ts");
	shared_ptr<Variable> event_ids = VariableManager::Search(var_prefix + "/event_id");
	shared_ptr<Variable> ev_names = VariableManager::Search(var_prefix + "/ev_name");
	shared_ptr<Variable> rng_count = VariableManager::Search(var_prefix + "/rng_count");
	
	BlocksByIteration::iterator it;
	BlocksByIteration::iterator end;
	seq_src_lps->GetBlocksByIteration(step, it, end);

	while (it != end)
	{
		int cur_pe, cur_src_lp, cur_dest_lp;
		float cur_recv_ts, cur_send_ts;
		string cur_name;
		
		int cur_id = (*it)->GetID();
		int cur_source = (*it)->GetSource();
		cur_src_lp = *(int*)(*it)->GetDataSpace().GetData();
		cur_dest_lp = *(int*)seq_dest_lps->GetBlock(cur_source, step, cur_id)->GetDataSpace().GetData();
		cur_send_ts = *(float*)seq_send_ts->GetBlock(cur_source, step, cur_id)->GetDataSpace().GetData();
		cur_recv_ts = *(float*)seq_recv_ts->GetBlock(cur_source, step, cur_id)->GetDataSpace().GetData();
        long ev_id = *(long*)event_ids->GetBlock(cur_source, step, cur_id)->GetDataSpace().GetData();
		long *cur_rng_counts = (long*)rng_count->GetBlock(cur_source, step, cur_id)->GetDataSpace().GetData();
		if (ev_names->GetBlock(cur_source, step, cur_id))
			cur_name = string((char*)ev_names->GetBlock(cur_source, step, cur_id)->GetDataSpace().GetData());
		else
			cout << "didn't get " << var_prefix << "/ev_name" << endl;

		boost::shared_ptr<Event> e(new Event(cur_src_lp, cur_dest_lp, cur_send_ts, cur_recv_ts, num_rng));
		e->set_gvt(current_gvt);
		e->set_damaris_iteration(step);
		e->set_event_name(cur_name);
        e->set_dest_pe(cur_source);
        e->set_event_id(ev_id);
		//cout << "rng counts: ";
		for (int i = 0; i < num_rng; i++)
		{
			e->set_rng_count(i, cur_rng_counts[i]);
			//cout << cur_rng_counts[i] << " ";
		}

		// put all of our seq events into to the events container
		if (var_prefix.compare("ross/fwd_event") == 0)
		{
			e->set_sync_type(3);
			//cout << "placed event " << cur_src_lp << ", " << cur_dest_lp << ", " 
			//	<< cur_send_ts << ", "<< cur_recv_ts << endl;
		}
		else
		{
			//cout << "placed event " << cur_src_lp << ", " << cur_dest_lp << ", " 
			//	<< cur_send_ts << ", "<< cur_recv_ts << ", " << cur_rng_counts[0] << endl;
			e->set_sync_type(1);
		}
        events.insert(e);
		it++;
	}
}

}
