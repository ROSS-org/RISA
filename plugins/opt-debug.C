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

void lp_analysis(int step);
void get_parameters(int32_t src);

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
	
	int opt_total = 0;
	for (int i = 0; i < num_pe; i++)
	{
		if (i != seq_rank)
			opt_total += pe_commit_ev[i];
		//printf("rank %d commit ev %d\n", i, pe_commit_ev[i]);
	}
	if (opt_total != pe_commit_ev[seq_rank])
	{
		printf("WARNING: step %d opt committed events = %d and seq committed events = %d\n",
				step, opt_total, pe_commit_ev[seq_rank]);
	}
	lp_analysis(step);
}

void lp_analysis(int step)
{
	int num_lp = 32;
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
	for (int i = 0; i < num_pe - 1; i++)
	{
		for (int j = 0; j < num_lp; j++)
		{
			if (lp_commit_ev[i][j] != lp_commit_ev[seq_rank][seq_idx])
			{
				printf("WARNINGstep %d LP %d opt commit ev %d seq commit ev %d\n",
						step, seq_idx, lp_commit_ev[i][j], lp_commit_ev[seq_rank][seq_idx]);
			}
			seq_idx++;
		}
	}

}

void opt_debug_setup(const std::string& event, int32_t src, int32_t step, const char* args)
{
	cout << "opt_debug_init() step " << step << " src " << src << endl;
	//if (!initialized)
	//{
		get_parameters(src);
	//	initialized = 1;
	//}

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

	cout << "src " << src << ": num_pe " << num_pe << " num_lp " << num_lp[src] << " seq_rank " << seq_rank << endl;
}


}
