#include <plugins/data/LPAnalyzer.h>
#include <plugins/data/Features.h>
#include <vtkInformation.h>
#include <vtkPartitionedDataSet.h>
#include <vtkTable.h>
#include <vtkUnsignedIntArray.h>
#include <vtkFloatArray.h>
#include <vtkVariantArray.h>

using namespace ross_damaris::data;
using namespace std;

vtkObjectFactoryNewMacro(LPAnalyzer);

// TODO should prob improve
// do we really want to do all this every time we create a new LPAnalyzer?
LPAnalyzer::LPAnalyzer() :
    LastStepProcessed(0),
    total_flagged(0),
    initial_round(true)
{
    this->SetNumberOfInputPorts(1);
    this->SetNumberOfOutputPorts(2);
    this->sim_config = config::SimConfig::get_instance();
    this->stream_client = streaming::StreamClient::get_instance();

    // init lp_avgs
    for (std::map<int, std::pair<int,int>>::iterator it = sim_config->lp_map.begin();
            it != sim_config->lp_map.end(); ++it)
    {
        int lpid = it->first;
        int peid = it->second.first;
        int kp_lid = it->second.second;
        int kp_gid = peid * sim_config->num_kp_pe() + kp_lid;

        // find if we already created a MovingAvgData for this KP
        auto kp_it = lp_avg_map.find(kp_gid);
        if (kp_it == lp_avg_map.end())
        {
            MovingAvgData mva_data(peid, kp_gid, kp_lid);
            auto rv = lp_avg_map.insert(std::make_pair(kp_gid, std::move(mva_data)));
            kp_it = rv.first;
        }

        // either way, kp_it should now be a valid iterator
        // using LPMetrics here, because it's averaging the LP metrics for a KP
        kp_it->second.set_kp_metric(EnumNameMetrics(LPMetrics::EVENT_PROC));
        kp_it->second.set_kp_metric(EnumNameMetrics(LPMetrics::EVENT_RB));
        kp_it->second.set_lp_metric(lpid, EnumNameMetrics(LPMetrics::EVENT_PROC));
        kp_it->second.set_lp_metric(lpid, EnumNameMetrics(LPMetrics::EVENT_RB));
    }
}

int LPAnalyzer::FillInputPortInformation(int port, vtkInformation* info)
{
    if (port == LP_INPUT)
    {
        info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
        info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSet");
        return 1;
    }
    return 0;
}

int LPAnalyzer::FillOutputPortInformation(int port, vtkInformation* info)
{
    if (port == LP_OUTPUT)
    {
        info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPartitionedDataSet");
        return 1;
    }
    else if (port == AVG_OUTPUT)
    {
        info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPartitionedDataSet");
        return 1;
    }
    return 0;
}

int LPAnalyzer::RequestData(vtkInformation* request, vtkInformationVector** input_vec,
        vtkInformationVector* output_vec)
{
    vtkPartitionedDataSet* lp_data = vtkPartitionedDataSet::GetData(input_vec[LP_INPUT], 0);
    rows_to_write.clear();
    CalculateMovingAverages(lp_data);
    //FindProblematicLPs();

    //vtkPartitionedDataSet* selected_lps = vtkPartitionedDataSet::GetData(output_vec, LP_OUTPUT);
    //selected_lps->SetNumberOfPartitions(total_flagged);
    //GetProblematicLPs(lp_data, selected_lps);
    return 1;
}

void LPAnalyzer::CalculateMovingAverages(vtkPartitionedDataSet* in_data)
{
    if (!in_data)
        return;

    // num entities
    unsigned int num_partitions = in_data->GetNumberOfPartitions();
    int total_kp = sim_config->num_kp_pe() * sim_config->num_local_pe();

    vtkTable *table = vtkTable::SafeDownCast(in_data->GetPartitionAsDataObject(0));
    vtkIdType total_steps = table->GetNumberOfRows();
    // When we input lp data from Aggregator, all rows should be added to average,
    // because we get a new (small) table each time
    for (int row = 0; row < total_steps; row++)
    //for (int row = this->LastStepProcessed + 1; row < total_steps; row++)
    {
        vector<double> kp_avgs_evproc(total_kp, 0);
        vector<double> kp_avgs_evrb(total_kp, 0);
        vector<int> kp_num_lp(total_kp, 0);

        //bool init_val = (row == 0 ? true : false);
        // TODO lpid only correct when running sim on a single node
        for (int lpid = 0; lpid < num_partitions; lpid++)
        {
            // get LP info
            int peid = sim_config->lp_map[lpid].first;
            int kp_lid = sim_config->lp_map[lpid].second;
            int kp_gid = peid * sim_config->num_kp_pe() + kp_lid;
            kp_num_lp[kp_gid]++;

            MovingAvgData& kp_madata = lp_avg_map.at(kp_gid);

            vtkTable *table = vtkTable::SafeDownCast(in_data->GetPartitionAsDataObject(lpid));
            float value = table->GetValueByName(row, EnumNameMetrics(LPMetrics::EVENT_PROC)).ToFloat();
            kp_avgs_evproc[kp_gid] += value;
            auto results = kp_madata.update_lp_moving_avg(lpid,
                    EnumNameMetrics(LPMetrics::EVENT_PROC), value, initial_round);
            auto std_dev = sqrt(results.second);
            //cout << "CalcMA: lp: " << lpid << endl;
            //cout << "\tev proc val: " << value << ", EMA: " << results.first << ", std_dev: " << std_dev << endl;
            if (value > results.first + 4 * std_dev)
            {
                kp_madata.flag_lp(lpid);
                rows_to_write.insert(table->GetValueByName(row, "timestamp").ToDouble());
            }

            value = table->GetValueByName(row, EnumNameMetrics(LPMetrics::EVENT_RB)).ToFloat();
            kp_avgs_evrb[kp_gid] += value;
            results = kp_madata.update_lp_moving_avg(lpid, EnumNameMetrics(LPMetrics::EVENT_RB), value, initial_round);
            std_dev = sqrt(results.second);
            if (value > results.first + 4 * std_dev)
            {
                kp_madata.flag_lp(lpid);
                rows_to_write.insert(table->GetValueByName(row, "timestamp").ToDouble());
            }
        }

        for (int kpid = 0; kpid < total_kp; kpid++)
        {
            MovingAvgData& kp_madata = lp_avg_map.at(kpid);
            kp_madata.update_kp_moving_avg(EnumNameMetrics(LPMetrics::EVENT_PROC),
                    kp_avgs_evproc[kpid] / kp_num_lp[kpid], initial_round);
            kp_madata.update_kp_moving_avg(EnumNameMetrics(LPMetrics::EVENT_RB),
                    kp_avgs_evrb[kpid] / kp_num_lp[kpid], initial_round);
        }
    }
    initial_round = false;
    this->LastStepProcessed += total_steps;
    //this->LastStepProcessed = total_steps - 1;
}

