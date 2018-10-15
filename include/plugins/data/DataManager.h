#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include <plugins/data/SampleIndex.h>
#include <plugins/flatbuffers/data_sample_generated.h>

// TODO this class may be eventually templatized, so keep in mind

namespace ross_damaris {
namespace data {

/**
 * @brief Manages data in a Boost MultiIndex container
 */
class DataManager {
public:
    // require move insertions over copy insertions?
    /**
     * @brief Inserts data to the multi-index
     * @param s A DataSample rvalue
     * @return A boolean value. true if s was inserted, false otherwise.
     */
    bool insert_data(DataSample&& s);

    /**
     * @brief Find a DataSample, given the mode and timestamp.
     * @param mode The Instrumentation mode.
     * @param ts The timestamp of the sample.
     * @return An iterator to the associated sample, if found.
     */
    SamplesByKey::iterator find_data(sample::InstMode mode, double ts);

    /**
     * @brief Find DataSamples and get begin and end iterators.
     * @param mode The instrumentation mode.
     * @param lower The lower bound timestamp (exclusive) of samples.
     * @param upper The upper bound timestamp (inclusive) of samples.
     * @param begin The beginning iterator is stored here.
     * @param end The end of the iterators for the found samples.
     */
    void find_data(sample::InstMode mode, double lower, double upper,
            SamplesByKey::iterator& begin, SamplesByKey::iterator& end);
    //void delete_data();

    SamplesByKey::iterator end() { return data_index_.get<by_sample_key>().end(); }

    const SamplesByMode::iterator& get_recent_mode_iterator();
    const SamplesByTime::iterator& get_recent_time_iterator();
    const SamplesByKey::iterator& get_recent_key_iterator();

    void print_manager_info();
    void clear();

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
