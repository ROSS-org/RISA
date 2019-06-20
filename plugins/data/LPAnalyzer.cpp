#include <plugins/data/LPAnalyzer.h>
#include <plugins/data/Features.h>
#include <vtkInformation.h>
#include <vtkPartitionedDataSet.h>
#include <vtkTable.h>
#include <vtkUnsignedIntArray.h>
#include <vtkFloatArray.h>
#include <vtkVariantArray.h>

using namespace ross_damaris::data;

vtkObjectFactoryNewMacro(LPAnalyzer);

LPAnalyzer::LPAnalyzer()
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
        kp_it->second.kp_avgs[EnumNameMetrics(LPMetrics::EVENT_PROC)] = std::make_pair(0.0, 0.0);;
        kp_it->second.kp_avgs[EnumNameMetrics(LPMetrics::EVENT_RB)] = std::make_pair(0.0, 0.0);

        MovingAvgData::metric_map m_map;
        m_map[EnumNameMetrics(LPMetrics::EVENT_PROC)] = std::make_pair(0.0, 0.0);
        m_map[EnumNameMetrics(LPMetrics::EVENT_RB)] = std::make_pair(0.0, 0.0);
        kp_it->second.lp_avgs.insert(std::make_pair(lpid, std::move(m_map)));
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
    
    return 1;
}

void LPAnalyzer::CalculateMovingAverages(vtkPartitionedDataSet* in_data)
{
    
}

