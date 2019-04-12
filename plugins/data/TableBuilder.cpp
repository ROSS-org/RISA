#include <plugins/data/TableBuilder.h>
#include <instrumentation/st-instrumentation-internal.h>
#include <iostream>

using namespace std;

vtkStdString pe_cols[] =
{
    "event_proc",
    "event_abort",
    "event_rbs",
    "rb_total",
    "rb_primary",
    "rb_secondary",
    "fc_attempts",
    "pq_size",
    "net_send",
    "net_recv",
    "num_gvt",
    "pe_event_ties",
    "all_red_count",
    "net_read_time",
    "net_other_time",
    "gvt_time",
    "fc_time",
    "ev_abort_time",
    "ev_proc_time",
    "pq_time",
    "rb_time",
    "cancelq_time",
    "avl_time",
    "buddy_time",
    "lz4_time"
};

vtkStdString kp_cols[] =
{
    "event_proc",
    "event_abort",
    "event_rbs",
    "rb_total",
    "rb_primary",
    "rb_secondary",
    "net_send",
    "net_recv",
    "virtual_time_diff"
};

vtkStdString lp_cols[] =
{
    "event_proc",
    "event_abort",
    "event_rbs",
    "net_send",
    "net_recv"
};

using namespace ross_damaris::data;

TableBuilder::TableBuilder() :
    collection(vtkPartitionedDataSetCollection::New()),
    pe_pds(vtkPartitionedDataSet::New()),
    kp_pds(vtkPartitionedDataSet::New()),
    lp_pds(vtkPartitionedDataSet::New()),
    sim_config_(config::SimConfig::get_instance())
{
    // vtkPartitionedDataSetCollection will have 3 vtkPartitionedDataSets
    // one each for PEs, KPs, and LPs
    // Then each partition will be an entity id and its data in a vtkTable
    // TODO need mapping from local ids to ROSS global ids
    pe_pds->SetNumberOfPartitions(sim_config_->num_local_pe());
    for (int i = 0; i < sim_config_->num_local_pe(); i++)
    {
        vtkTable *cur_table = vtkTable::New();
        for (int i = 0; i < NUM_PE_UINT - 1; i++)
        {
            vtkSmartPointer<vtkUnsignedIntArray> col = vtkUnsignedIntArray::New();
            col->SetNumberOfComponents(1);
            col->SetName(pe_cols[i].c_str());
            col->SetNumberOfTuples(0);
            cur_table->AddColumn(col);
        }
        for (int i = 0; i < NUM_PE_FLOAT; i++)
        {
            vtkSmartPointer<vtkFloatArray> col = vtkFloatArray::New();
            col->SetNumberOfComponents(1);
            col->SetName(pe_cols[NUM_PE_UINT - 1 + i].c_str());
            col->SetNumberOfTuples(0);
            cur_table->AddColumn(col);
        }
        pe_pds->SetPartition(i, cur_table);
    }

    kp_pds->SetNumberOfPartitions(sim_config_->num_local_pe() * sim_config_->num_kp_pe());
    for (int i = 0; i < sim_config_->num_local_pe() * sim_config_->num_kp_pe(); i++)
    {
        vtkTable *cur_table = vtkTable::New();
        for (int i = 0; i < NUM_KP_UINT - 1; i++)
        {
            vtkSmartPointer<vtkUnsignedIntArray> col = vtkUnsignedIntArray::New();
            col->SetNumberOfComponents(1);
            col->SetName(kp_cols[i].c_str());
            col->SetNumberOfTuples(0);
            cur_table->AddColumn(col);
        }
        for (int i = 0; i < NUM_KP_FLOAT; i++)
        {
            vtkSmartPointer<vtkFloatArray> col = vtkFloatArray::New();
            col->SetNumberOfComponents(1);
            col->SetName(kp_cols[NUM_KP_UINT - 1 + i].c_str());
            col->SetNumberOfTuples(0);
            cur_table->AddColumn(col);
        }
        kp_pds->SetPartition(i, cur_table);
    }

    lp_pds->SetNumberOfPartitions(sim_config_->num_local_lp());
    for (int i = 0; i < sim_config_->num_local_lp(); i++)
    {
        vtkTable *cur_table = vtkTable::New();
        for (int i = 0; i < NUM_LP_UINT - 2; i++)
        {
            vtkSmartPointer<vtkUnsignedIntArray> col = vtkUnsignedIntArray::New();
            col->SetNumberOfComponents(1);
            col->SetName(lp_cols[i].c_str());
            col->SetNumberOfTuples(0);
            cur_table->AddColumn(col);
        }
        lp_pds->SetPartition(i, cur_table);
        //cout << "lp " << i << " num rows " << cur_table->GetNumberOfRows() <<
        //    " num cols " << cur_table->GetNumberOfColumns() << endl;
    }

    collection->SetNumberOfPartitionedDataSets(NUM_PARTITIONS);
    collection->SetPartitionedDataSet(PE_PARTITION, pe_pds);
    collection->SetPartitionedDataSet(KP_PARTITION, kp_pds);
    collection->SetPartitionedDataSet(LP_PARTITION, lp_pds);
}

