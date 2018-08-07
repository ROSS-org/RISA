#include <iostream>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/reg.h>
#include "damaris/data/VariableManager.hpp"
#include "damaris/data/ParameterManager.hpp"
#include "damaris/env/Environment.hpp"
#include "ross.h"
#include "Event.h"
#include "EventIndex.h"

using namespace damaris;
using namespace opt_debug;

extern "C" {

static int num_pe = 0;
static int seq_rank = -1;
static int *num_lp = NULL;
static int initialized = 0;
static int errors_found = 0;
static int first_error_lpid = -1;
static float prev_gvt = 0.0, current_gvt = 0.0;
static int *event_map;
static char **event_handlers;
static int num_types;
EventIndex events;

void lp_analysis(int32_t step);
void event_analysis(int32_t step);
void save_events(int32_t step, const std::string& var_prefix);
void opt_debug_gc(int32_t step, const char *varname);
shared_ptr<Event> get_event(int src_lp, int dest_lp, float send_ts, float recv_ts);
void check_opt_events(int32_t step);

// damaris event for optimistic debug analysis
void opt_debug_comparison(const std::string& event, int32_t src, int32_t step, const char* args)
{
	int pe_commit_ev[num_pe];
	step--;

	shared_ptr<Variable> p = VariableManager::Search("ross/pes/gvt_inst/committed_events");
	if (p)
	{
		for (int i = 0; i < num_pe; i++)
			pe_commit_ev[i] = *(int*)p->GetBlock(i, step, 0)->GetDataSpace().GetData();
	}
	else
		cout << "ross/pes/gvt_inst/committed_events not found!" << endl;
	
	prev_gvt = current_gvt;
	p = VariableManager::Search("current_gvt");
	if (p)
		current_gvt = *(float*)p->GetBlock(0, step, 0)->GetDataSpace().GetData();
	else
		cout << "current_gvt not found!" << endl;
	cout << "*************** starting step " << step <<
		" - current GVT: " << current_gvt << " ***************" << endl;
	
	int opt_total = 0;
	for (int i = 0; i < num_pe; i++)
	{
		if (i != seq_rank)
			opt_total += pe_commit_ev[i];
		//printf("rank %d commit ev %d\n", i, pe_commit_ev[i]);
	}
	if (opt_total != pe_commit_ev[seq_rank])
	{
		printf("[GVT %f - %f] WARNING: step %d opt committed events = %d and seq committed events = %d\n",
				prev_gvt, current_gvt, step, opt_total, pe_commit_ev[seq_rank]);
	}
	//lp_analysis(step);
	//event_analysis(step);
	save_events(step, "ross/seq_event");
	check_opt_events(step);
}

void lp_analysis(int32_t step)
{
	int *lp_commit_ev[num_pe];

	shared_ptr<Variable> p = VariableManager::Search("ross/lps/gvt_inst/committed_events");
	if (p)
	{
		for (int i = 0; i < num_pe; i++)
			lp_commit_ev[i] = (int*)p->GetBlock(i, step, 0)->GetDataSpace().GetData();
	}
	else
		cout << "ross/lps/gvt_inst/committed_events not found!" << endl;

	int seq_idx = 0;
	for (int i = 0; i < num_pe; i++)
	{
		if (i == seq_rank)
			continue;
		for (int j = 0; j < num_lp[i]; j++)
		{
			if (lp_commit_ev[i][j] != lp_commit_ev[seq_rank][seq_idx])
			{
				printf("[GVT %f - %f] WARNING: step %d LP %d opt commit ev %d seq commit ev %d\n",
						prev_gvt, current_gvt, step, seq_idx, lp_commit_ev[i][j], lp_commit_ev[seq_rank][seq_idx]);
			}
			seq_idx++;
		}
	}

}

void save_events(int32_t step, const std::string& var_prefix)
{
	shared_ptr<Variable> seq_src_lps = VariableManager::Search(var_prefix + "/src_lp");
	shared_ptr<Variable> seq_dest_lps = VariableManager::Search(var_prefix + "/dest_lp");
	shared_ptr<Variable> seq_send_ts = VariableManager::Search(var_prefix + "/send_ts");
	shared_ptr<Variable> seq_recv_ts = VariableManager::Search(var_prefix + "/recv_ts");
	
	int total_events = seq_src_lps->CountLocalBlocks(step);
	for (int i = 0; i < total_events; i++)
	{
		int cur_pe, cur_src_lp, cur_dest_lp;
		float cur_recv_ts, cur_send_ts;
		
		cur_src_lp = *(int*)seq_src_lps->GetBlock(seq_rank, step, i)->GetDataSpace().GetData();
		cur_dest_lp = *(int*)seq_dest_lps->GetBlock(seq_rank, step, i)->GetDataSpace().GetData();
		cur_send_ts = *(float*)seq_send_ts->GetBlock(seq_rank, step, i)->GetDataSpace().GetData();
		cur_recv_ts = *(float*)seq_recv_ts->GetBlock(seq_rank, step, i)->GetDataSpace().GetData();

		boost::shared_ptr<Event> e(new Event(cur_src_lp, cur_dest_lp, cur_send_ts, cur_recv_ts));
		e->set_gvt(current_gvt);
		e->set_damaris_iteration(step);
		e->set_sync_type(1);
		// put all of our seq events into to the events container
		events.insert(e);
		//cout << "placed event " << cur_src_lp << ", " << cur_dest_lp << ", " 
		//	<< cur_send_ts << ", "<< cur_recv_ts << endl;
	}

}

void check_opt_events(int32_t step)
{
	shared_ptr<Variable> src_lps = VariableManager::Search("ross/opt_event/src_lp");
	shared_ptr<Variable> dest_lps = VariableManager::Search("ross/opt_event/dest_lp");
	shared_ptr<Variable> send_ts = VariableManager::Search("ross/opt_event/send_ts");
	shared_ptr<Variable> recv_ts = VariableManager::Search("ross/opt_event/recv_ts");

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

		shared_ptr<Event> seq_event = get_event(cur_src_lp, cur_dest_lp, cur_send_ts, cur_recv_ts);
		if (!seq_event)
		{
			cout << "OPTIMISTIC ERROR FOUND:" << endl;
			cout << "[send: " << cur_send_ts << "] Optimistic event: src_lp " << cur_src_lp <<
				" dest_lp " << cur_dest_lp << " ts " << cur_recv_ts << endl;
			int event_id = event_map[cur_src_lp];
			cout << "Check RC of ";
			if (event_id != -1)
				cout << event_handlers[event_id] << endl;
			else
				cout << "EVENT SYMBOL WAS NOT FOUND DURING INITIALIZATION" << endl;
			errors_found = 1;
			if (first_error_lpid == -1)
				first_error_lpid = cur_src_lp;
		}
		it++;
	}

}

