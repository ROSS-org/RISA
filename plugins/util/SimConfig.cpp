#include <boost/program_options.hpp>
#include <damaris/data/VariableManager.hpp>
#include <damaris/data/ParameterManager.hpp>
#include <plugins/util/config-c.h>
#include <plugins/util/SimConfig.h>
#include <ross.h>
#include <instrumentation/st-instrumentation-internal.h>

using namespace std;
//namespace po = boost::program_options;
namespace rds = ross_damaris::sample;
using namespace ross_damaris::config;
using namespace ross_damaris::util;
using namespace damaris;

/**
 * @brief C wrapper for ROSS ranks to call SimConfig::parse_file
 *
 * Declared in plugins/util/config-c.h.
 */
void parse_file(const char *filename)
{
    ifstream ifs(filename);
    auto opts = SimConfig::set_options();
    po::variables_map vars;
    SimConfig::parse_file(ifs, opts, vars);
    ifs.close();
    SimConfig::set_ross_parameters(vars);
}

po::options_description SimConfig::set_options()
{
    po::options_description opts;
    opts.add_options()
        ("inst.engine-stats", po::value<int>()->default_value(0), "collect sim engine metrics")
        ("inst.model-stats", po::value<int>()->default_value(0), "collect model level metrics")
        ("inst.num-gvt", po::value<int>()->default_value(10), "num GVT between sampling points")
        ("inst.rt-interval", po::value<double>()->default_value(1000.0), "real time sampling interval")
        ("inst.vt-interval", po::value<double>()->default_value(1000000.0), "virtual time sampling interval")
        ("inst.vt-samp-end", po::value<double>()->default_value(0.0), "end time for virtual time sampling")
        ("inst.pe-data", po::value<bool>()->default_value(true), "turn on/off PE level sim data")
        ("inst.kp-data", po::value<bool>()->default_value(false), "turn on/off KP level sim data")
        ("inst.lp-data", po::value<bool>()->default_value(false), "turn on/off LP level sim data")
        ("inst.event-trace", po::value<int>()->default_value(0), "turn on event tracing")
        ("damaris.enable-damaris", po::value<bool>()->default_value(0), "turn on Damaris in situ component")
        ("damaris.data-xml", po::value<std::string>(), "path to damaris XML file to be used")
        ("damaris.write-data", po::value<bool>()->default_value(false), "turn on writing flatbuffer data to file")
        ("damaris.stream-data", po::value<bool>()->default_value(true), "turn on streaming flatbuffer data")
        ("damaris.stream.address", po::value<string>()->default_value("localhost"), "IP address to stream data to")
        ("damaris.stream.port", po::value<string>()->default_value("8000"), "port for streaming data")
        ;

    return opts;
}

void SimConfig::parse_file(ifstream& file, po::options_description& opts, po::variables_map& var_map)
{
    po::parsed_options parsed = parse_config_file(file, opts, false);
    store(parsed, var_map);
    notify(var_map); // not sure if needed?
}

// TODO need to support all instrumentation parameters
void SimConfig::set_ross_parameters(po::variables_map& var_map)
{
    // set ROSS globals based on variables in config file
    if (var_map["inst.pe-data"].as<bool>() == true)
        g_st_pe_data = 1;
    else
        g_st_pe_data = 0;

    if (var_map["inst.kp-data"].as<bool>() == true)
        g_st_kp_data = 1;
    else
        g_st_kp_data = 0;

    if (var_map["inst.lp-data"].as<bool>() == true)
        g_st_lp_data = 1;
    else
        g_st_lp_data = 0;

    g_st_engine_stats = var_map["inst.engine-stats"].as<int>();
    g_st_model_stats = var_map["inst.model-stats"].as<int>();
    g_st_num_gvt = var_map["inst.num-gvt"].as<int>();
    g_st_rt_interval = var_map["inst.rt-interval"].as<double>();
    g_st_vt_interval = var_map["inst.vt-interval"].as<double>();
    g_st_sampling_end = var_map["inst.vt-samp-end"].as<double>();
    g_st_ev_trace = var_map["inst.event-trace"].as<int>();
}

