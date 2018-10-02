#ifndef SAMPLE_INDEX_H
#define SAMPLE_INDEX_H

#include <boost/shared_ptr.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/tuple/tuple.hpp>

#include <plugins/data/DataSample.h>

namespace ross_damaris {
namespace data{

namespace bmi = boost::multi_index;

/* tags for use with boost multi index containers */
struct by_mode {};
struct by_time {};
struct by_status {};
struct by_sample_key {};
/* end tags */

typedef boost::multi_index_container<
        boost::shared_ptr<ross_damaris::data::DataSample>,
        bmi::indexed_by<
                bmi::ordered_non_unique<bmi::tag<by_mode>,
                    bmi::const_mem_fun<DataSample,int,&DataSample::get_mode> >,
                bmi::ordered_non_unique<bmi::tag<by_time>,
                    bmi::const_mem_fun<DataSample,double,&DataSample::get_sampling_time> >,
                bmi::ordered_non_unique<bmi::tag<by_status>,
                    bmi::const_mem_fun<DataSample,int,&DataSample::get_status> >,
                bmi::ordered_unique<bmi::tag<by_sample_key>,
                    bmi::composite_key<DataSample,
                        bmi::const_mem_fun<DataSample,int,&DataSample::get_mode>,
                        bmi::const_mem_fun<DataSample,double,&DataSample::get_sampling_time>
                    >
                >
        >
    > SampleIndex;

typedef SampleIndex::index<by_mode>::type SamplesByMode;
typedef SampleIndex::index<by_time>::type SamplesByTime;
typedef SampleIndex::index<by_status>::type SamplesByStatus;
typedef SampleIndex::index<by_sample_key>::type SamplesByKey;

} // end namespace data
} // end namespace ross_damaris

#endif  // SAMPLE_INDEX_H
