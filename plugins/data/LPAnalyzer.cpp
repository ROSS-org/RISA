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

LPAnalyzer::LPAnalyzer() :
    LastStepProcessed(-1)
{
    this->SetNumberOfInputPorts(1);
    this->SetNumberOfOutputPorts(2);
    this->sim_config = config::SimConfig::get_instance();

    // init lp_avgs
    for (std::map<int, std::pair<int,int>>::iterator it = sim_config->lp_map.begin();
            it != sim_config->lp_map.end(); ++it)
    {
        auto lpid = it->first;
        auto peid = it->second.first;
        auto kp_lid = it->second.second;
        auto kp_gid = peid * sim_config->num_kp_pe() + kp_lid;

        // find if we already created a MovingAvgData for this KP
        auto kp_it = lp_avg_map.find(kp_gid);
        if (kp_it == lp_avg_map.end())
        {
            MovingAvgData mva_data(peid, kp_gid, kp_lid);
            auto rv = lp_avg_map.insert(std::make_pair(lpid, std::move(mva_data)));
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
    CalculateMovingAverages(lp_data);
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
    for (int row = this->LastStepProcessed + 1; row < total_steps; row++)
    {
        vector<double> kp_avgs_evproc(total_kp, 0);
        vector<double> kp_avgs_evrb(total_kp, 0);
        vector<int> kp_num_lp(total_kp, 0);

        bool init_val = (row == 0 ? true : false);
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
            kp_madata.update_lp_moving_avg(lpid, EnumNameMetrics(LPMetrics::EVENT_PROC), value, init_val);

            value = table->GetValueByName(row, EnumNameMetrics(LPMetrics::EVENT_RB)).ToFloat();
            kp_avgs_evrb[kp_gid] += value;
            kp_madata.update_lp_moving_avg(lpid, EnumNameMetrics(LPMetrics::EVENT_RB), value, init_val);
        }

        for (int kpid = 0; kpid < total_kp; kpid++)
        {
            MovingAvgData& kp_madata = lp_avg_map.at(kpid);
            kp_madata.update_kp_moving_avg(EnumNameMetrics(LPMetrics::EVENT_PROC),
                    kp_avgs_evproc[kpid] / kp_num_lp[kpid], init_val);
            kp_madata.update_kp_moving_avg(EnumNameMetrics(LPMetrics::EVENT_RB),
                    kp_avgs_evrb[kpid] / kp_num_lp[kpid], init_val);
        }
    }
    this->LastStepProcessed = total_steps - 1;
}