void SimConfig::set_parameters(po::variables_map& var_map)
{
    pe_data_ = var_map["inst.pe-data"].as<bool>();
    kp_data_ = var_map["inst.kp-data"].as<bool>();
    lp_data_ = var_map["inst.lp-data"].as<bool>();
    num_gvt_ = var_map["inst.num-gvt"].as<int>();
    rt_interval_ = var_map["inst.rt-interval"].as<double>();
    vt_interval_ = var_map["inst.vt-interval"].as<double>();
    vt_samp_end_ = var_map["inst.vt-samp-end"].as<double>();
    write_data_ = var_map["damaris.write-data"].as<bool>();
    stream_data_ = var_map["damaris.stream-data"].as<bool>();
    stream_addr_ = var_map["damaris.stream.address"].as<string>();
    stream_port_ = var_map["damaris.stream.port"].as<string>();

    // TODO probably want to change how instrumentation modes are turned on
    if (var_map["inst.engine-stats"].as<int>() == 1)
    {
        inst_mode_sim_[rds::InstMode_GVT] = true;
        inst_mode_sim_[rds::InstMode_RT] = false;
        inst_mode_sim_[rds::InstMode_VT] = false;
    }
    else if (var_map["inst.engine-stats"].as<int>() == 2)
    {
        inst_mode_sim_[rds::InstMode_GVT] = false;
        inst_mode_sim_[rds::InstMode_RT] = true;
        inst_mode_sim_[rds::InstMode_VT] = false;
    }
    else if (var_map["inst.engine-stats"].as<int>() == 3)
    {
        inst_mode_sim_[rds::InstMode_GVT] = false;
        inst_mode_sim_[rds::InstMode_RT] = false;
        inst_mode_sim_[rds::InstMode_VT] = true;
    }
    else if (var_map["inst.engine-stats"].as<int>() == 4)
    {
        inst_mode_sim_[rds::InstMode_GVT] = true;
        inst_mode_sim_[rds::InstMode_RT] = true;
        inst_mode_sim_[rds::InstMode_VT] = true;
    }

    if (var_map["inst.model-stats"].as<int>() == 1)
    {
        inst_mode_model_[rds::InstMode_GVT] = true;
        inst_mode_model_[rds::InstMode_RT] = false;
        inst_mode_model_[rds::InstMode_VT] = false;
    }
    else if (var_map["inst.model-stats"].as<int>() == 2)
    {
        inst_mode_model_[rds::InstMode_GVT] = false;
        inst_mode_model_[rds::InstMode_RT] = true;
        inst_mode_model_[rds::InstMode_VT] = false;
    }
    else if (var_map["inst.model-stats"].as<int>() == 3)
    {
        inst_mode_model_[rds::InstMode_GVT] = false;
        inst_mode_model_[rds::InstMode_RT] = false;
        inst_mode_model_[rds::InstMode_VT] = true;
    }
    else if (var_map["inst.model-stats"].as<int>() == 4)
    {
        inst_mode_model_[rds::InstMode_GVT] = true;
        inst_mode_model_[rds::InstMode_RT] = true;
        inst_mode_model_[rds::InstMode_VT] = true;
    }

    // TODO need to set event tracing
}

