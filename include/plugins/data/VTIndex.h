#ifndef VT_INDEX_H
#define VT_INDEX_H

#include <memory>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/tuple/tuple.hpp>

#include <plugins/data/VTSample.h>

namespace ross_damaris {
namespace data {

namespace bmi = boost::multi_index;

// tags
struct by_kp_gid{};
struct by_vts{};
struct by_rts{};
struct by_kp_vt{};
struct by_kp_vt_rt{};

typedef boost::multi_index_container<
        std::shared_ptr<ross_damaris::data::VTSample>,
        bmi::indexed_by<
                bmi::ordered_non_unique<bmi::tag<by_kp_gid>,
                    bmi::const_mem_fun<VTSample,int,&VTSample::get_kp_gid> >,
                bmi::ordered_non_unique<bmi::tag<by_vts>,
                    bmi::const_mem_fun<VTSample,double,&VTSample::get_vts> >,
                bmi::ordered_non_unique<bmi::tag<by_rts>,
                    bmi::const_mem_fun<VTSample,double,&VTSample::get_rts> >,
                bmi::ordered_non_unique<bmi::tag<by_kp_vt>,
                    bmi::composite_key<VTSample,
                        bmi::const_mem_fun<VTSample,int,&VTSample::get_kp_gid>,
                        bmi::const_mem_fun<VTSample,double,&VTSample::get_vts>
                    >
                >,
                bmi::ordered_unique<bmi::tag<by_kp_vt_rt>,
                    bmi::composite_key<VTSample,
                        bmi::const_mem_fun<VTSample,int,&VTSample::get_kp_gid>,
                        bmi::const_mem_fun<VTSample,double,&VTSample::get_vts>,
                        bmi::const_mem_fun<VTSample,double,&VTSample::get_rts>
                    >
                >
        >
    > VTIndex;

typedef VTIndex::index<by_kp_gid>::type VTSByKP;
typedef VTIndex::index<by_vts>::type VTSByVT;
typedef VTIndex::index<by_rts>::type VTSByRT;
typedef VTIndex::index<by_kp_vt>::type VTSByKPVT;
typedef VTIndex::index<by_kp_vt_rt>::type VTSByKPVTRT;

} // end namespace data
} // end namespace ross_damaris

#endif // VT_INDEX_H