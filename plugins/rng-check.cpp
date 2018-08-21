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
using namespace std;

extern "C" {

static int num_pe = 0;
static int *num_lp = NULL;
static int num_rng = 0;
static int initialized = 0;
static int errors_found = 0;
static int first_error_iteration = -1;
static int first_error_lpid = -1;
static float first_error_recv = DBL_MAX;
static string first_error_name;
static float prev_gvt = 0.0, current_gvt = 0.0;
static int *event_map;
static char **event_handlers;
static int num_types;
static EventIndex events;
static EventIndex spec_events;
static set<string> error_set;

void rng_check(int32_t step);

void rng_check_event(const std::string& event, int32_t src, int32_t step, const char* args)
{
	cout << "plugin: rng_check_event()" << endl;
	int pe_commit_ev[num_pe];
	step--;

	shared_ptr<Variable> p = VariableManager::Search("current_gvt");
	if (p)
		current_gvt = *(float*)p->GetBlock(0, step, 0)->GetDataSpace().GetData();
	else
		cout << "current_gvt not found!" << endl;
	cout << "*************** starting step " << step <<
		" - current GVT: " << current_gvt << " ***************" << endl;

	save_events(step, "ross/fwd_event", num_rng, 0.0, events);
	rng_check(step);

	variable_gc(step, "ross/fwd_event/src_lp");
	variable_gc(step, "ross/fwd_event/dest_lp");
	variable_gc(step, "ross/fwd_event/send_ts");
	variable_gc(step, "ross/fwd_event/recv_ts");
	variable_gc(step, "ross/fwd_event/event_id");
	variable_gc(step, "ross/fwd_event/src_pe");
	variable_gc(step, "ross/fwd_event/ev_name");
	variable_gc(step, "ross/fwd_event/rng_count_before");
	variable_gc(step, "ross/fwd_event/rng_count_end");

	variable_gc(step, "ross/rev_event/src_lp");
	variable_gc(step, "ross/rev_event/dest_lp");
	variable_gc(step, "ross/rev_event/send_ts");
	variable_gc(step, "ross/rev_event/recv_ts");
	variable_gc(step, "ross/rev_event/event_id");
	variable_gc(step, "ross/rev_event/src_pe");
	variable_gc(step, "ross/rev_event/ev_name");
	variable_gc(step, "ross/rev_event/rng_count_before");
	variable_gc(step, "ross/rev_event/rng_count_end");


    remove_events_below_gvt(events, current_gvt);
}

void rng_check(int32_t step)
{
	cout << "plugin: rng_check()" << endl;
	shared_ptr<Variable> src_lps = VariableManager::Search("ross/rev_event/src_lp");
	shared_ptr<Variable> dest_lps = VariableManager::Search("ross/rev_event/dest_lp");
	shared_ptr<Variable> send_ts = VariableManager::Search("ross/rev_event/send_ts");
	shared_ptr<Variable> recv_ts = VariableManager::Search("ross/rev_event/recv_ts");
	shared_ptr<Variable> event_ids = VariableManager::Search("ross/rev_event/event_id");
	shared_ptr<Variable> send_pes = VariableManager::Search("ross/rev_event/src_pe");
	shared_ptr<Variable> ev_names = VariableManager::Search("ross/rev_event/ev_name");
	shared_ptr<Variable> rng_count_before = VariableManager::Search("ross/rev_event/rng_count_before");
	shared_ptr<Variable> rng_count_end = VariableManager::Search("ross/rev_event/rng_count_end");

	BlocksByIteration::iterator it;
	BlocksByIteration::iterator end;
	src_lps->GetBlocksByIteration(step, it, end);

	while (it != end)
	{
		int cur_id = (*it)->GetID();
		int cur_source = (*it)->GetSource();
		int cur_src_lp = *(int*)(*it)->GetDataSpace().GetData();
		int cur_dest_lp = *(int*)dest_lps->GetBlock(cur_source, step, cur_id)->GetDataSpace().GetData();
		float cur_send_ts = *(float*)send_ts->GetBlock(cur_source, step, cur_id)->GetDataSpace().GetData();
		float cur_recv_ts = *(float*)recv_ts->GetBlock(cur_source, step, cur_id)->GetDataSpace().GetData();
		int cur_send_pe = *(int*)send_pes->GetBlock(cur_source, step, cur_id)->GetDataSpace().GetData();
		long ev_id = *(long*)event_ids->GetBlock(cur_source, step, cur_id)->GetDataSpace().GetData();
		long *cur_rng_count_before = (long*)rng_count_before->GetBlock(cur_source, step, cur_id)->GetDataSpace().GetData();
		long *cur_rng_count_end = (long*)rng_count_end->GetBlock(cur_source, step, cur_id)->GetDataSpace().GetData();
		string cur_name;
	    if (ev_names->GetBlock(cur_source, step, cur_id))
			cur_name = string((char*)ev_names->GetBlock(cur_source, step, cur_id)->GetDataSpace().GetData());

        std::pair<EventsByID::iterator, EventsByID::iterator> ev_iterators = get_events_by_id(events, cur_send_pe, ev_id);
        EventsByID::iterator evit = ev_iterators.first;
        bool found = false;
        //cout << "rev_event: (" << cur_send_pe << ", " << ev_id << ") (" << cur_src_lp << ", " << cur_dest_lp << ", " 
        //    << cur_send_ts << ", "<< cur_recv_ts << ") " <<  cur_rng_counts[0] << endl;
        if (evit == ev_iterators.second)
        {
            // TODO we must be losing events when deleting from MultiIndex
            cout << "No forward events found" << endl;
            found = true;
        }
        while (evit != ev_iterators.second)
        {
            //(*evit)->print_event_data("fwd_event");
            if (cur_src_lp == (*evit)->get_src_lp() && cur_dest_lp == (*evit)->get_dest_lp() &&
                    cur_send_ts == (*evit)->get_send_ts() && cur_recv_ts == (*evit)->get_recv_ts())
            {
                // now check RNG counters
                bool all_match = true;
                for (int i = 0; i < num_rng; i++)
                {
                    if (cur_rng_count_end[i] != (*evit)->get_rng_count_before(i) ||
                            cur_rng_count_before[i] != (*evit)->get_rng_count_end(i))
                        all_match = false;
                }
                if (all_match)
                {
                    found = true;
                    break;
                }
            }
            //else
            //{
            //    cout << "possible error: fwd and rev events don't match!" << endl;
            //    cout << "rev_event cur_send_pe " << cur_send_pe << " event id " << ev_id << ": " << cur_src_lp << ", " << cur_dest_lp << ", " 
            //        << cur_send_ts << ", "<< cur_recv_ts << ", " <<  cur_rng_counts[0] << endl;
            //    (*evit)->print_event_data("fwd_event");
            //}
            evit++;
        }

        if (!found)
        {
            error_set.insert(cur_name);
            cout << "RNG ERROR FOUND LP " << cur_dest_lp << " event: " << cur_name << endl; 
            cout << "rev_event cur_send_pe " << cur_send_pe << " event id " << ev_id << ": " << cur_src_lp << ", " << cur_dest_lp << ", " 
                << cur_send_ts << ", "<< cur_recv_ts << "; ";
            for (int i = 0; i < num_rng; i++)
            {
                cout << "rng_count[" << i << "] (" << cur_rng_count_before[i] << ", " << cur_rng_count_end[i] << ") ";
            }
            cout << endl;
            evit = ev_iterators.first;
            while (evit != ev_iterators.second)
            {
                if (cur_src_lp == (*evit)->get_src_lp() && cur_dest_lp == (*evit)->get_dest_lp() &&
                        cur_send_ts == (*evit)->get_send_ts() && cur_recv_ts == (*evit)->get_recv_ts())
                {
                    for (int i = 0; i < num_rng; i++)
                    {
                        if (cur_rng_count_end[i] != (*evit)->get_rng_count_before(i) ||
                                cur_rng_count_before[i] != (*evit)->get_rng_count_end(i))
                            (*evit)->print_event_data("fwd_event");
                    }
                }

                evit++;
            }
        }

		//if (fwd_event)
		//{
		//	for (int i = 0; i < num_rng; i++)
		//	{
		//		//cout << "seq[i] " << fwd_event->get_rng_count(i);
		//		//cout << " opt[i] " << cur_rng_counts[i] << endl;;
		//		if (fwd_event->get_rng_count(i) != cur_rng_counts[i])
		//		{
		//			cout << "RNG ERROR FOUND LP " << cur_src_lp << endl; 
        //            cout << "rev_event cur_send_pe " << cur_send_pe << " event id " << ev_id << ": " << cur_src_lp << ", " << cur_dest_lp << ", " 
        //                << cur_send_ts << ", "<< cur_recv_ts << ", " <<  cur_rng_counts[0] << endl;
        //            fwd_event->print_event_data("fwd_event");
		//		}
		//	}
		//}
        //else
        //    cout << "error: didn't find a fwd event" << endl;
		it++;
	}

}

// use group scope for this event, so we only do it once
void rng_check_setup(const std::string& event, int32_t src, int32_t step, const char* args)
{
	cout << "plugin: rng_check_setup()" << endl;
	/*
	 * Note: couldn't get correct values from Damaris paramters
	 * even though the clients can successfully call 
	 * damaris_parameter_get() and the the correct parameter value,
	 * I couldn't get any method of pulling the parameters in this plugin
	 * to work correctly
	 */

	num_pe = Environment::CountTotalClients();
	
	shared_ptr<Variable> p;

	if (!num_lp)
		num_lp = (int*) calloc(num_pe, sizeof(int));

	p = VariableManager::Search("nlp");
	if (p)
    {
        for (int i = 0; i < num_pe; i++)
            num_lp[i] = *(int*)p->GetBlock(i, 0, 0)->GetDataSpace().GetData();
    }
	else
		cout << "num_lp param not found!" << endl;

    p = VariableManager::Search("num_rngs");
    if (p)
        num_rng = *(int*)p->GetBlock(0, 0, 0)->GetDataSpace().GetData();
    else
        cout << "num_rngs variable not found!" << endl;

    cout << "end rng_check_setup: num_pe " << num_pe << " num_rng " << num_rng << endl;
}

void rng_check_finalize(const std::string& event, int32_t src, int32_t step, const char* args)
{
	cout << "\n*************** DAMARIS OPTIMISTIC RNG DEBUG FINAL OUTPUT ***************" << endl;
    if (error_set.empty())
    {
        cout << "\nNo RNG Errors were detected!" << endl;
    }
    else
    {
        cout << "\nRNG Errors were detected! Inspect the following event handlers:" << endl;
        for (set<string>::iterator it = error_set.begin(); it != error_set.end(); ++it)
            cout << *it << endl;

    }
	//if (errors_found)
	//{
	//	cout << "\nOptimistic Errors were found!" << endl; 
	//	cout << "Try looking at LP " << first_error_lpid << " for RC bugs ";
	//    cout << "in event function handler: ";
	//	if (event_id != -1)
	//		cout << event_handlers[event_id] << " specifically: " << first_error_name << endl;
	//	else
	//		cout << "\nEVENT SYMBOL WAS NOT FOUND DURING INITIALIZATION" << endl;
	//}
	//else
	//	cout << "\nNo Optimistic Errors detected!" << endl;
	//cout << endl;
}
}
