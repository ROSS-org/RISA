#ifndef SIM_CONFIG_H
#define SIM_CONFIG_H

#include <iostream>
#include <fstream>
#include <unordered_set>
#include <plugins/flatbuffers/data_sample_generated.h>
//#include <plugins/server/RDServer.h>
#include <flatbuffers/minireflect.h>
#include <boost/program_options.hpp>
#include <plugins/util/ModelMDIndex.h>

namespace po = boost::program_options;
namespace ross_damaris {

// need this forward declaration to make friend
namespace server {
  class RDServer;
}

namespace config {

/**
 * @brief keeps track of simulation settings
 *
 * RDServer is a friend of this class because it is the only one who
 * should be able to make changes to the configuration.
 * Getters are provided so that other classes can read simulation
 * settings if necessary.
 * Uses boost program_options for getting config info from file.
 */
class SimConfig {
public:
    static SimConfig* get_instance();

    void print_parameters();
    void print_mode_types(int type);

    /* getters */
    int total_pe() { return total_pe_; }
    int num_local_pe() { return num_local_pe_; }
    int num_kp_pe() { return num_kp_pe_; }
    const std::vector<int>& num_lp() { return num_lp_; }
    int num_lp(int peid) { return num_lp_[peid]; }
    int num_local_lp() { return num_local_lp_; }
    bool pe_data() { return pe_data_; }
    bool kp_data() { return kp_data_; }
    bool lp_data() { return lp_data_; }
    int num_gvt() { return num_gvt_; }
    double rt_interval() { return rt_interval_; }
    double vt_interval() { return vt_interval_; }
    double vt_samp_end() { return vt_samp_end_; }
    bool write_data() { return write_data_; }
    bool stream_data() { return stream_data_; }
    std::string stream_addr() { return stream_addr_; }
    std::string stream_port() { return stream_port_; }
    bool inst_mode_sim(ross_damaris::sample::InstMode mode) { return inst_mode_sim_[mode]; }
    bool inst_mode_model(ross_damaris::sample::InstMode mode) { return inst_mode_model_[mode]; }

    static po::options_description set_options();
    static void parse_file(std::ifstream& file, po::options_description& opts,
            po::variables_map& var_map);
    static void set_ross_parameters(po::variables_map& var_map);

    void set_model_metadata();
    ross_damaris::util::ModelMDByFullID::iterator end();
    ross_damaris::util::ModelMDByFullID::iterator get_lp_model_md(int peid, int kpid, int lpid);

private:
    SimConfig();
    SimConfig(const SimConfig&) = delete;
    SimConfig& operator=(const SimConfig&) = delete;

    void set_parameters(po::variables_map& var_map);

    int total_pe_;
    int num_local_pe_;
    int num_kp_pe_;
    std::vector<int> num_lp_; // TODO is this for all PEs, or just my PEs?
    int num_local_lp_;

    bool inst_mode_sim_[ross_damaris::sample::InstMode_MAX];
    bool inst_mode_model_[ross_damaris::sample::InstMode_MAX];
    bool pe_data_;
    bool kp_data_;
    bool lp_data_;
    int num_gvt_;
    double rt_interval_;
    double vt_interval_;
    double vt_samp_end_;

    bool write_data_;
    bool stream_data_;
    std::string stream_addr_;
    std::string stream_port_;

    // mapping info for model LP metadata
    ross_damaris::util::ModelMDIndex md_index_;
};

} // end namespace config
} // end namespace ross_damaris

#endif // SIM_CONFIG_H
