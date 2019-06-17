#include <iostream>
#include <set>
#include <cmath>
#include <numeric>

#include <vtkToolkits.h>
#include <plugins/data/FeatureExtractor.h>
#include <vtkObjectFactory.h>
#include <vtkTable.h>
#include <vtkUnsignedIntArray.h>
#include <vtkFloatArray.h>
#include <vtkVariantArray.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkCompositeDataSet.h>
#include <vtkInformation.h>
#include <vtkPartitionedDataSet.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkCompositeDataIterator.h>
#include <boost/math/distributions/students_t.hpp>

using namespace ross_damaris::data;
using namespace std;
using namespace boost::math;

vtkObjectFactoryNewMacro(FeatureExtractor);

FeatureExtractor::FeatureExtractor() :
    NumSteps(10),
    ExtractionOption(true),
    HypTestOption(true)
{
    this->SetNumberOfInputPorts(2);
    this->SetNumberOfOutputPorts(2);
}

int FeatureExtractor::FillInputPortInformation(int port, vtkInformation* info)
{
    if (port == INPUT_DATA)
    {
        info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
        info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSetCollection");
        return 1;
    }
    else if (port == INPUT_FEATURES)
    {
        info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
        info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSet");
        return 1;
    }
    return 0;
}

int FeatureExtractor::FillOutputPortInformation(int port, vtkInformation* info)
{
    if (port == FULL_FEATURES)
    {
        info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPartitionedDataSet");
        return 1;
    }
    else if (port == SELECTED_FEATURES)
    {
        info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPartitionedDataSet");
        return 1;
    }
    return 0;
}

