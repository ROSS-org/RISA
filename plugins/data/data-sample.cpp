#include <plugins/data/DataSample.h>

using namespace ross_damaris::data;

//void DataSample::push_data_ptr(const sample::DamarisDataSample& s)
//{
//    data_ptrs_.push_back(s);
//}

void DataSample::push_ds_ptr(const damaris::DataSpace<damaris::Buffer> ds)
{
    ds_ptrs_.push_back(ds);
}
