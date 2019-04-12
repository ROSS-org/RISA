#ifndef TABLE_BUILDER_H
#define TABLE_BUILDER_H

#include <memory>

#include <vtkSmartPointer.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkPartitionedDataSet.h>
#include <vtkTable.h>
#include <vtkVariantArray.h>
#include <vtkStdString.h>
#include <vtkUnsignedIntArray.h>
#include <vtkFloatArray.h>

#include <plugins/util/SimConfig.h>

#define NUM_PE_UINT 14
#define NUM_PE_FLOAT 12
#define NUM_KP_UINT 9
#define NUM_KP_FLOAT 1
#define NUM_LP_UINT 7

namespace ross_damaris {
namespace data{

struct TableBuilder
{

    enum {
        PE_PARTITION,
        KP_PARTITION,
        LP_PARTITION,
        NUM_PARTITIONS
    };

    // TODO may have to do things differently when wanting to make multiple argobots threads
    // work on this
    vtkSmartPointer<vtkPartitionedDataSetCollection> collection;
    vtkSmartPointer<vtkPartitionedDataSet> pe_pds;
    vtkSmartPointer<vtkPartitionedDataSet> kp_pds;
    vtkSmartPointer<vtkPartitionedDataSet> lp_pds;

    config::SimConfig* sim_config_;

    TableBuilder();
    void save_data(char* buffer, size_t buf_size);
};

} // end namespace data
} // end namespace ross_damaris
#endif // end TABLE_BUILDER_H