int FeatureExtractor::RequestData(vtkInformation* request, vtkInformationVector** input_vec,
        vtkInformationVector* output_vec)
{
    vtkPartitionedDataSetCollection* raw_data =
        vtkPartitionedDataSetCollection::GetData(input_vec[INPUT_DATA], 0);
    //vtkCompositeDataIterator* iter = raw_data->NewIterator();
    ////iter->SkipEmptyNodesOn();
    //iter->InitTraversal();

    vtkPartitionedDataSet* input_features =
        vtkPartitionedDataSet::GetData(input_vec[INPUT_FEATURES], 0);

    int num_pds = raw_data->GetNumberOfPartitionedDataSets();

    vtkPartitionedDataSet* pe_data;
    vtkPartitionedDataSet* kp_data;
    vtkPartitionedDataSet* lp_data;

    pe_data = raw_data->GetPartitionedDataSet(toUType(Port::PE_DATA));
    kp_data = raw_data->GetPartitionedDataSet(toUType(Port::KP_DATA));
    lp_data = raw_data->GetPartitionedDataSet(toUType(Port::LP_DATA));

    vtkPartitionedDataSet* full_pds = vtkPartitionedDataSet::GetData(output_vec, FULL_FEATURES);
    full_pds->SetNumberOfPartitions(3);

    vtkPartitionedDataSet* selected_pds = vtkPartitionedDataSet::GetData(output_vec, SELECTED_FEATURES);
    selected_pds->SetNumberOfPartitions(3);

    vtkTable* pe_features = nullptr;
    vtkTable* kp_features = nullptr;
    vtkTable* lp_features = nullptr;

    if (ExtractionOption)
    {
        if (pe_data)
        {
            pe_features = vtkTable::New();
            CalculatePrimaryFeatures<PEMetrics>(pe_data, Port::PE_DATA, pe_features);
        }

        if (kp_data)
        {
            kp_features = vtkTable::New();
            CalculatePrimaryFeatures<KPMetrics>(kp_data, Port::KP_DATA, kp_features);
        }

        if (lp_data)
        {
            lp_features = vtkTable::New();
            CalculatePrimaryFeatures<LPMetrics>(lp_data, Port::LP_DATA, lp_features);
        }
    }
    else
    {
        // TODO probably has some bugs here
        if (!input_features)
            return 0;

        // in this case the full feature table was already extracted and provided as input
        vtkTable* pe_in = vtkTable::SafeDownCast(input_features->GetPartitionAsDataObject(
                    toUType(Port::PE_DATA)));
        vtkTable* kp_in = vtkTable::SafeDownCast(input_features->GetPartitionAsDataObject(
                    toUType(Port::KP_DATA)));
        vtkTable* lp_in = vtkTable::SafeDownCast(input_features->GetPartitionAsDataObject(
                    toUType(Port::LP_DATA)));
        pe_features->ShallowCopy(pe_in);
        kp_features->ShallowCopy(kp_in);
        lp_features->ShallowCopy(lp_in);
    }

    vtkTable* pe_selected = nullptr;
    vtkTable* kp_selected = nullptr;
    vtkTable* lp_selected = nullptr;

    if (HypTestOption)
    {
        if (pe_features)
        {
            pe_selected = vtkTable::New();
            PerformHypothesisTests(pe_features, pe_selected);
        }

        if (kp_features)
        {
            kp_selected = vtkTable::New();
            PerformHypothesisTests(kp_features, kp_selected);
        }

        if (lp_features)
        {
            lp_selected = vtkTable::New();
            PerformHypothesisTests(lp_features, lp_selected);
        }
    }

    full_pds->SetPartition(0, pe_features);
    full_pds->SetPartition(1, kp_features);
    full_pds->SetPartition(2, lp_features);
    selected_pds->SetPartition(0, pe_selected);
    selected_pds->SetPartition(1, kp_selected);
    selected_pds->SetPartition(2, lp_selected);

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

    vtkFloatArray* eff_col = vtkFloatArray::New();
    eff_col->SetName("efficiency");
    features->AddColumn(eff_col);
    eff_col->Delete();

    for (unsigned int p = 0; p < num_partitions; p++)
    {
        vtkTable *table = vtkTable::SafeDownCast(in_data->GetPartitionAsDataObject(p));
        vtkIdType num_row = table->GetNumberOfRows();
        if (num_row - NumSteps <= 0)
            continue;
        vtkIdType start_row = num_row - NumSteps;

        vtkVariantArray* row = vtkVariantArray::New();
        row->SetNumberOfValues(num_metrics * (static_cast<int>(PrimaryFeatures::NUM_FEAT) +
                    static_cast<int>(DerivedFeatures::NUM_FEAT)) + 2);
        // TODO incorrect when using more than 1 compute node
        row->SetValue(0, p);
        int row_idx = 2;
        float ev_proc, ev_rb;

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

                for (int f = static_cast<int>(DerivedFeatures::START);
                        f < static_cast<int>(DerivedFeatures::NUM_FEAT); f++)
                {
                    std::string col_name = metric_name + "/" +
                        EnumNameDerivedFeatures(static_cast<DerivedFeatures>(f));
                    vtkFloatArray* col = vtkFloatArray::New();
                    col->SetName(col_name.c_str());
                    features->AddColumn(col);
                    col->Delete();
                }
            }

            float min_val = table->GetValueByName(0, metric_name.c_str()).ToFloat();
            float max_val = min_val;
            float sum = 0, mean = 0, m2 = 0, m3 = 0, m4 = 0;
            float n, inv_n, val, delta, A, B;

            for (vtkIdType r = start_row; r < num_row; r++)
            {
                n = r + 1.;
                inv_n = 1. / n;

                val = table->GetValueByName(r, metric_name.c_str()).ToFloat();
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

            float std_dev = 0, var = 0, skew = 0, kur = 0;
            if (NumSteps > 1 && m2 > 1.e-150)
            {
                float n = static_cast<float>(NumSteps);
                float inv_n = 1. / n;
                float nm1 = n - 1.;

                // Variance
                var = m2 / nm1;

                // Standard deviation
                std_dev = sqrt(var);

                // Skeweness and kurtosis
                float var_inv = nm1 / m2;
                float nvar_inv = var_inv * inv_n;
                skew = nvar_inv * sqrt(var_inv) * m3;
                kur = nvar_inv * var_inv * m4 - 3.;

            }

            if (static_cast<E>(m) == E::EVENT_PROC)
                ev_proc = sum;
            else if (static_cast<E>(m) == E::EVENT_RB)
                ev_rb = sum;

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

        float net_events = ev_proc - ev_rb;
        float eff = 0;
        if (net_events > 0)
            eff = 1 - (ev_rb/net_events);
        row->SetValue(1, eff);

        features->InsertNextRow(row);
        row->Delete();
        initialized = true;
    }

}

