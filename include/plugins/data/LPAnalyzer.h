#ifndef LP_ANALYZER_H
#define LP_ANALYZER_H

#include <vtkTableAlgorithm.h>
#include <vtkPartitionedDataSet.h>
#include <vtkFloatArray.h>
#include <plugins/data/Features.h>
#include <plugins/data/MovingAvgData.h>
#include <plugins/util/SimConfig.h>
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

protected:
    LPAnalyzer();
    ~LPAnalyzer() = default;

    int FillInputPortInformation(int port, vtkInformation* info) override;
    int FillOutputPortInformation(int port, vtkInformation* info) override;

    int RequestData(vtkInformation* request, vtkInformationVector** input_vec,
            vtkInformationVector* output_vec) override;

    void CalculateMovingAverages(vtkPartitionedDataSet* in_data);

private:
    LPAnalyzer(const LPAnalyzer&) = delete;
    LPAnalyzer& operator=(const LPAnalyzer&) = delete;

    int LastStep;
    std::map<int, MovingAvgData> lp_avg_map;
    config::SimConfig* sim_config;
};

} // end namespace data
} // end namespace ross_damaris

#endif // LP_ANALYZER_H
