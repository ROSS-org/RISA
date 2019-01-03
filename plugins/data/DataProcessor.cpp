/**
 * @file DataProcessor.cpp
 *
 * The DataProcessor handles the bulk of the data processing.
 * This includes data reduction, as well as garbage collection
 * for the multi-index.
 * This functionality will likely run on its own thread
 * that gets started when initializing the Damaris rank.
 */

#include <iostream>
#include <fstream>
#include <boost/asio.hpp>
#include "damaris/data/VariableManager.hpp"
#include "damaris/data/Variable.hpp"

#include <plugins/data/DataProcessor.h>
#include <plugins/util/damaris-util.h>

using namespace ross_damaris;
using namespace ross_damaris::sample;
using namespace ross_damaris::streaming;
using namespace ross_damaris::util;
using namespace ross_damaris::data;
using namespace std;

// For now, we're only supporting model data in VT mode
void DataProcessor::forward_data(InstMode mode, double ts, int32_t step)
{
    // TODO figure out why I removed this. it seems to be correct without it
    // (and incorrect with it), but not sure why...
    // It may be that when we have multiple samples for same KP and same time,
    // we overwrite them? Or perhaps the earlier samples are kept and latter (and thus
    // correct) samples are discarded.
    //if (mode == InstMode_VT)
    //    invalidate_data(ts, step);

    SamplesByKey::iterator it, end;
    //auto it = data_manager_->find_data(mode, ts);
    data_manager_->find_data(mode, last_processed_ts_, ts, it, end);
    while (it != end)
    {
        DamarisDataSampleT combined_sample;
        DamarisDataSampleT ds;
        auto outer_it = (*it)->ds_ptrs_begin();
        auto outer_end = (*it)->ds_ptrs_end();
        auto ds_it = (*outer_it).second.begin();
        auto ds_end = (*outer_it).second.end();
        auto dataspace = (*ds_it).second;
        auto data_fb = GetDamarisDataSample(dataspace.GetData());
        data_fb->UnPackTo(&combined_sample);
        ds_it++;
        while (outer_it != outer_end)
        {
            while (ds_it != ds_end)
            {
                dataspace = (*ds_it).second;
                data_fb = GetDamarisDataSample(dataspace.GetData());
                data_fb->UnPackTo(&ds);
                std::move(ds.pe_data.begin(), ds.pe_data.end(),
                        std::back_inserter(combined_sample.pe_data));
                std::move(ds.kp_data.begin(), ds.kp_data.end(),
                        std::back_inserter(combined_sample.kp_data));
                std::move(ds.lp_data.begin(), ds.lp_data.end(),
                        std::back_inserter(combined_sample.lp_data));
                std::move(ds.model_data.begin(), ds.model_data.end(),
                        std::back_inserter(combined_sample.model_data));
                ds_it++;
            }
            outer_it++;
            if (outer_it != outer_end)
            {
                ds_it = (*outer_it).second.begin();
                ds_end = (*outer_it).second.end();
            }
        }

        stream_client_->enqueue_data(&combined_sample);
        it++;
    }
    last_processed_ts_ = ts;
}

// only for VTS
// TODO if we don't actually need this data for invalidation, get rid of this
// TODO also perhaps this should be moved to the RDServer? Or the DataManager,
// and RD server gives it the info necessary to delete
void DataProcessor::invalidate_data(double ts, int32_t step)
{
    // first lets invalidate the data
    int num_blocks = DUtil::get_num_blocks("ross/vt_rb/ts", step);
    if (num_blocks > 0)
    {
        // some of our model data has been invalidated
        // get data for kp_gid, event_id
        damaris::BlocksByIteration::iterator begin, end;
        DUtil::get_damaris_iterators("ross/vt_rb/ts", step, begin, end);
        while (begin != end)
        {
            double ts = *(static_cast<double*>((*begin)->GetDataSpace().GetData()));
            int kpid = *(static_cast<int*>(DUtil::get_value_from_damaris("ross/vt_rb/kp_gid",
                        (*begin)->GetSource(), (*begin)->GetIteration(), (*begin)->GetID())));
            int event_id = *(static_cast<int*>(DUtil::get_value_from_damaris("ross/vt_rb/event_id",
                        (*begin)->GetSource(), (*begin)->GetIteration(), (*begin)->GetID())));

            auto sample_it = data_manager_->find_data(InstMode_VT, ts);
            if (sample_it != data_manager_->end())
            {
                //auto dataspace = (*sample_it)->get_data_at(kpid);
                bool removed = (*sample_it)->remove_ds_ptr(kpid, event_id);
                if (!removed)
                    cout << "[DataProcessor] nothing was removed!\n";
            }

            begin++;
        }
    }


}

void DataProcessor::set_manager_ptr(boost::shared_ptr<DataManager>&& ptr)
{
    data_manager_ = boost::shared_ptr<DataManager>(ptr);
}

void DataProcessor::set_stream_ptr(boost::shared_ptr<streaming::StreamClient>&& ptr)
{
    stream_client_ = boost::shared_ptr<streaming::StreamClient>(ptr);
}

