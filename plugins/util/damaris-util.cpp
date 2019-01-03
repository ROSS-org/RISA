#include <plugins/util/damaris-util.h>

using namespace ross_damaris::util;
using namespace damaris;

// return the pointer to the variable, because it may be a single value or an array
void *DUtil::get_value_from_damaris(const std::string& varname, int32_t src, int32_t step, int32_t block_id)
{
    std::shared_ptr<Variable> v = VariableManager::Search(varname);
    if (v && v->GetBlock(src, step, block_id))
        return v->GetBlock(src, step, block_id)->GetDataSpace().GetData();
    else
    {
        //cout << "ERROR in get_value_from_damaris() for variable " << varname << endl;
        return nullptr;
    }
}

void DUtil::get_damaris_iterators(const std::string& varname, int32_t step, BlocksByIteration::iterator& begin, BlocksByIteration::iterator& end)
{
    std::shared_ptr<Variable> v = VariableManager::Search(varname);
    if (v)
        v->GetBlocksByIteration(step, begin, end);
}

int DUtil::get_num_blocks(const std::string& varname, int iteration)
{
    std::shared_ptr<Variable> v = VariableManager::Search(varname);
    if (v)
        return v->CountLocalBlocks(iteration);
    else
        return 0;
}

