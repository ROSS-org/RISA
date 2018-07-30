#include <iostream>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/reg.h>
#include "damaris/data/VariableManager.hpp"
#include "ross.h"

using namespace damaris;

extern "C" {

void seq_init_event(const std::string& event, int32_t src, int32_t step, const char* args)
{
	int num_args;
	char *arg_str;

	// grab all the args from Damaris
    VariableManager::iterator it = VariableManager::Begin();
    VariableManager::iterator end = VariableManager::End();

    while (it != end)
    {
		if (it->get()->GetName().compare("opt_debug/num_sim_args") == 0)
		{
			//cout << "variable.GetName: " << it->get()->GetName() << endl;
			num_args = *(int*)it->get()->GetBlock(0, 0, 0)->GetDataSpace().GetData();
			//cout << "nbr items " << num_args << endl;

		}
		else if (it->get()->GetName().compare("opt_debug/sim_args") == 0)
		{
			//cout << "variable.GetName: " << it->get()->GetName() << endl;
			it->get()->GetBlock(0, 0, 0)->NbrOfItems();
			arg_str = (char*)(it->get()->GetBlock(0, 0, 0)->GetDataSpace().GetData());
			//cout << "arg str: ";
			//for (int i = 0; i < it->get()->GetBlock(0, 0, 0)->NbrOfItems(); i++)
			//	cout << arg_str[i];
			//cout << endl;
		}
        it++;
    }


}

// damaris event for optimistic debug analysis
void opt_debug_comparison(const std::string& event, int32_t src, int32_t step, const char* args)
{
	int num_pe = 4;
	int pe_commit_ev[num_pe];
	int seq_rank = -1, tmp;
	step--;

	// grab all the args from Damaris
    VariableManager::iterator it = VariableManager::Begin();
    VariableManager::iterator end = VariableManager::End();

    while (it != end)
    {
		if (it->get()->GetName().compare("ross/pes/gvt_inst/committed_events") == 0)
		{
			//cout << "variable.GetName: " << it->get()->GetName() << endl;
			//cout << "variable.GetName: " << it->get()->GetName() <<  " step " << step;
			//cout << " num local blocks: " << it->get()->CountLocalBlocks(step) << endl;
			for (int i = 0; i < num_pe; i++)
				pe_commit_ev[i] = *(int*)it->get()->GetBlock(i, step, 0)->GetDataSpace().GetData();

		}
		if (it->get()->GetName().compare("ross/pes/gvt_inst/sync") == 0)
		{
			for (int i = 0; i < num_pe; i++)
			{
				tmp = *(int*)it->get()->GetBlock(i, step, 0)->GetDataSpace().GetData();
				if (tmp == 1)
					seq_rank = i;
			}

		}
        it++;
    }

	int opt_total = 0;
	for (int i = 0; i < num_pe; i++)
	{
		if (i != seq_rank)
			opt_total += pe_commit_ev[i];
		//printf("rank %d commit ev %d\n", i, pe_commit_ev[i]);
	}
	if (opt_total != pe_commit_ev[seq_rank])
		printf("WARNING: step %d opt committed events = %d and seq committed events = %d\n",
				step, opt_total, pe_commit_ev[seq_rank]);
}



}
