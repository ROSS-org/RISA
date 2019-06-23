#ifndef LP_ANALYZER_H
#define LP_ANALYZER_H

#include <vtkTableAlgorithm.h>
#include <vtkPartitionedDataSet.h>
#include <vtkFloatArray.h>
#include <plugins/data/Features.h>
#include <plugins/data/MovingAvgData.h>
#include <plugins/util/SimConfig.h>
#include <plugins/streaming/StreamClient.h>
#include <plugins/data/VTIndex.h>
#include <vector>

namespace ross_damaris {
namespace data {

class LPAnalyzer : public vtkTableAlgorithm
{
public:
    vtkTypeMacro(LPAnalyzer, vtkTableAlgorithm);
    static LPAnalyzer* New();

    enum InputPorts
    {
        LP_INPUT
    };

    enum OutputPorts
    {
        LP_OUTPUT,
        AVG_OUTPUT
    };

    vtkSetMacro(TS, double);
    vtkGetMacro(TS, double);

    size_t FindProblematicLPs(vtkPartitionedDataSet* lp_pds, TSIndex& samples);

protected:
    LPAnalyzer();
    ~LPAnalyzer() = default;

    int FillInputPortInformation(int port, vtkInformation* info) override;
    int FillOutputPortInformation(int port, vtkInformation* info) override;

    int RequestData(vtkInformation* request, vtkInformationVector** input_vec,
            vtkInformationVector* output_vec) override;

    void CalculateMovingAverages(vtkPartitionedDataSet* in_data);
    vtkIdType FindTSRow(vtkTable* entity_data, double ts);
    //void GetProblematicLPs(vtkPartitionedDataSet* in_data, vtkPartitionedDataSet* out_data);

private:
    LPAnalyzer(const LPAnalyzer&) = delete;
    LPAnalyzer& operator=(const LPAnalyzer&) = delete;

    int LastStepProcessed;
    double TS;
    int total_flagged;
    bool initial_round;
    std::map<int, MovingAvgData> lp_avg_map;
    std::set<double> rows_to_write;
    config::SimConfig* sim_config;
    streaming::StreamClient* stream_client;
};

} // end namespace data
} // end namespace ross_damaris

#endif // LP_ANALYZER_H
