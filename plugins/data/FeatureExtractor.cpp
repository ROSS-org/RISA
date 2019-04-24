#include <iostream>
#include <set>

#include <vtkToolkits.h>
#include <plugins/data/FeatureExtractor.h>
#include <vtkObjectFactory.h>
#include <vtkTable.h>
#include <vtkUnsignedIntArray.h>
#include <vtkDoubleArray.h>
#include <vtkVariantArray.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkCompositeDataSet.h>
#include <vtkInformation.h>
#include <vtkPartitionedDataSet.h>
#include <vtkExtractSelectedRows.h>

using namespace ross_damaris::data;
using namespace std;

vtkObjectFactoryNewMacro(FeatureExtractor);

FeatureExtractor::FeatureExtractor() :
    NumSteps(10)
{
    this->SetNumberOfInputPorts(3);
    this->SetNumberOfOutputPorts(3);
}

int FeatureExtractor::FillInputPortInformation(int port, vtkInformation* info)
{
    Port p = static_cast<Port>(port);
    if (p == Port::PE_DATA || p == Port::KP_DATA || p == Port::LP_DATA)
    {
        info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
        info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSet");
        return 1;
    }
    return 0;
}

int FeatureExtractor::RequestData(vtkInformation* request, vtkInformationVector** input_vec,
        vtkInformationVector* output_vec)
{
    vtkPartitionedDataSet* pe_data = vtkPartitionedDataSet::GetData(input_vec[toUType(Port::PE_DATA)], 0);
    vtkPartitionedDataSet* kp_data = vtkPartitionedDataSet::GetData(input_vec[toUType(Port::KP_DATA)], 0);
    vtkPartitionedDataSet* lp_data = vtkPartitionedDataSet::GetData(input_vec[toUType(Port::LP_DATA)], 0);

    vtkTable* pe_features = vtkTable::GetData(output_vec, toUType(Port::PE_DATA));
    vtkTable* kp_features = vtkTable::GetData(output_vec, toUType(Port::KP_DATA));
    vtkTable* lp_features = vtkTable::GetData(output_vec, toUType(Port::LP_DATA));

    if (pe_data)
        CalculatePrimaryFeatures<PEMetrics>(pe_data, Port::PE_DATA, pe_features);

    if (kp_data)
        CalculatePrimaryFeatures<KPMetrics>(kp_data, Port::KP_DATA, kp_features);

    if (lp_data)
        CalculatePrimaryFeatures<LPMetrics>(lp_data, Port::LP_DATA, lp_features);

    return 1;
}

template <typename E>
void FeatureExtractor::CalculatePrimaryFeatures(vtkPartitionedDataSet* in_data, Port data_type,
        vtkTable* features)
{
    if (!in_data)
        return;
    if (!features)
        return;

    bool initialized = false;
    int num_metrics = get_num_metrics<E>();
    std::string entity_name = EnumNamePort(data_type);

    unsigned int num_partitions = in_data->GetNumberOfPartitions();

    vtkUnsignedIntArray* entity_col = vtkUnsignedIntArray::New();
    entity_col->SetName(entity_name.c_str());
    features->AddColumn(entity_col);
    entity_col->Delete();

    for (unsigned int p = 0; p < num_partitions; p++)
    {
        vtkTable *table = vtkTable::SafeDownCast(in_data->GetPartitionAsDataObject(p));
        vtkIdType num_row = table->GetNumberOfRows();
        if (num_row - NumSteps <= 0)
            continue;
        vtkIdType start_row = num_row - NumSteps;

        vtkVariantArray* row = vtkVariantArray::New();
        row->SetNumberOfValues(num_metrics * (static_cast<int>(PrimaryFeatures::NUM_FEAT) +
                    static_cast<int>(DerivedFeatures::NUM_FEAT)) + 1);
        // TODO incorrect when using more than 1 compute node
        row->SetValue(0, p);
        int row_idx = 1;

        for (int m = 0; m < num_metrics; m++)
        {
            std::string metric_name = EnumNameMetrics(static_cast<E>(m));
            if (!initialized)
            {
                for (int f = static_cast<int>(PrimaryFeatures::START);
                        f < static_cast<int>(PrimaryFeatures::NUM_FEAT); f++)
                {
                    std::string col_name = metric_name + "_" +
                        EnumNamePrimaryFeatures(static_cast<PrimaryFeatures>(f));
                    vtkDoubleArray* col = vtkDoubleArray::New();
                    col->SetName(col_name.c_str());
                    features->AddColumn(col);
                    col->Delete();
                }

                for (int f = static_cast<int>(DerivedFeatures::START);
                        f < static_cast<int>(DerivedFeatures::NUM_FEAT); f++)
                {
                    std::string col_name = metric_name + "_" +
                        EnumNameDerivedFeatures(static_cast<DerivedFeatures>(f));
                    vtkDoubleArray* col = vtkDoubleArray::New();
                    col->SetName(col_name.c_str());
                    features->AddColumn(col);
                    col->Delete();
                }
            }

            double min_val = table->GetValueByName(0, metric_name.c_str()).ToDouble();
            double max_val = min_val;
            double sum = 0, mean = 0, m2 = 0, m3 = 0, m4 = 0;
            double n, inv_n, val, delta, A, B;

            for (vtkIdType r = start_row; r < num_row; r++)
            {
                n = r + 1.;
                inv_n = 1. / n;

                val = table->GetValueByName(r, metric_name.c_str()).ToDouble();
                delta = val - mean;
                sum += val;

                A = delta * inv_n;
                mean += A;
                m4 += A * ( A * A * delta * r * ( n * ( n - 3. ) + 3. ) + 6. * A * m2 - 4. * m3  );

                B = val - mean;
                m3 += A * ( B * delta * ( n - 2. ) - 3. * m2 );
                m2 += delta * B;

                if (val < min_val)
                    min_val = val;
                else if (val > max_val)
                    max_val = val;

            }

            double std_dev = 0, var = 0, skew = 0, kur = 0;
            if (num_row > 1 && m2 > 1.e-150)
            {
                double n = static_cast<double>(num_row);
                double inv_n = 1. / n;
                double nm1 = n - 1.;

                // Variance
                var = m2 / nm1;

                // Standard deviation
                std_dev = sqrt(var);

                // Skeweness and kurtosis
                double var_inv = nm1 / m2;
                double nvar_inv = var_inv * inv_n;
                skew = nvar_inv * sqrt(var_inv) * m3;
                kur = nvar_inv * var_inv * m4 - 3.;

            }

            row->SetValue(row_idx, min_val);
            row->SetValue(row_idx + 1, max_val);
            row->SetValue(row_idx + 2, sum);
            row->SetValue(row_idx + 3, mean);
            row->SetValue(row_idx + 4, m2);
            row->SetValue(row_idx + 5, m3);
            row->SetValue(row_idx + 6, m4);
            row->SetValue(row_idx + 7, std_dev);
            row->SetValue(row_idx + 8, var);
            row->SetValue(row_idx + 9, skew);
            row->SetValue(row_idx + 10, kur);
            row_idx += static_cast<int>(PrimaryFeatures::NUM_FEAT) +
                static_cast<int>(DerivedFeatures::NUM_FEAT);
        }
        features->InsertNextRow(row);
        row->Delete();
        initialized = true;
    }

}