vtkIdType LPAnalyzer::FindTSRow(vtkTable* entity_data, double ts)
{
    // start at end, of table, since that should be faster in practice
    for (vtkIdType row = entity_data->GetNumberOfRows() - 1; row >= 0; row--)
    {
        if (ts == entity_data->GetValueByName(row, "timestamp").ToDouble())
            return row;
    }
    return -1;
}

// TODO ideally when we flag an LP, we'd choose to stream some number of prev samples as well
// TODO also examine cases where LPs are flagged and tweak as necessary
size_t LPAnalyzer::FindProblematicLPs(vtkPartitionedDataSet* lp_pds, TSIndex& samples)
{
    size_t reduced_data_size = 0;
    // Some LPs were already flagged based on their own moving averages
    // now see if we need to flag more based on comparing to the LP avg on this KP
    int total_kp = sim_config->num_kp_pe() * sim_config->num_local_pe();
    bool data_to_write = false;
    total_flagged = 0;
    set<double> timestamps;
    for (int kpid = 0; kpid < total_kp; kpid++)
    {
        MovingAvgData& kp_madata = lp_avg_map.at(kpid);
        kp_madata.compare_lp_to_kp(EnumNameMetrics(LPMetrics::EVENT_PROC));
        kp_madata.compare_lp_to_kp(EnumNameMetrics(LPMetrics::EVENT_RB));
        total_flagged += kp_madata.get_num_flagged();

        // convert any LPs on this KP to flatbuffers
        for (auto lp_it = kp_madata.flagged_lps.begin(); lp_it != kp_madata.flagged_lps.end(); ++lp_it)
        {
            if (!lp_it->second)
                continue;

            // for each time stamp here, we want some number of previous time steps saved
            // to have higher resolution data
            for (double ts : rows_to_write)
            {
                // by_gvt is a ranked index now
                auto max_rank = samples.get<by_gvt>().find_rank(ts);
                size_t start_rank = 0;
                size_t past_ranks = 10;
                if (max_rank - past_ranks >= 0)
                    start_rank = max_rank - past_ranks;

                for (auto i = start_rank; i <= max_rank; i++)
                {
                    auto samp = samples.get<by_gvt>().nth(i);
                    auto cur_ts = (*samp)->get_gvt();
                    SampleFBBuilder* samp_fbb = (*samp)->get_sample_fbb();
                    vtkTable *table = vtkTable::SafeDownCast(lp_pds->GetPartitionAsDataObject(lp_it->first));
                    auto row = FindTSRow(table, cur_ts);
                    samp_fbb->add_lp(table, row, kp_madata.get_peid(), kp_madata.get_kp_lid(), lp_it->first);
                    timestamps.insert(cur_ts);
                    data_to_write = true;
                }
            }
            kp_madata.unflag_lp(lp_it->first);
        }
    }

    if (data_to_write)
    {
        for (double ts : timestamps)
        {
            auto samp = samples.get<by_gvt>().find(ts);
            SampleFBBuilder* samp_fbb = (*samp)->get_sample_fbb();
            samp_fbb->finish();
            size_t size, offset;
            uint8_t* raw = samp_fbb->release_raw(size, offset);
            stream_client->enqueue_data(raw, &raw[offset], size);
            reduced_data_size += size;
        }
    }

    printf("LastStepProcessed: %d, total_flagged_lps: %d\n", LastStepProcessed, total_flagged);
    return reduced_data_size;
}

//void LPAnalyzer::GetProblematicLPs(vtkPartitionedDataSet* in_data, vtkPartitionedDataSet* out_data)
//{
//    if (!in_data)
//        return;
//    if (!out_data)
//        return;
//
//    
//}

