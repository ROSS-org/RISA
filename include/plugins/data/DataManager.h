#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include <plugins/data/SampleIndex.h>
#include <plugins/flatbuffers/data_sample_generated.h>

// TODO this class may be eventually templatized, so keep in mind

namespace ross_damaris {
namespace data {

class DataManager {
public:
    //DataManager() {  }

    // require move insertions over copy insertions?
    void insert_data(DataSample&& s);
    SamplesByKey::iterator find_data(sample::InstMode mode, double ts);
    void delete_data();

    SamplesByKey::iterator end() { return data_index_.get<by_sample_key>().end(); }

    const SamplesByMode::iterator& get_recent_mode_iterator();
    const SamplesByTime::iterator& get_recent_time_iterator();
    const SamplesByKey::iterator& get_recent_key_iterator();

    void print_manager_info();

private:
    SampleIndex data_index_;

    // these variables save the most recent iterators
    // as an effort to help speed up some lookup operations
    SamplesByMode::iterator recent_mode_it_;
    SamplesByTime::iterator recent_time_it_;
    SamplesByKey::iterator recent_key_it_;
};

} // end namespace data
} // end namespace ross_damaris

#endif // DATA_MANAGER_H
