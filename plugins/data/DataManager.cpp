#include <plugins/data/DataManager.h>
#include <utility>
#include <boost/smart_ptr/make_shared.hpp>

using namespace ross_damaris::data;
using namespace ross_damaris::sample;
using namespace std;

const SamplesByMode::iterator& DataManager::get_recent_mode_iterator()
{
    return recent_mode_it_;
}

const SamplesByTime::iterator& DataManager::get_recent_time_iterator()
{
    return recent_time_it_;
}

const SamplesByKey::iterator& DataManager::get_recent_key_iterator()
{
    return recent_key_it_;
}

//template <typename iterator>
//std::pair<iterator, bool> DataManager::insert_data(DataSample&& s)
void DataManager::insert_data(DataSample&& s)
{
    boost::shared_ptr<DataSample> s_ptr = boost::make_shared<DataSample>(std::move(s));
    auto mypair = data_index_.insert(std::move(s_ptr));
    cout << "insert return: " << mypair.second << endl;
}

SamplesByKey::iterator DataManager::find_data(InstMode mode, double ts)
{
    return data_index_.get<by_sample_key>().find(
            boost::make_tuple(mode, ts));
}

void DataManager::print_manager_info()
{
    cout << "[DataManager] index size " << data_index_.size() << endl;
}
