#include <plugins/data/DataSample.h>

using namespace ross_damaris::data;


void DataSample::push_ds_ptr(int id, const damaris::DataSpace<damaris::Buffer> ds)
{
    ds_ptrs_[id] = ds;
}
