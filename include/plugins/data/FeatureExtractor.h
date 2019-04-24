#ifndef FEATURE_EXTRACTOR_H
#define FEATURE_EXTRACTOR_H

#include <vtkTableAlgorithm.h>
#include <vtkPartitionedDataSet.h>
#include <plugins/data/Features.h>

namespace ross_damaris {
namespace data {

class FeatureExtractor : public vtkTableAlgorithm
{
public:
    vtkTypeMacro(FeatureExtractor, vtkTableAlgorithm);
    static FeatureExtractor* New();

    vtkSetMacro(NumSteps, int);
    vtkGetMacro(NumSteps, int);

protected:
    FeatureExtractor();
    ~FeatureExtractor() = default;

    int FillInputPortInformation(int port, vtkInformation* info) override;

    int RequestData(vtkInformation* request, vtkInformationVector** input_vec,
            vtkInformationVector* output_vec) override;

    template <typename E>
    void CalculatePrimaryFeatures(vtkPartitionedDataSet* in_data, Port data_type, vtkTable* features);

    int NumSteps;

private:
    FeatureExtractor(const FeatureExtractor&) = delete;
    FeatureExtractor& operator=(const FeatureExtractor&) = delete;
};

} // end namespace data
} // end namespace ross_damaris

#endif // FEATURE_EXTRACTOR_H
