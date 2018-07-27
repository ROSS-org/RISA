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
//void optimistic_debug(const std::string& event, int32_t src, int32_t step, const char* args)
//{
//    // have GVT value passed in through args
//    // run sequential up to that point in virtual time
//}
//


}
