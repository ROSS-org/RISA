#ifndef EVENT_INDEX_H
#define EVENT_INDEX_H

#include <boost/shared_ptr.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/tuple/tuple.hpp>

#include "Event.h"

namespace opt_debug {

namespace bmi = boost::multi_index;

/* tags for use with boost multi index containers */
struct by_src_lp {};
struct by_send_ts {};
struct by_recv_ts {};
struct by_full_event {};
struct by_damaris_it {};
struct by_sync_type {};
/* end tags */

typedef boost::multi_index_container<
        boost::shared_ptr<Event>,
        bmi::indexed_by<
                bmi::ordered_non_unique<bmi::tag<by_src_lp>,
                    bmi::const_mem_fun<Event,int,&Event::get_src_lp> >,
                bmi::ordered_non_unique<bmi::tag<by_send_ts>,
                    bmi::const_mem_fun<Event,float,&Event::get_send_ts> >,
                bmi::ordered_non_unique<bmi::tag<by_recv_ts>,
                    bmi::const_mem_fun<Event,float,&Event::get_recv_ts> >,
                bmi::ordered_non_unique<bmi::tag<by_damaris_it>,
                    bmi::const_mem_fun<Event,int,&Event::get_damaris_iteration> >,
                bmi::ordered_non_unique<bmi::tag<by_sync_type>,
                    bmi::const_mem_fun<Event,int,&Event::get_sync_type> >,
                bmi::ordered_non_unique<bmi::tag<by_full_event>,
                    bmi::composite_key<Event,
                        bmi::const_mem_fun<Event,int,&Event::get_src_lp>,
                        bmi::const_mem_fun<Event,int,&Event::get_dest_lp>,
                        bmi::const_mem_fun<Event,float,&Event::get_send_ts>,
                        bmi::const_mem_fun<Event,float,&Event::get_recv_ts>
                    >
                >
        >
    > EventIndex;

typedef EventIndex::index<by_src_lp>::type EventBySrc;
typedef EventIndex::index<by_send_ts>::type EventBySend;
typedef EventIndex::index<by_recv_ts>::type EventByRecv;
typedef EventIndex::index<by_damaris_it>::type EventByIteration;
typedef EventIndex::index<by_sync_type>::type EventBySync;
typedef EventIndex::index<by_full_event>::type Events;

}


#endif  // EVENT_INDEX_H
