#ifndef DAMARIS_UTIL_H
#define DAMARIS_UTIL_H

#include "damaris/data/VariableManager.hpp"

namespace ross_damaris {
namespace util {

class DUtil
{
    public:
    static void *get_value_from_damaris(const std::string& varname, int32_t src, int32_t step, int32_t block_id);
    static int get_num_blocks(const std::string& varname, int iteration);
};

} // end namespace util
} //end namespace ross_damaris

#endif // DAMARIS_UTIL_H
