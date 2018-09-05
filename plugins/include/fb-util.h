#ifndef FB_UTIL_H
#define FB_UTIL_H

#include "sim-config.h"
#include "flatbuffers/minireflect.h"
#include <iostream>

namespace ross_damaris {
namespace util {

namespace rds = ross_damaris::sample;

class FBUtil
{

    public:
    template <typename T>
    static void add_metric(rds::SimEngineMetricsT *obj, const std::string& var_name, T value);

    static void collect_metrics(rds::SimEngineMetricsT *metrics_objects, const std::string& var_prefix,
        int32_t src, int32_t step, int32_t block_id, int32_t num_entities);
};

} // end namespace flatbuffers
} // end namespace ross_damaris
#endif // FB_UTIL_H
