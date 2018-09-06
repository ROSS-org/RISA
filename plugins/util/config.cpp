#include <boost/program_options.hpp>
#include "config.h"
#include "sim-config.h"
#include "fb-util.h"

using namespace std;
//namespace po = boost::program_options;
namespace rds = ross_damaris::sample;
using namespace ross_damaris;

po::options_description ross_damaris::set_options()
{
    po::options_description opts;
    opts.add_options()
        ("inst.engine-stats", po::value<int>()->default_value(0), "collect sim engine metrics")
        ("inst.model-stats", po::value<int>()->default_value(0), "collect model level metrics")
        ("inst.num-gvt", po::value<int>()->default_value(10), "num GVT between sampling points")
        ("inst.rt-interval", po::value<double>()->default_value(0.0), "real time sampling interval")
        ("inst.vt-interval", po::value<double>()->default_value(0.0), "virtual time sampling interval")
        ("inst.vt-samp-end", po::value<double>()->default_value(0.0), "end time for virtual time sampling")
        ("inst.pe-data", po::value<bool>()->default_value(true), "turn on/off PE level sim data")
        ("inst.kp-data", po::value<bool>()->default_value(false), "turn on/off KP level sim data")
        ("inst.lp-data", po::value<bool>()->default_value(false), "turn on/off LP level sim data")
        ("inst.event-trace", po::value<int>()->default_value(0), "turn on event tracing")
        ("damaris.enable-damaris", po::value<bool>()->default_value(0), "turn on Damaris in situ component")
        ("damaris.data-xml", po::value<std::string>(), "path to damaris XML file to be used")
        ("damaris.write-data", po::value<bool>()->default_value(false), "turn on writing flatbuffer data to file")
        ("damaris.stream-data", po::value<bool>()->default_value(true), "turn on streaming flatbuffer data")
        ;

    return opts;
}

void ross_damaris::parse_file(ifstream& file, po::options_description& opts, po::variables_map& var_map)
{
    po::parsed_options parsed = parse_config_file(file, opts, false);
    store(parsed, var_map);
    notify(var_map); // not sure if needed?
}

void ross_damaris::SimConfig::set_parameters(po::variables_map& var_map)
{
    pe_data = var_map["inst.pe-data"].as<bool>();
    kp_data = var_map["inst.kp-data"].as<bool>();
    lp_data = var_map["inst.lp-data"].as<bool>();
    num_gvt = var_map["inst.num-gvt"].as<int>();
    rt_interval = var_map["inst.rt-interval"].as<double>();
    vt_interval = var_map["inst.vt-interval"].as<double>();
    vt_samp_end = var_map["inst.vt-samp-end"].as<double>();
    write_data = var_map["damaris.write-data"].as<bool>();
    stream_data = var_map["damaris.stream-data"].as<bool>();

    // TODO probably want to change how instrumentation modes are turned on
    if (var_map["inst.engine-stats"].as<int>() == 1)
    {
        inst_mode_sim[rds::InstMode_GVT] = true;
        inst_mode_sim[rds::InstMode_RT] = false;
        inst_mode_sim[rds::InstMode_VT] = false;
    }
    else if (var_map["inst.engine-stats"].as<int>() == 2)
    {
        inst_mode_sim[rds::InstMode_GVT] = false;
        inst_mode_sim[rds::InstMode_RT] = true;
        inst_mode_sim[rds::InstMode_VT] = false;
    }
    else if (var_map["inst.engine-stats"].as<int>() == 3)
    {
        inst_mode_sim[rds::InstMode_GVT] = false;
        inst_mode_sim[rds::InstMode_RT] = false;
        inst_mode_sim[rds::InstMode_VT] = true;
    }
    else if (var_map["inst.engine-stats"].as<int>() == 4)
    {
        inst_mode_sim[rds::InstMode_GVT] = true;
        inst_mode_sim[rds::InstMode_RT] = true;
        inst_mode_sim[rds::InstMode_VT] = true;
    }

    if (var_map["inst.model-stats"].as<int>() == 1)
    {
        inst_mode_model[rds::InstMode_GVT] = true;
        inst_mode_model[rds::InstMode_RT] = false;
        inst_mode_model[rds::InstMode_VT] = false;
    }
    else if (var_map["inst.model-stats"].as<int>() == 2)
    {
        inst_mode_model[rds::InstMode_GVT] = false;
        inst_mode_model[rds::InstMode_RT] = true;
        inst_mode_model[rds::InstMode_VT] = false;
    }
    else if (var_map["inst.model-stats"].as<int>() == 3)
    {
        inst_mode_model[rds::InstMode_GVT] = false;
        inst_mode_model[rds::InstMode_RT] = false;
        inst_mode_model[rds::InstMode_VT] = true;
    }
    else if (var_map["inst.model-stats"].as<int>() == 4)
    {
        inst_mode_model[rds::InstMode_GVT] = true;
        inst_mode_model[rds::InstMode_RT] = true;
        inst_mode_model[rds::InstMode_VT] = true;
    }

    // TODO need to set event tracing
}

void ross_damaris::SimConfig::print_parameters()
{
    cout << "\nNumber of PEs: " << num_pe;
    cout << "\nNumber of KPs per PE: " << kp_per_pe;
    cout << "\nLPs per PE: ";
    int total_lp = 0;
    for (int i = 0; i < num_pe; i++)
    {
        total_lp += num_lp[i];
        cout << num_lp[i] << " ";
    }
    cout << "\nTotal LPs: " << total_lp;
    cout << "\nInstrumentation Modes:";
    if (inst_mode_sim[rds::InstMode_GVT] || inst_mode_model[rds::InstMode_GVT])
    {
        cout << "\n\tGVT-based: ";
        print_mode_types(rds::InstMode_GVT);
        cout << "\n\tNumber of GVTs between samples: " << num_gvt;
    }
    if (inst_mode_sim[rds::InstMode_RT] || inst_mode_model[rds::InstMode_RT])
    {
        cout << "\n\tReal Time Sampling: ";
        print_mode_types(rds::InstMode_RT);
        cout << "\n\tSampling interval " << rt_interval;
    }
    if (inst_mode_sim[rds::InstMode_VT] || inst_mode_model[rds::InstMode_VT])
    {
        cout << "\n\tVirtual Time Sampling: ";
        print_mode_types(rds::InstMode_VT);
        cout << "\n\tSampling interval: " << vt_interval;
        cout << "\n\tSampling End time: " << vt_samp_end;
    }
    
    cout << "\n\n";
}

void ross_damaris::SimConfig::print_mode_types(int type)
{
    if (inst_mode_sim[type])
    {
        cout << "simulation engine data (";
        if (pe_data)
            cout << "PE, ";
        if (kp_data)
            cout << "KP, ";
        if (lp_data)
            cout << "LP";
        cout << ") ";
    }
    if (inst_mode_model[type])
        cout << "model data";
}
