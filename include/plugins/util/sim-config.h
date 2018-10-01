#ifndef SIM_CONFIG_H
#define SIM_CONFIG_H

#include <iostream>
#include <fstream>
#include <unordered_set>
#include <plugins/flatbuffers/data_sample_generated.h>
#include <flatbuffers/minireflect.h>
#include <boost/program_options.hpp>

namespace po = boost::program_options;
namespace ross_damaris {

struct SimConfig {
    int total_pe;
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

    bool write_data;
    bool stream_data;
    std::string stream_addr;
    std::string stream_port;
    SimConfig() :
        num_pe(1),
        kp_per_pe(16),
        pe_data(true),
        kp_data(false),
        lp_data(false),
        num_gvt(10),
        rt_interval(0.0),
        vt_interval(0.0),
        vt_samp_end(0.0),
        write_data(false),
        stream_data(true) {  }

    void set_parameters(po::variables_map& var_map);
    void print_parameters();
    void print_mode_types(int type);
};

class ModelConfig {
    private:
    std::map<std::string, std::unordered_set<std::string>> lp_type_var_map_;

    public:
    ModelConfig() {  }
    void model_setup();
    void print_model_info();

};

po::options_description set_options();
void parse_file(std::ifstream& file, po::options_description& opts, po::variables_map& var_map);
void set_ross_parameters(po::variables_map& var_map);

} // end namespace ross_damaris

#endif // SIM_CONFIG_H
