#include "damaris/data/VariableManager.hpp"
#include "damaris/data/Variable.hpp"
#include <iostream>
#include <fstream>
#include <boost/asio.hpp>

#include <plugins/streaming/stream-client.h>
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

void DataProcessor::forward_data(InstMode mode, double ts, streaming::StreamClient* client)
{
    auto it = data_manager_->find_data(mode, ts);
    if (it != data_manager_->end())
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

        flatbuffers::FlatBufferBuilder *fbb = new flatbuffers::FlatBufferBuilder();
        auto new_samp = DamarisDataSample::Pack(*fbb, &combined_sample);
        fbb->Finish(new_samp);
        client->write(fbb);
        cout << "writing to stream client\n";
        delete fbb;
    }
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