void SimConfig::print_parameters()
{
    cout << "\nNumber of PEs: " << num_local_pe_;
    cout << "\nNumber of KPs per PE: " << num_kp_pe_;
    cout << "\nLPs per PE: ";
    int total_lp = 0;
    for (int i = 0; i < num_local_pe_; i++)
    {
        total_lp += num_lp_[i];
        cout << num_lp_[i] << " ";
    }
    cout << "\nTotal LPs: " << total_lp;
    cout << "\nInstrumentation Modes:";
    if (inst_mode_sim_[rds::InstMode_GVT] || inst_mode_model_[rds::InstMode_GVT])
    {
        cout << "\n\tGVT-based: ";
        print_mode_types(rds::InstMode_GVT);
        cout << "\n\tNumber of GVTs between samples: " << num_gvt_;
    }
    if (inst_mode_sim_[rds::InstMode_RT] || inst_mode_model_[rds::InstMode_RT])
    {
        cout << "\n\tReal Time Sampling: ";
        print_mode_types(rds::InstMode_RT);
        cout << "\n\tSampling interval " << rt_interval_;
    }
    if (inst_mode_sim_[rds::InstMode_VT] || inst_mode_model_[rds::InstMode_VT])
    {
        cout << "\n\tVirtual Time Sampling: ";
        print_mode_types(rds::InstMode_VT);
        cout << "\n\tSampling interval: " << vt_interval_;
        cout << "\n\tSampling End time: " << vt_samp_end_;
    }

    cout << "\n\n";
}

void SimConfig::print_mode_types(int type)
{
    if (inst_mode_sim_[type])
    {
        cout << "simulation engine data (";
        if (pe_data_)
            cout << "PE, ";
        if (kp_data_)
            cout << "KP, ";
        if (lp_data_)
            cout << "LP";
        cout << ") ";
    }
    if (inst_mode_model_[type])
        cout << "model data";
}

void SimConfig::set_model_metadata()
{
    // TODO add a check to see if simulation is even collecting model data
    std::shared_ptr<Variable> v = VariableManager::Search("model_map/lp_md");
    BlocksByIteration::iterator begin, end;
    v->GetBlocksByIteration(0, begin, end);
    while (begin != end)
    {
        // each iterator points to a single LP's model metadata
        DataSpace<Buffer> ds((*begin)->GetDataSpace());
        char* dbuf_cur = reinterpret_cast<char*>(ds.GetData());
        size_t cur_size = 0;

        int *peid = reinterpret_cast<int*>(dbuf_cur);
        dbuf_cur += sizeof(*peid);
        cur_size += sizeof(*peid);
        int *kpid = reinterpret_cast<int*>(dbuf_cur);
        dbuf_cur += sizeof(*kpid);
        cur_size += sizeof(*kpid);
        int *lpid = reinterpret_cast<int*>(dbuf_cur);
        dbuf_cur += sizeof(*lpid);
        cur_size += sizeof(*lpid);
        int *num_vars = reinterpret_cast<int*>(dbuf_cur);
        dbuf_cur += sizeof(*num_vars);
        cur_size += sizeof(*num_vars);
        size_t *name_sz = reinterpret_cast<size_t*>(dbuf_cur);
        dbuf_cur += sizeof(*name_sz);
        cur_size += sizeof(*name_sz);
        char* lp_name = reinterpret_cast<char*>(dbuf_cur);
        dbuf_cur += *name_sz;
        cur_size += *name_sz;
        shared_ptr<ModelLPMetadata> lp_md = std::make_shared<ModelLPMetadata>(
                *peid, *kpid, *lpid, string(lp_name));

        for (int i = 0; i < *num_vars; i++)
        {
            size_t *var_name_sz = reinterpret_cast<size_t*>(dbuf_cur);
            dbuf_cur += sizeof(*var_name_sz);
            cur_size += sizeof(*var_name_sz);
            int *id = reinterpret_cast<int*>(dbuf_cur);
            dbuf_cur += sizeof(*id);
            cur_size += sizeof(*id);
            dbuf_cur += 4;
            cur_size += 4;
            char* var_name = reinterpret_cast<char*>(dbuf_cur);
            dbuf_cur += *var_name_sz;
            cur_size += *var_name_sz;
            lp_md->add_variable(string(var_name), *id);
        }
        md_index_.insert(std::move(lp_md));

        ++begin;
    }

}

ModelMDByFullID::iterator SimConfig::get_lp_model_md(int peid, int kpid, int lpid)
{
    return md_index_.get<by_full_id>().find(std::make_tuple(peid, kpid, lpid));
}
