#ifndef FEATURE_EXTRACTOR_H
#define FEATURE_EXTRACTOR_H

#include <vtkTableAlgorithm.h>
#include <vtkPartitionedDataSet.h>
#include <vtkFloatArray.h>
#include <plugins/data/Features.h>
#include <vector>

namespace ross_damaris {
namespace data {

class FeatureExtractor : public vtkTableAlgorithm
{
public:
    vtkTypeMacro(FeatureExtractor, vtkTableAlgorithm);
    static FeatureExtractor* New();

    enum InputPorts
    {
        INPUT_DATA,
        INPUT_FEATURES
    };

    enum OutputPorts
    {
        FULL_FEATURES,
        SELECTED_FEATURES
    };

    vtkSetMacro(NumSteps, int);
    vtkGetMacro(NumSteps, int);

    vtkSetMacro(ExtractionOption, bool);
    vtkGetMacro(ExtractionOption, bool);

    vtkSetMacro(HypTestOption, bool);
    vtkGetMacro(HypTestOption, bool);

protected:
    FeatureExtractor();
    ~FeatureExtractor() = default;

    int FillInputPortInformation(int port, vtkInformation* info) override;
    int FillOutputPortInformation(int port, vtkInformation* info) override;

    int RequestData(vtkInformation* request, vtkInformationVector** input_vec,
            vtkInformationVector* output_vec) override;

    template <typename E>
    void CalculatePrimaryFeatures(vtkPartitionedDataSet* in_data, Port data_type, vtkTable* features);

    void PerformHypothesisTests(vtkTable* in_features, vtkTable *selected);

    template <typename T>
    float GetColumnMean(const T* array);

    std::vector<float> GetRankVector(const vtkFloatArray* array);
    std::vector<bool> benjamini_yekutieli(std::vector<float>& pvals, float alpha);

    template <typename T>
    void print_vector(std::vector<T> vect);

    int NumSteps;
    bool ExtractionOption;
    bool HypTestOption;

private:
    FeatureExtractor(const FeatureExtractor&) = delete;
    FeatureExtractor& operator=(const FeatureExtractor&) = delete;
};

} // end namespace data
} // end namespace ross_damaris

#endif // FEATURE_EXTRACTOR_H