shared_ptr<Event> get_event(int src_lp, int dest_lp, float send_ts, float recv_ts)
{
	const Events::iterator& end = events.get<by_full_event>().end();
	Events::iterator begin = events.get<by_full_event>().find(
			boost::make_tuple(src_lp, dest_lp, send_ts, recv_ts));
	if(begin == end) return shared_ptr<Event>();
	return *begin;
}

void event_analysis(int32_t step)
{
	shared_ptr<Variable> seq_src_lps = VariableManager::Search("ross/seq_event/src_lp");
	shared_ptr<Variable> seq_dest_lps = VariableManager::Search("ross/seq_event/dest_lp");
	shared_ptr<Variable> seq_send_ts = VariableManager::Search("ross/seq_event/send_ts");
	shared_ptr<Variable> seq_recv_ts = VariableManager::Search("ross/seq_event/recv_ts");

	shared_ptr<Variable> src_lps = VariableManager::Search("ross/opt_event/src_lp");
	shared_ptr<Variable> dest_lps = VariableManager::Search("ross/opt_event/dest_lp");
	shared_ptr<Variable> send_ts = VariableManager::Search("ross/opt_event/send_ts");
	shared_ptr<Variable> recv_ts = VariableManager::Search("ross/opt_event/recv_ts");

	if (!src_lps || !dest_lps || !recv_ts)
	{
		cout << "ERROR in event_analysis()" << endl;
		return;
	}

	int opt_idx[num_pe];
	for (int i = 0; i < num_pe; i++)
		opt_idx[i] = 0;
	
	int total_events = seq_src_lps->CountLocalBlocks(step);
	int tmp1 = seq_dest_lps->CountLocalBlocks(step);
	int tmp2 = seq_recv_ts->CountLocalBlocks(step);
	if (total_events != tmp1 || total_events != tmp2)
		cout << "ERROR found in event data; blocks not same for every seq var" << endl;
	int cur_pe, cur_src_lp, cur_dest_lp;
	int opt_pe, opt_src_lp, opt_dest_lp;
	float cur_recv_ts, opt_recv_ts;
	float cur_send_ts, opt_send_ts;
	for (int i = 0; i < total_events; i++)
	{
		cur_src_lp = *(int*)seq_src_lps->GetBlock(seq_rank, step, i)->GetDataSpace().GetData();
		cur_dest_lp = *(int*)seq_dest_lps->GetBlock(seq_rank, step, i)->GetDataSpace().GetData();
		cur_recv_ts = *(float*)seq_recv_ts->GetBlock(seq_rank, step, i)->GetDataSpace().GetData();
		cur_send_ts = *(float*)seq_send_ts->GetBlock(seq_rank, step, i)->GetDataSpace().GetData();
		cout << "[send: " << cur_send_ts << "] Sequential event: src_lp = " << cur_src_lp << 
			" dest_lp " << cur_dest_lp << " ts " << cur_recv_ts << endl;

		// TODO fix for CODES where LPs may not be evenly divided by num PEs
		// perhaps give the plugin access to the mapping functions used in LP setup?
		cur_pe = cur_dest_lp / num_lp[0];
		//cout << "dest pe " << cur_pe << endl;

		// we need to first check to see if there is an event located in the block to search
		// if not, optimistic error
		if (src_lps->GetBlock(cur_pe, step, opt_idx[cur_pe]))
		{
			opt_src_lp = *(int*)src_lps->GetBlock(cur_pe, step, opt_idx[cur_pe])->GetDataSpace().GetData();
			opt_dest_lp =*(int*)dest_lps->GetBlock(cur_pe, step, opt_idx[cur_pe])->GetDataSpace().GetData();
			opt_recv_ts = *(float*)recv_ts->GetBlock(cur_pe, step, opt_idx[cur_pe])->GetDataSpace().GetData();
			opt_send_ts = *(float*)send_ts->GetBlock(cur_pe, step, opt_idx[cur_pe])->GetDataSpace().GetData();
			//cout << "Optimistic event: src_lp = " << opt_src_lp << " dest_lp " << opt_dest_lp << " ts " << opt_recv_ts << endl;
			// now look in appropriate PE's data for matching event
			if (opt_src_lp != cur_src_lp || opt_dest_lp != cur_dest_lp ||
					opt_recv_ts != cur_recv_ts || opt_send_ts != cur_send_ts)
			{
				cout << "OPTIMISTIC ERROR FOUND:" << endl;
				//cout << "[send: " << cur_send_ts << "] Sequential event: src_lp = " << cur_src_lp << 
				//	" dest_lp " << cur_dest_lp << " ts " << cur_recv_ts << endl;
				cout << "[send: " << opt_send_ts << "] Optimistic event: src_lp = " << opt_src_lp << " dest_lp " << opt_dest_lp << " ts " << opt_recv_ts << endl;
				int event_id = event_map[opt_src_lp];
				cout << "Check RC of ";
				if (event_id != -1)
					cout << event_handlers[event_id] << endl;
				else
					cout << "EVENT SYMBOL WAS NOT FOUND DURING INITIALIZATION" << endl;
				errors_found = 1;
				if (first_error_lpid == -1)
					first_error_lpid = opt_src_lp;

			}
			opt_idx[cur_pe]++;
		}
		else
		{
			cout << "OPTIMISTIC ERROR FOUND:" << endl;
			cout << "[send: " << cur_send_ts << "] Sequential event: src_lp = " << cur_src_lp << 
				" dest_lp " << cur_dest_lp << " ts " << cur_recv_ts << endl;
			errors_found = 1;
		}

	}
	opt_debug_gc(step, "ross/seq_event/src_lp");
	opt_debug_gc(step, "ross/seq_event/dest_lp");
	opt_debug_gc(step, "ross/seq_event/recv_ts");

	opt_debug_gc(step, "ross/opt_event/src_lp");
	opt_debug_gc(step, "ross/opt_event/dest_lp");
	opt_debug_gc(step, "ross/opt_event/recv_ts");
}

void opt_debug_setup(const std::string& event, int32_t src, int32_t step, const char* args)
{
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


void opt_debug_finalize(const std::string& event, int32_t src, int32_t step, const char* args)
{
	int event_id = event_map[first_error_lpid];
	cout << "\n*************** DAMARIS OPTIMISTIC DEBUG FINAL OUTPUT ***************" << endl;
	if (errors_found)
	{
		cout << "\nOptimistic Errors were found!" << endl; 
		cout << "Try looking at LP " << first_error_lpid << " for RC bugs ";
	    cout << "in event function handler: ";
		if (event_id != -1)
			cout << event_handlers[event_id] << endl;
		else
			cout << "EVENT SYMBOL WAS NOT FOUND DURING INITIALIZATION" << endl;
	}
	else
		cout << "\nNo Optimistic Errors detected!" << endl;
	cout << endl;
}

void opt_debug_gc(int32_t step, const char *varname)
{
	shared_ptr<Variable> v = VariableManager::Search(varname);

	// Non-time-varying data are not removed 
	if (v->IsTimeVarying()) {
		v->ClearUpToIteration(step);
	}

}

}
