#ifndef DAMARIS_UTIL_H
#define DAMARIS_UTIL_H

#include <damaris/data/VariableManager.hpp>
#include <damaris/data/Variable.hpp>

namespace ross_damaris {
namespace util {

/**
 * @brief A collection of helpful utility functions for accessing Damaris data.
 */
class DUtil
{
    public:
    static void *get_value_from_damaris(const std::string& varname, int32_t src, int32_t step, int32_t block_id);
    static void get_damaris_iterators(const std::string& varname, int32_t step,
            damaris::BlocksByIteration::iterator& begin, damaris::BlocksByIteration::iterator& end);
    static int get_num_blocks(const std::string& varname, int iteration);
};

} // end namespace util
} //end namespace ross_damaris

#endif // DAMARIS_UTIL_H