// TODO can probably improve efficiency of this part later
// in particular simplify the math and reduce loops
void FeatureExtractor::PerformHypothesisTests(vtkTable* in_features, vtkTable* selected)
{
    if (!in_features)
        return;
    if (!selected)
        return;

    vtkIdType num_cols = in_features->GetNumberOfColumns();
    vtkFloatArray* eff_col = vtkFloatArray::SafeDownCast(in_features->GetColumnByName("efficiency"));
    auto eff_ranks = GetRankVector(eff_col);
    float eff_mean = accumulate(eff_ranks.begin(), eff_ranks.end(), 0.0) / eff_ranks.size();
    float eff_stddev = 0.0;
    auto n = eff_col->GetNumberOfValues();

    for (int i = 0; i < n; i++)
        eff_stddev += pow(eff_ranks[i] - eff_mean, 2);
    eff_stddev = sqrt(eff_stddev / (n-1));
    //cout << "efficiency details:\n";
    //print_vector(eff_ranks);
    //cout << "eff mean = " << eff_mean << ", std dev = " << eff_stddev << endl;

    if (n <= 2)
        return;
    students_t t_dist(n-2);
    vector<float> p_vals;
    // TODO update to generic entity
    //vtkUnsignedIntArray* pe_array = vtkUnsignedIntArray::New();
    //pe_array->SetName("pe");
    //selected->AddColumn(pe_array);
    //pe_array->Delete();

    for (vtkIdType i = 2; i < num_cols; i++)
    {
        //vtkDoubleArray* new_col = vtkDoubleArray::New();
        //new_col->SetName(in_features->GetColumnName(i));
        //selected->AddColumn(new_col);
        //new_col->Delete();

        vtkFloatArray* col = vtkFloatArray::SafeDownCast(in_features->GetColumn(i));
        auto col_ranks = GetRankVector(col);
        float col_mean = accumulate(col_ranks.begin(), col_ranks.end(), 0.0) / col_ranks.size();
        float col_stddev = 0.0;
        for (int i = 0; i < n; i++)
            col_stddev += pow(col_ranks[i] - col_mean, 2);
        col_stddev = sqrt(col_stddev / (n-1));
        //cout << "col var details:\n";
        //print_vector(col_ranks);
        //cout << "col mean = " << col_mean << ", std dev = " << col_stddev << endl;

        float denom = col_stddev * eff_stddev;
        float cov = 0.0;
        for (int i = 0; i < n; i++)
            cov += (col_ranks[i] - col_mean) * (eff_ranks[i] - eff_mean);
        cov /= (n - 1);
        //cout << "cov = " << cov << ", denom = " << denom << endl;

        float r = 0.0;
        if (denom != 0)
            r = cov / denom;
        if (r > 0)
            r -= 0.0001;
        else if (r < 0)
            r += 0.0001;
        float t = (r * sqrt(n-2)) / sqrt(1 - r * r);
        //cout << "r = " << r << ", t = " << t;
        // mult by 2 for 2-tailed test
        float p = 2 * cdf(complement(t_dist, fabs(t)));
        //cout << ", p-val: " << p << endl;
        p_vals.push_back(p);
    }
    // Now use p_vals to selected which features to use in selected table
    auto is_selected = benjamini_yekutieli(p_vals, 0.05);
    //print_vector(is_selected);
    selected->AddColumn(in_features->GetColumn(0));
    for (int i = 0; i < is_selected.size(); i++)
    {
        if (is_selected[i])
        {
            selected->AddColumn(in_features->GetColumn(i+2));
            //cout << in_features->GetColumnName(i+2) << ": p-val = " << p_vals[i] << endl;
        }
    }
    //selected->Dump(18, 10);
}

template <typename T>
float FeatureExtractor::GetColumnMean(const T* array)
{
    float sum = 0.0;
    auto n = array->GetNumberOfValues();
    for (vtkIdType i = 0; i < n; i++)
        sum += array->GetValue(i);
    return sum / n;
}

std::vector<float> FeatureExtractor::GetRankVector(const vtkFloatArray* array)
{
    int n = array->GetNumberOfValues();
    vector<float> ranks(n);
    vector<pair<float, int>> tuples;
    for (int i = 0; i < n; i++)
    {
        tuples.push_back(make_pair(array->GetValue(i), i));
    }

    sort(tuples.begin(), tuples.end());

    int i = 0, j;
    while (i < n)
    {
        j = i;
        float sum = i + 1;
        while (j < n && tuples[j].first == tuples[j+1].first)
        {
            j++;
            sum += j + 1;
        }

        int num_ties = j - i + 1;
        float rank = sum / num_ties;

        while (i <= j && i < n)
        {
            ranks[tuples[i].second] = rank;
            i++;
        }
    }
    return ranks;
}

template <typename T>
void FeatureExtractor::print_vector(vector<T> vect)
{
    cout << "[";
    for (auto i : vect)
        cout << i << " ";
    cout << "]" << endl;
}

vector<bool> FeatureExtractor::benjamini_yekutieli(vector<float>& pvals, float alpha)
{
    vector<pair<float, int>> tuples;
    int m = pvals.size();
    for (int i = 0; i < m; i++)
        tuples.push_back(make_pair(pvals[i], i));
    sort(tuples.begin(), tuples.end());

    vector<int> K;
    for (int i = 1; i <= m; i++)
        K.push_back(i);

    vector<float> C;
    C.push_back(1.0 / K[0]);
    for (int i = 1; i < K.size(); i++)
        C.push_back(C[i-1] + (1.0 / K[i]));

    vector<float> T;
    for (int i = 0; i < K.size(); i++)
        T.push_back((alpha * K[i]) / (m * C[i]));

    int k_max = m;
    vector<bool> result(pvals.size());
    for (int i = 0; i < K.size(); i++)
    {
        if (tuples[i].first > T[i])
        {
            k_max = i;
            break;
        }
    }

    // TODO putting a limit on this for now
    if (k_max > 9)
        k_max = 9;

    // need to put answers in original order
    for (int i = 0; i < tuples.size(); i++)
    {
        if (i <= k_max)
            result[tuples[i].second] = true;
        else
            result[tuples[i].second] = false;

    }
    return result;
}
