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

bool DataManager::insert_data(DataSample&& s)
{
    boost::shared_ptr<DataSample> s_ptr = boost::make_shared<DataSample>(std::move(s));
    auto mypair = data_index_.insert(std::move(s_ptr));
    return mypair.second;
}

SamplesByKey::iterator DataManager::find_data(InstMode mode, double ts)
{
    return data_index_.get<by_sample_key>().find(
            boost::make_tuple(mode, ts));
}

// returns samples exclusive of lower and inclusive of upper
void DataManager::find_data(InstMode mode, double lower, double upper,
        SamplesByKey::iterator& begin, SamplesByKey::iterator& end)
{
    if (lower == upper)
    {
        begin = find_data(mode, lower);
        end = data_index_.get<by_sample_key>().end();
    }
    else
    {
        // don't want to include lower bound
        begin = data_index_.get<by_sample_key>().upper_bound(
                boost::make_tuple(mode, lower));
        end = data_index_.get<by_sample_key>().upper_bound(
                boost::make_tuple(mode, upper));
    }
}

void DataManager::delete_data(double ts, sample::InstMode mode)
{
    SamplesByKey::iterator begin, end;
    find_data(mode, 0, ts, begin, end);
    data_index_.get<by_sample_key>().erase(begin, end);
}

void DataManager::print_manager_info()
{
    cout << "[DataManager] index size " << data_index_.size() << endl;
}

void DataManager::clear()
{
    data_index_.clear();
}
