#include <plugins/data/analysis-tasks.h>
#include <plugins/data/DataSample.h>
#include <plugins/flatbuffers/data_sample_generated.h>
#include <plugins/util/damaris-util.h>

#include <damaris/buffer/DataSpace.hpp>
#include <damaris/buffer/Buffer.hpp>
#include <damaris/data/Variable.hpp>

using namespace ross_damaris::util;
using namespace ross_damaris::sample;
using namespace ross_damaris::data;

void aggregate_data(void *arg)
{

}

void initial_data_processing(void *arguments)
{
    // first get my ES and pool pointers
    ABT_xstream xstream;
    ABT_pool pool;
    ABT_xstream_self(&xstream);
    ABT_xstream_get_main_pools(xstream, 1, &pool);
    int pool_id;
    ABT_pool_get_id(pool, &pool_id);

    initial_task_args *args;
    args = (initial_task_args*)arguments;
    int step = args->step;
    printf("initial_data_processing() called at step %d, pool_id %d\n", step, pool_id);
    damaris::BlocksByIteration::iterator begin, end;
    DUtil::get_damaris_iterators("ross/sample", step, begin, end);

    damaris::DataSpace<damaris::Buffer> ds;
    while (begin != end)
    {
        initial_task_args * task_args = (initial_task_args*)malloc(sizeof(initial_task_args));
        task_args->step = step;
        task_args->dm = args->dm;
        cout << "ds.RefCount() = " << (*begin)->GetDataSpace().RefCount() << endl;
        task_args->ds = reinterpret_cast<const void*>(&((*begin)->GetDataSpace()));

        //cout << "src: " << (*begin)->GetSource() << " iteration: " << (*begin)->GetIteration()
        //    << " domain id: " << (*begin)->GetID() << endl;
        ABT_task_create(pool, insert_data_mic, task_args, NULL);
        begin++;
    }

    free(args);
}

void insert_data_mic(void *arguments)
{
    initial_task_args *args;
    args = (initial_task_args*)arguments;
    int step = args->step;
    printf("insert_data_mic() called at step %d\n", step);
    boost::shared_ptr<DataManager> dm = *reinterpret_cast<boost::shared_ptr<DataManager>*>(args->dm);
    cout << "dm.use_count() = " << dm.use_count() << endl;
    const damaris::DataSpace<damaris::Buffer> ds = *reinterpret_cast<const damaris::DataSpace<damaris::Buffer>*>(args->ds);
    cout << "ds.RefCount() = " << ds.RefCount() << endl;
    auto data_fb = GetDamarisDataSample(ds.GetData());
    double ts;
    switch (data_fb->mode())
    {
        case InstMode_GVT:
            ts = data_fb->last_gvt();
            break;
        case InstMode_VT:
            ts = data_fb->virtual_ts();
            break;
        case InstMode_RT:
            ts = data_fb->real_ts();
            break;
        default:
            break;
    }

    int id = data_fb->entity_id();

    // first we need to check to see if this is data for an existing sampling point
    auto sample_it = dm->find_data(data_fb->mode(), ts);
    if (sample_it != dm->end())
    {
        (*sample_it)->push_ds_ptr(id, data_fb->event_id(), ds);
        //cout << "num ds_ptrs " << (*sample_it)->get_ds_ptr_size() << endl;
    }
    else // couldn't find it
    {
        DataSample s(ts, data_fb->mode(), DataStatus_speculative);
        s.push_ds_ptr(id, data_fb->event_id(), ds);
        dm->insert_data(std::move(s));
        //data_manager_->print_manager_info();
    }

    //cur_mode_ = data_fb->mode();
    //cur_ts_ = ts;

    free(args);
}

void remove_data_mic(void *arg)
{

}
