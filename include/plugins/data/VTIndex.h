#ifndef VT_INDEX_H
#define VT_INDEX_H

#include <memory>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/ranked_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/tuple/tuple.hpp>

#include <plugins/data/VTSample.h>

namespace ross_damaris {
namespace data {

namespace bmi = boost::multi_index;

// tags
struct by_peid{};
struct by_kp_gid{};
struct by_vts{};
struct by_rts{};
struct by_gvt{};
struct by_pe_gvt{};
struct by_pe_rt{};
struct by_kp_vt{};
struct by_kp_vt_rt{};

typedef boost::multi_index_container<
        std::shared_ptr<ross_damaris::data::TSSample>,
        bmi::indexed_by<
                bmi::ordered_non_unique<bmi::tag<by_peid>,
                    bmi::const_mem_fun<TSSample,int,&TSSample::get_peid> >,
                bmi::ordered_non_unique<bmi::tag<by_kp_gid>,
                    bmi::const_mem_fun<TSSample,int,&TSSample::get_kp_gid> >,
                bmi::ordered_non_unique<bmi::tag<by_vts>,
                    bmi::const_mem_fun<TSSample,double,&TSSample::get_vts> >,
                bmi::ordered_non_unique<bmi::tag<by_rts>,
                    bmi::const_mem_fun<TSSample,double,&TSSample::get_rts> >,
                bmi::ranked_non_unique<bmi::tag<by_gvt>,
                    bmi::const_mem_fun<TSSample,double,&TSSample::get_gvt> >,
                bmi::ordered_non_unique<bmi::tag<by_kp_vt>,
                    bmi::composite_key<TSSample,
                        bmi::const_mem_fun<TSSample,int,&TSSample::get_kp_gid>,
                        bmi::const_mem_fun<TSSample,double,&TSSample::get_vts>
                    >
                >,
                bmi::ordered_non_unique<bmi::tag<by_pe_gvt>,
                    bmi::composite_key<TSSample,
                        bmi::const_mem_fun<TSSample,int,&TSSample::get_peid>,
                        bmi::const_mem_fun<TSSample,double,&TSSample::get_gvt>
                    >
                >,
                bmi::ordered_non_unique<bmi::tag<by_pe_rt>,
                    bmi::composite_key<TSSample,
                        bmi::const_mem_fun<TSSample,int,&TSSample::get_peid>,
                        bmi::const_mem_fun<TSSample,double,&TSSample::get_rts>
                    >
                >,
                bmi::ordered_non_unique<bmi::tag<by_kp_vt_rt>,
                    bmi::composite_key<TSSample,
                        bmi::const_mem_fun<TSSample,int,&TSSample::get_kp_gid>,
                        bmi::const_mem_fun<TSSample,double,&TSSample::get_vts>,
                        bmi::const_mem_fun<TSSample,double,&TSSample::get_rts>
                    >
                >
        >
    > TSIndex;

typedef TSIndex::index<by_peid>::type SampleByPE;
typedef TSIndex::index<by_kp_gid>::type SampleByKP;
typedef TSIndex::index<by_vts>::type SampleByVT;
typedef TSIndex::index<by_rts>::type SampleByRT;
typedef TSIndex::index<by_gvt>::type SampleByGVT;
typedef TSIndex::index<by_kp_vt>::type SampleByKPVT;
typedef TSIndex::index<by_pe_rt>::type SampleByPERT;
typedef TSIndex::index<by_pe_gvt>::type SampleByPEGVT;
typedef TSIndex::index<by_kp_vt_rt>::type SampleByKPVTRT;

} // end namespace data
} // end namespace ross_damaris

#endif // VT_INDEX_H
