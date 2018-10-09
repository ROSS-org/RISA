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

void DataProcessor::aggregate_data()
{

}

void DataProcessor::forward_model_data()
{

}

// For now, we're only supporting model data in VT mode
void DataProcessor::forward_data(InstMode mode, double ts)
{
    SamplesByKey::iterator it, end;
    //auto it = data_manager_->find_data(mode, ts);
    data_manager_->find_data(mode, last_processed_ts_, ts, it, end);
    while (it != end)
    {
        DamarisDataSampleT combined_sample;
        DamarisDataSampleT ds;
        auto ds_it = (*it)->ds_ptrs_begin();
        auto ds_end = (*it)->ds_ptrs_end();
        auto dataspace = (*ds_it).second;
        auto data_fb = GetDamarisDataSample(dataspace.GetData());
        data_fb->UnPackTo(&combined_sample);
        ds_it++;
        //for (int i = 0; i < (*it)->get_ds_ptr_size(); i++)
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

        stream_client_->enqueue_data(&combined_sample);
        it++;
    }
    last_processed_ts_ = ts;
}

void DataProcessor::delete_data(double ts)
{

}

void DataProcessor::set_manager_ptr(boost::shared_ptr<DataManager>&& ptr)
{
    data_manager_ = boost::shared_ptr<DataManager>(ptr);
}

void DataProcessor::set_stream_ptr(boost::shared_ptr<streaming::StreamClient>&& ptr)
{
    stream_client_ = boost::shared_ptr<streaming::StreamClient>(ptr);
}
