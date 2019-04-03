#ifndef MODEL_MD_INDEX_H
#define MODEL_MD_INDEX_H

#include <memory>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/tuple/tuple.hpp>

#include <plugins/util/ModelMetadata.h>

namespace ross_damaris {
namespace util {

namespace bmi = boost::multi_index;

// tags
struct by_pe{};
struct by_kp{};
struct by_lp{};
struct by_name{};
struct by_full_id{};

typedef boost::multi_index_container<
        std::shared_ptr<ross_damaris::util::ModelLPMetadata>,
        bmi::indexed_by<
                bmi::ordered_non_unique<bmi::tag<by_pe>,
                    bmi::const_mem_fun<ModelLPMetadata,int,&ModelLPMetadata::get_peid> >,
                bmi::ordered_non_unique<bmi::tag<by_kp>,
                    bmi::const_mem_fun<ModelLPMetadata,int,&ModelLPMetadata::get_kpid> >,
                bmi::ordered_unique<bmi::tag<by_lp>,
                    bmi::const_mem_fun<ModelLPMetadata,int,&ModelLPMetadata::get_lpid> >,
                bmi::ordered_non_unique<bmi::tag<by_name>,
                    bmi::const_mem_fun<ModelLPMetadata,std::string,&ModelLPMetadata::get_name> >,
                bmi::ordered_unique<bmi::tag<by_full_id>,
                    bmi::composite_key<ModelLPMetadata,
                        bmi::const_mem_fun<ModelLPMetadata,int,&ModelLPMetadata::get_peid>,
                        bmi::const_mem_fun<ModelLPMetadata,int,&ModelLPMetadata::get_kpid>,
                        bmi::const_mem_fun<ModelLPMetadata,int,&ModelLPMetadata::get_lpid>
                    >
                >
        >
    > ModelMDIndex;

typedef ModelMDIndex::index<by_pe>::type ModelMDByPE;
typedef ModelMDIndex::index<by_kp>::type ModelMDByKP;
typedef ModelMDIndex::index<by_lp>::type ModelMDByLP;
typedef ModelMDIndex::index<by_name>::type ModelMDByName;
typedef ModelMDIndex::index<by_full_id>::type ModelMDByFullID;

} // end namespace util
} // end namespace ross_damaris

#endif // MODEL_MD_INDEX_H
