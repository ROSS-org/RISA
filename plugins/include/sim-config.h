#ifndef SIM_CONFIG_H
#define SIM_CONFIG_H

#include <iostream>
#include <fstream>
#include "schemas/data_sample_generated.h"
#include "flatbuffers/minireflect.h"
#include <boost/program_options.hpp>

namespace po = boost::program_options;
namespace ross_damaris {

struct SimConfig {
    int num_pe;
    std::vector<int> num_lp;
    int kp_per_pe;

    bool inst_mode_sim[ross_damaris::sample::InstMode_MAX];
    bool inst_mode_model[ross_damaris::sample::InstMode_MAX];
    bool pe_data;
    bool kp_data;
    bool lp_data;
    int num_gvt;
    double rt_interval;
    double vt_interval;
    double vt_samp_end;
    SimConfig() :
        num_pe(1),
        kp_per_pe(16),
        pe_data(true),
        kp_data(false),
        lp_data(false),
        num_gvt(10),
        rt_interval(0.0),
        vt_interval(0.0),
        vt_samp_end(0.0) {  }

    void set_parameters(po::variables_map& var_map);
    void print_parameters();
    void print_mode_types(int type);
};

po::options_description set_options();
void parse_file(std::ifstream& file, po::options_description& opts, po::variables_map& var_map);
} // end namespace ross_damaris

#endif // SIM_CONFIG_H