void TableBuilder::save_data(char* buffer, size_t buf_size)
{
    // pe_pds->GetNumberOfPartitions();
    // vtkDataObject = pe_pds->GetPartionAsDataObject(id);

    sample_metadata *sample_md = reinterpret_cast<sample_metadata*>(buffer);
    buffer += sizeof(*sample_md);
    buf_size -= sizeof(*sample_md);

    if (sample_md->has_pe)
    {
        // create a vtkVariantArray, which will become a row for this PE's table
        vtkSmartPointer<vtkVariantArray> arr = vtkVariantArray::New();

        unsigned int* pe_int_data = reinterpret_cast<unsigned int*>(buffer);
        buffer += sizeof(unsigned int) * NUM_PE_UINT;
        buf_size -= sizeof(unsigned int) * NUM_PE_UINT;
        for (int i = 1; i < NUM_PE_UINT; i++)
            arr->InsertNextValue(pe_int_data[i]);

        float *pe_float_data = reinterpret_cast<float*>(buffer);
        buffer += sizeof(float) * NUM_PE_FLOAT;
        buf_size -= sizeof(float) * NUM_PE_FLOAT;
        for (int i = 0; i < NUM_PE_FLOAT; i++)
            arr->InsertNextValue(pe_float_data[i]);

        vtkSmartPointer<vtkTable> pe_table =
            vtkTable::SafeDownCast(pe_pds->GetPartitionAsDataObject(sample_md->peid));

        pe_table->InsertNextRow(arr);
        //printf("saved PE %d for GVT %f: num rows %lld, num cols %lld\n",
        //        sample_md->peid, sample_md->last_gvt, pe_table->GetNumberOfRows(),
        //        pe_table->GetNumberOfColumns());
        //if (pe_table->GetNumberOfRows() >= 17 && sample_md->peid == 0)
        //    pe_table->Dump(7, 20);
    }

    for (int kp = 0; kp < sample_md->has_kp; kp++)
    {
        // TODO potentially some bugs in here for padding in the structs
        // used to save the data on the ROSS side
        int kpid = sample_md->peid * sim_config_->num_kp_pe() + kp;
        // create a vtkVariantArray, which will become a row for this PE's table
        vtkSmartPointer<vtkVariantArray> arr = vtkVariantArray::New();

        unsigned int* kp_int_data = reinterpret_cast<unsigned int*>(buffer);
        buffer += sizeof(unsigned int) * NUM_KP_UINT;
        buf_size -= sizeof(unsigned int) * NUM_KP_UINT;
        for (int i = 1; i < NUM_KP_UINT; i++)
            arr->InsertNextValue(kp_int_data[i]);

        float *kp_float_data = reinterpret_cast<float*>(buffer);
        buffer += sizeof(float) * NUM_KP_FLOAT;
        buf_size -= sizeof(float) * NUM_KP_FLOAT;
        for (int i = 0; i < NUM_KP_FLOAT; i++)
            arr->InsertNextValue(kp_float_data[i]);

        vtkSmartPointer<vtkTable> kp_table =
            vtkTable::SafeDownCast(kp_pds->GetPartitionAsDataObject(kpid));

        kp_table->InsertNextRow(arr);
        //printf("saved KP %d for GVT %f: num rows %lld, num cols %lld\n",
        //        kpid, sample_md->last_gvt, kp_table->GetNumberOfRows(),
        //        kp_table->GetNumberOfColumns());
        //if (kp_table->GetNumberOfRows() >= 17 && kpid == 0)
        //    kp_table->Dump(7, 20);
    }

    for (int lp = 0; lp < sample_md->has_lp; lp++)
    {
        // create a vtkVariantArray, which will become a row for this PE's table
        vtkSmartPointer<vtkVariantArray> arr = vtkVariantArray::New();

        unsigned int* lp_int_data = reinterpret_cast<unsigned int*>(buffer);
        buffer += sizeof(unsigned int) * NUM_LP_UINT;
        buf_size -= sizeof(unsigned int) * NUM_LP_UINT;
        // TODO this isn't necessarily correct
        int lpid = static_cast<int>(lp_int_data[1]);
        for (int i = 2; i < NUM_LP_UINT; i++)
            arr->InsertNextValue(lp_int_data[i]);

        vtkSmartPointer<vtkTable> lp_table =
            vtkTable::SafeDownCast(lp_pds->GetPartitionAsDataObject(lpid));

        lp_table->InsertNextRow(arr);
        //printf("saved lp %d for GVT %f: num rows %lld, num cols %lld\n",
        //        lpid, sample_md->last_gvt, lp_table->GetNumberOfRows(),
        //        lp_table->GetNumberOfColumns());
        //if (lp_table->GetNumberOfRows() >= 17 && lpid == 0)
        //    lp_table->Dump(7, 20);
    }
}
