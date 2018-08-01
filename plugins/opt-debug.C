#include <iostream>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/reg.h>
#include "damaris/data/VariableManager.hpp"
#include "damaris/data/ParameterManager.hpp"
#include "damaris/env/Environment.hpp"
#include "ross.h"

using namespace damaris;

extern "C" {

static int num_pe = 0;
static int seq_rank = -1;
static int *num_lp = NULL;
static int initialized = 0;
static float prev_gvt = 0.0, current_gvt = 0.0;

void lp_analysis(int step);
void get_parameters(int32_t src);
void event_analysis(int32_t step);

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
	lp_analysis(step);
	event_analysis(step);
}

void lp_analysis(int step)
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

void event_analysis(int32_t step)
{
	shared_ptr<Variable> seq_src_lps = VariableManager::Search("ross/seq_event/src_lp");
	shared_ptr<Variable> seq_dest_lps = VariableManager::Search("ross/seq_event/dest_lp");
	shared_ptr<Variable> seq_recv_ts = VariableManager::Search("ross/seq_event/recv_ts");

	shared_ptr<Variable> src_lps = VariableManager::Search("ross/opt_event/src_lp");
	shared_ptr<Variable> dest_lps = VariableManager::Search("ross/opt_event/dest_lp");
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
	float cur_ts, opt_ts;
	for (int i = 0; i < total_events; i++)
	{
		cur_src_lp = *(int*)seq_src_lps->GetBlock(seq_rank, step, i)->GetDataSpace().GetData();
		cur_dest_lp = *(int*)seq_dest_lps->GetBlock(seq_rank, step, i)->GetDataSpace().GetData();
		cur_ts = *(float*)seq_recv_ts->GetBlock(seq_rank, step, i)->GetDataSpace().GetData();
		//cout << "Sequential event: src_lp = " << cur_src_lp << " dest_lp " << cur_dest_lp << " ts " << cur_ts << endl;

		// TODO fix for CODES where LPs may not be evenly divided by num PEs
		// perhaps give the plugin access to the mapping functions used in LP setup?
		cur_pe = cur_dest_lp / num_lp[0];
		//cout << "dest pe " << cur_pe << endl;

		// we need to first check to see if there is an event located in the block to search
		// if not, optimistic error

		opt_src_lp = *(int*)src_lps->GetBlock(cur_pe, step, opt_idx[cur_pe])->GetDataSpace().GetData();
		opt_dest_lp =*(int*)dest_lps->GetBlock(cur_pe, step, opt_idx[cur_pe])->GetDataSpace().GetData();
		opt_ts = *(float*)recv_ts->GetBlock(cur_pe, step, opt_idx[cur_pe])->GetDataSpace().GetData();
		//cout << "Optimistic event: src_lp = " << opt_src_lp << " dest_lp " << opt_dest_lp << " ts " << opt_ts << endl;

		// now look in appropriate PE's data for matching event
		if (opt_src_lp != cur_src_lp ||
				opt_dest_lp != cur_dest_lp ||
				opt_ts != cur_ts)
		{
			cout << "OPTIMISTIC ERROR FOUND:" << endl;
			cout << "Sequential event: src_lp = " << cur_src_lp << " dest_lp " << cur_dest_lp << " ts " << cur_ts << endl;
			cout << "Optimistic event: src_lp = " << opt_src_lp << " dest_lp " << opt_dest_lp << " ts " << opt_ts << endl;

		}
		opt_idx[cur_pe]++;
	}

}

void opt_debug_setup(const std::string& event, int32_t src, int32_t step, const char* args)
{
	get_parameters(src);
}

void get_parameters(int32_t src)
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

	//cout << "src " << src << ": num_pe " << num_pe << " num_lp " << num_lp[src] << " seq_rank " << seq_rank << endl;
}


}
