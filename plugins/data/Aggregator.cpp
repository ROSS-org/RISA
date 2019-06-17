#include <plugins/data/Aggregator.h>
#include <vtkInformation.h>
#include <vtkPartitionedDataSet.h>
#include <vtkTable.h>
#include <vtkUnsignedIntArray.h>
#include <vtkFloatArray.h>
#include <vtkVariantArray.h>

using namespace ross_damaris::data;

vtkObjectFactoryNewMacro(Aggregator);

Aggregator::Aggregator() :
    NumSteps(100),
    TS(0.0),
    AggPE(true),
    AggKP(false),
    AggLP(false)
{
    this->SetNumberOfInputPorts(2);
    this->SetNumberOfOutputPorts(1);
}

int Aggregator::FillInputPortInformation(int port, vtkInformation* info)
{
    if (port == INPUT_PE)
    {
        info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
        info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSet");
        return 1;
    }
    else if (port == 1)
    {
        info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
        info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSet");
        return 1;
    }
    return 0;
}

int Aggregator::FillOutputPortInformation(int port, vtkInformation* info)
{
    if (port == FULL_FEATURES)
    {
        info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPartitionedDataSet");
        return 1;
    }
    return 0;
}

int Aggregator::RequestData(vtkInformation* request, vtkInformationVector** input_vec,
        vtkInformationVector* output_vec)
{
    vtkPartitionedDataSet* pe_data = vtkPartitionedDataSet::GetData(input_vec[INPUT_PE], 0);
    vtkTable* pe_features = nullptr;

    vtkPartitionedDataSet* full_pds = vtkPartitionedDataSet::GetData(output_vec, FULL_FEATURES);
    full_pds->SetNumberOfPartitions(3);

    if (AggPE && pe_data)
    {
        pe_features = vtkTable::New();
        CalculateFeatures<PEMetrics>(pe_data, Port::PE_DATA, pe_features);
    }

    full_pds->SetPartition(0, pe_features);

    return 1;
}

vtkIdType Aggregator::FindTSRow(vtkTable* entity_data, double ts)
{
    // start at end, of table, since that should be faster in practice

    for (vtkIdType row = entity_data->GetNumberOfRows() - 1; row >= 0; row--)
    {
        if (ts == entity_data->GetValueByName(row, "timestamp").ToDouble())
            return row;
    }
    return -1;
}

template <typename E>
void Aggregator::CalculateFeatures(vtkPartitionedDataSet* in_data, Port data_type, vtkTable* features)
{
    if (!in_data)
        return;
    if (!features)
        return;

    bool initialized = false;
    int num_metrics = get_num_metrics<E>();
    std::string entity_name = EnumNamePort(data_type);

    // num entities
    unsigned int num_partitions = in_data->GetNumberOfPartitions();

    vtkUnsignedIntArray* entity_col = vtkUnsignedIntArray::New();
    entity_col->SetName(entity_name.c_str());
    features->AddColumn(entity_col);
    entity_col->Delete();

    vtkIdType end_row = -1;
    for (unsigned int p = 0; p < num_partitions; p++)
    {
        vtkTable *table = vtkTable::SafeDownCast(in_data->GetPartitionAsDataObject(p));
        // TODO need to find the row containing TS, and then it's numsteps back from there
        // all entities will have the same TS in the same Row Id, so only need to find on first entity
        if (p == 0)
            end_row = FindTSRow(table, TS);
        if (end_row - NumSteps <= 0)
            return;
        vtkIdType start_row = end_row - NumSteps;
        vtkIdType num_row = NumSteps;

        vtkVariantArray* row = vtkVariantArray::New();
        row->SetNumberOfValues(num_metrics * static_cast<int>(PrimaryFeatures::NUM_FEAT) + 1);
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
                    std::string col_name = metric_name + "/" +
                        EnumNamePrimaryFeatures(static_cast<PrimaryFeatures>(f));
                    vtkFloatArray* col = vtkFloatArray::New();
                    col->SetName(col_name.c_str());
                    features->AddColumn(col);
                    col->Delete();
                }
            }

            float min_val = table->GetValueByName(0, metric_name.c_str()).ToFloat();
            float max_val = min_val;
            float sum = 0, mean = 0, m2 = 0;
            float n, inv_n, val, delta, A, B;

            for (vtkIdType r = start_row; r < end_row; r++)
            {
                n = r + 1.;
                inv_n = 1. / n;

                val = table->GetValueByName(r, metric_name.c_str()).ToFloat();
                delta = val - mean;
                sum += val;

                A = delta * inv_n;
                mean += A;

                B = val - mean;
                m2 += delta * B;

                if (val < min_val)
                    min_val = val;
                else if (val > max_val)
                    max_val = val;

            }

            float std_dev = 0, var = 0;
            if (num_row > 1 && m2 > 1.e-150)
            {
                float n = static_cast<float>(num_row);
                float inv_n = 1. / n;
                float nm1 = n - 1.;

                // Variance
                var = m2 / nm1;

                // Standard deviation
                std_dev = sqrt(var);
            }

            row->SetValue(row_idx, min_val);
            row->SetValue(row_idx + 1, max_val);
            row->SetValue(row_idx + 2, sum);
            row->SetValue(row_idx + 3, mean);
            row->SetValue(row_idx + 4, std_dev);
            row->SetValue(row_idx + 5, var);
            row_idx += static_cast<int>(PrimaryFeatures::NUM_FEAT);
        }

        features->InsertNextRow(row);
        row->Delete();
        initialized = true;
    }

}
