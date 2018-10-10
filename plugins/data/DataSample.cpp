#include <plugins/data/DataSample.h>

using namespace ross_damaris::data;


void DataSample::push_ds_ptr(int id, const damaris::DataSpace<damaris::Buffer> ds)
{
    push_ds_ptr(id, -1, ds);
}

void DataSample::push_ds_ptr(int id, int event_id, const damaris::DataSpace<damaris::Buffer> ds)
{
    std::cout << "push_ds_ptr id " << id << " event_id " << event_id << "\n";
    std::unordered_map<int, damaris::DataSpace<damaris::Buffer>>& entity_map = ds_ptrs_[id];
    entity_map[event_id] = ds;
}

damaris::DataSpace<damaris::Buffer> DataSample::get_dataspace(int id)
{
    return get_dataspace(id, -1);
}

damaris::DataSpace<damaris::Buffer> DataSample::get_dataspace(int id, int event_id)
{
    auto entity_map = ds_ptrs_[id];
    return entity_map[event_id];
}

bool DataSample::remove_ds_ptr(int id)
{
    return remove_ds_ptr(id, -1);
}

bool DataSample::remove_ds_ptr(int id, int event_id)
{
    auto entity_map = ds_ptrs_[id];
    auto rv = entity_map.erase(event_id);
    if (entity_map.empty())
        ds_ptrs_.erase(id);
    return rv == 1 ? true : false;
}

std::unordered_map<int, std::unordered_map<int,
    damaris::DataSpace<damaris::Buffer>>>::iterator DataSample::ds_ptrs_begin()
{
    return ds_ptrs_.begin();
}

std::unordered_map<int, std::unordered_map<int,
    damaris::DataSpace<damaris::Buffer>>>::iterator DataSample::ds_ptrs_end()
{
    return ds_ptrs_.end();
}

//std::unordered_map<int, damaris::DataSpace<damaris::Buffer>>::iterator
//    DataSample::start_iteration()
//{
//    outer_it_ = ds_ptrs_.begin();
//    return (*outer_it_).begin();
//}
//
//std::unordered_map<int, damaris::DataSpace<damaris::Buffer>>::iterator
//    DataSample::get_next_iterator()
//{
//    if (outer_it_ != ds_ptrs_.end())
//        outer_it_++;
//    return (*outer_it_).begin();
//}
//
//std::unordered_map<int, damaris::DataSpace<damaris::Buffer>>::iterator
//    DataSample::end_iterator()
//{
//    return (*outer_it_).end();
//}
