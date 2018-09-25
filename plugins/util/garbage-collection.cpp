#include <iostream>
#include "damaris/data/VariableManager.hpp"

using namespace damaris;

extern "C" {

// keep data for this number of steps
static int num_steps = 5;

void damaris_gc(const std::string& event, int32_t src, int32_t step, const char* args)
{
    (void) event;
    (void) src;
    (void) args;

    if (step < num_steps) return;

    VariableManager::iterator it = VariableManager::Begin();
    VariableManager::iterator end = VariableManager::End();

    //cout << "damaris_gc step " << step << endl;

    while (it != end)
    {
        // don't delete non-time varying data like PE IDs, etc
        if (it->get()->IsTimeVarying())
        {
            (*it)->ClearUpToIteration(step-num_steps);
        }
        it++;
    }
}

}
