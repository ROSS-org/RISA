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

static int num_pe = 0;
static int seq_rank = -1;
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

void rng_check(int32_t step);

void rng_check_event(const std::string& event, int32_t src, int32_t step, const char* args)
{
	cout << "plugin: rng_check_event()" << endl;
	int pe_commit_ev[num_pe];
	step--;

	prev_gvt = current_gvt;
	shared_ptr<Variable> p = VariableManager::Search("current_gvt");
	if (p)
		current_gvt = *(float*)p->GetBlock(0, step, 0)->GetDataSpace().GetData();
	else
		cout << "current_gvt not found!" << endl;
	cout << "*************** starting step " << step <<
		" - current GVT: " << current_gvt << " ***************" << endl;
	
	//save_events(step, "ross/fwd_event");
	rng_check(step);

}

void rng_check(int32_t step)
{
	cout << "plugin: rng_check()" << endl;
	shared_ptr<Variable> src_lps = VariableManager::Search("ross/opt_event/src_lp");
	shared_ptr<Variable> dest_lps = VariableManager::Search("ross/opt_event/dest_lp");
	shared_ptr<Variable> send_ts = VariableManager::Search("ross/opt_event/send_ts");
	shared_ptr<Variable> recv_ts = VariableManager::Search("ross/opt_event/recv_ts");
	shared_ptr<Variable> ev_names = VariableManager::Search("ross/opt_event/ev_name");
	shared_ptr<Variable> rng_count = VariableManager::Search("ross/opt_event/rng_count_begin");
	shared_ptr<Variable> prev_rng_count = VariableManager::Search("ross/opt_event/rng_count_end");

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
		long *cur_rng_counts = (long*)rng_count->GetBlock(cur_source, step, cur_id)->GetDataSpace().GetData();
		string cur_name;
	    //if (ev_names->GetBlock(cur_source, step, cur_id))
		//	cur_name = string((char*)ev_names->GetBlock(cur_source, step, cur_id)->GetDataSpace().GetData());

		shared_ptr<Event> seq_event = get_event(events, cur_src_lp, cur_dest_lp, cur_send_ts, cur_recv_ts);
		//cout << "event " << cur_src_lp << ", " << cur_dest_lp << ", " 
		//	<< cur_send_ts << ", "<< cur_recv_ts << ", " <<  cur_rng_counts[0] << endl;
		if (seq_event)
		{
			for (int i = 0; i < num_rng; i++)
			{
				//cout << "seq[i] " << seq_event->get_rng_count(i);
				//cout << " opt[i] " << cur_rng_counts[i] << endl;;
				if (seq_event->get_rng_count(i) != cur_rng_counts[i])
				{
					cout << "RNG ERROR FOUND LP " << cur_src_lp << endl; 
				}
			}
		}
		it++;
	}

}

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
	
	shared_ptr<Variable> p = VariableManager::Search("sync");
	if (p && *(int*)p->GetBlock(src, 0, 0)->GetDataSpace().GetData() == 1)
		seq_rank = src;

	if (!num_lp)
		num_lp = (int*) calloc(num_pe, sizeof(int));

	p = VariableManager::Search("nlp");
	if (p)
		num_lp[src] = *(int*)p->GetBlock(src, 0, 0)->GetDataSpace().GetData();
	else
		cout << "num_lp param not found!" << endl;

	// TODO switch to just getting type info from the seq rank only?
	if (src == seq_rank)
	{
		p = VariableManager::Search("num_types");
		if (p)
			num_types = *(int*)p->GetBlock(src, 0, 0)->GetDataSpace().GetData();
		else
			cout << "num_types variable not found!" << endl;
		
		p = VariableManager::Search("num_rngs");
		if (p)
			num_rng = *(int*)p->GetBlock(src, 0, 0)->GetDataSpace().GetData();
		else
			cout << "num_rngs variable not found!" << endl;
		cout << "number of RNGs: " << num_rng << endl;
		
		char *temp_str;
		p = VariableManager::Search("lp_types_str");
		if (p)
			temp_str = (char*)p->GetBlock(src, 0, 0)->GetDataSpace().GetData();
		else
			cout << "lp_types_str variable not found!" << endl;

		event_handlers = (char**)calloc(num_types, sizeof(char*));
		for (int i = 0; i < num_types; i++)
			event_handlers[i] = (char*)calloc(256, sizeof(char));

		int idx = 0;
		const char s[2] = " ";
		char *token;
		
		/* get the first token */
		token = strtok(temp_str, s);
		
		/* walk through other tokens */
		while(token != NULL)
		{
			strcpy(event_handlers[idx], token);
			event_handlers[idx][strlen(token)] = '\0';
			idx++;
			token = strtok(NULL, s);
		}
		//cout << "event handlers: " << endl;
		//for (int i = 0; i < num_types; i++)
		//	cout << i << " " << event_handlers[i] << endl;
		
		p = VariableManager::Search("lp_types");
		event_map = (int*) calloc(num_lp[seq_rank], sizeof(int));
		if (p)
			memcpy(event_map, (int*)p->GetBlock(src, 0, 0)->GetDataSpace().GetData(), sizeof(int) * num_lp[seq_rank]);
		else
			cout << "lp_types variable not found!" << endl;
		//cout << "event map: " << endl;
		//for (int i = 0; i < num_lp[seq_rank]; i++)
		//	cout << i << " " << event_map[i] << endl;
	}

	//cout << "src " << src << ": num_pe " << num_pe << " num_lp " << num_lp[src] << " seq_rank " << seq_rank << endl;
}
}
