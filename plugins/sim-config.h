#ifndef SIM_CONFIG_H
#define SIM_CONFIG_H

#include "schemas/data_sample_generated.h"
#include "flatbuffers/minireflect.h"

namespace ross_damaris {

struct SimConfig {
    int num_pe;
    std::vector<int> num_lp;
    int kp_per_pe;
    bool inst_mode_sim[ross_damaris::sample::InstMode_MAX];
    bool inst_mode_model[ross_damaris::sample::InstMode_MAX];
    SimConfig() :
        num_pe(1),
        kp_per_pe(16) {}
};

} // end namespace ross_damaris

#endif // SIM_CONFIG_H
