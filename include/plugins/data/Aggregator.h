#ifndef AGGREGATOR_H
#define AGGREGATOR_H

#include <vtkTableAlgorithm.h>
#include <vtkPartitionedDataSet.h>
#include <vtkFloatArray.h>
#include <plugins/data/Features.h>
#include <vector>

namespace ross_damaris {
namespace data {

class Aggregator : public vtkTableAlgorithm
{
public:
    vtkTypeMacro(Aggregator, vtkTableAlgorithm);
    static Aggregator* New();

    enum InputPorts
    {
        INPUT_PE,
        INPUT_KP,
        INPUT_LP
    };

    enum OutputPorts
    {
        FULL_FEATURES,
    };

    vtkSetMacro(NumSteps, int);
    vtkGetMacro(NumSteps, int);

    vtkSetMacro(TS, double);
    vtkGetMacro(TS, double);

    vtkSetMacro(AggPE, bool);
    vtkGetMacro(AggPE, bool);

    vtkSetMacro(AggKP, bool);
    vtkGetMacro(AggKP, bool);

    vtkSetMacro(AggLP, bool);
    vtkGetMacro(AggLP, bool);

protected:
    Aggregator();
    ~Aggregator() = default;

    int FillInputPortInformation(int port, vtkInformation* info) override;
    int FillOutputPortInformation(int port, vtkInformation* info) override;

    int RequestData(vtkInformation* request, vtkInformationVector** input_vec,
            vtkInformationVector* output_vec) override;

    template <typename E>
    void CalculateFeatures(vtkPartitionedDataSet* in_data, Port data_type, vtkTable* Features);

    vtkIdType FindTSRow(vtkTable* entity_data, double ts);

    int NumSteps;
    double TS;
    bool AggPE;
    bool AggKP;
    bool AggLP;

private:
    Aggregator(const Aggregator&) = delete;
    Aggregator& operator=(const Aggregator&) = delete;
};

} // end namespace data
} // end namespace ross_damaris
#endif // AGGREGATOR_H
