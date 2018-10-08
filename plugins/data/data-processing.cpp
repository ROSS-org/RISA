#include "damaris/data/VariableManager.hpp"
#include "damaris/data/Variable.hpp"
#include <iostream>
#include <fstream>
#include <boost/asio.hpp>

#include <plugins/util/fb-util.h>
#include <plugins/util/damaris-util.h>
#include <plugins/util/sim-config.h>
#include <plugins/data/DataProcessor.h>

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

void DataProcessor::forward_data(InstMode mode, double ts)
{
    SamplesByKey::iterator it, end;
    //auto it = data_manager_->find_data(mode, ts);
    data_manager_->find_data(mode, last_processed_ts_, ts, it, end);
    while (it != end)
    {
        DamarisDataSampleT combined_sample;
        DamarisDataSampleT ds;
        bool data_found = false;
        for (int i = 0; i < (*it)->get_ds_ptr_size(); i++)
        {
            auto dataspace = (*it)->get_data_at(i);
            auto data_fb = GetDamarisDataSample(dataspace.GetData());
            if (i == 0)
            {
                data_fb->UnPackTo(&combined_sample);
                continue;
            }
            data_fb->UnPackTo(&ds);
            std::move(ds.pe_data.begin(), ds.pe_data.end(), std::back_inserter(combined_sample.pe_data));
            std::move(ds.kp_data.begin(), ds.kp_data.end(), std::back_inserter(combined_sample.kp_data));
            std::move(ds.lp_data.begin(), ds.lp_data.end(), std::back_inserter(combined_sample.lp_data));
            std::move(ds.model_data.begin(), ds.model_data.end(), std::back_inserter(combined_sample.model_data));
        }

        stream_client_->enqueue_data(&combined_sample);
        it++;
    }
    last_processed_ts_ = ts;
}

void DataProcessor::delete_data(double ts)
{

}

// pass in rvalue so we don't create an extra shared_ptr,
// that will just be deleted at end of function
void DataProcessor::set_manager_ptr(boost::shared_ptr<DataManager>&& ptr)
{
    data_manager_ = boost::shared_ptr<DataManager>(ptr);
}

void DataProcessor::set_stream_ptr(boost::shared_ptr<streaming::StreamClient>&& ptr)
{
    stream_client_ = boost::shared_ptr<streaming::StreamClient>(ptr);
}
