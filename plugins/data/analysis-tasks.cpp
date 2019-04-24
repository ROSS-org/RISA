#include <unordered_map>
#include <unordered_set>
#include <plugins/data/analysis-tasks.h>
#include <plugins/flatbuffers/data_sample_generated.h>
#include <plugins/util/damaris-util.h>
#include <plugins/util/SimConfig.h>
#include <plugins/streaming/StreamClient.h>
#include <plugins/data/SampleFBBuilder.h>
#include <plugins/data/VTIndex.h>
#include <plugins/data/TableBuilder.h>
#include <plugins/data/FeatureExtractor.h>

#include <damaris/buffer/DataSpace.hpp>
#include <damaris/buffer/Buffer.hpp>
#include <damaris/data/VariableManager.hpp>
#include <damaris/data/Variable.hpp>

#include <instrumentation/st-instrumentation-internal.h>
//#include <instrumentation/st-model-data.h>

using namespace ross_damaris;
using namespace ross_damaris::util;
using namespace ross_damaris::sample;
using namespace ross_damaris::data;
using namespace std;
namespace fb = flatbuffers;

config::SimConfig* sim_config = nullptr;
streaming::StreamClient* stream_client = nullptr;
VTIndex vtsamples;

const char * const inst_buffer_names[] = {
    "",
    "ross/inst_sample/gvt_inst",
    "ross/inst_sample/vts_inst",
    "ross/inst_sample/rts_inst"
};

static TableBuilder* table_builder;

map<double, SampleFBBuilder> sample_processing_gvt(int mode, int step);
map<double, SampleFBBuilder> sample_processing_rts(int mode, int step);
void sample_processing_vts(int mode, int step);

void init_analysis_tasks()
{
    sim_config = config::SimConfig::get_instance();
    stream_client = streaming::StreamClient::get_instance();
    table_builder = new TableBuilder();
}

// TODO add in size checking for errors
void create_flatbuffer(InstMode mode, char* buffer, size_t buf_size, sample_metadata *sample_md,
        SampleFBBuilder& sample_fbb)
{
    // TODO are we supposed to use mode here?
    (void)mode;
    if (sample_md->has_pe)
    {
        st_pe_stats *pe_stats = reinterpret_cast<st_pe_stats*>(buffer);
        buffer += sizeof(*pe_stats);
        buf_size -= sizeof(*pe_stats);
        sample_fbb.add_pe(pe_stats);
    }

    if (sample_md->has_kp)
    {
        for (int i = 0; i < sample_md->has_kp; i++)
        {
            st_kp_stats *kp_stats = reinterpret_cast<st_kp_stats*>(buffer);
            buffer += sizeof(*kp_stats);
            buf_size -= sizeof(*kp_stats);
            sample_fbb.add_kp(kp_stats, sample_md->peid);
        }
    }

    if (sample_md->has_lp)
    {
        for (int i = 0; i < sample_md->has_lp; i++)
        {
            st_lp_stats *lp_stats = reinterpret_cast<st_lp_stats*>(buffer);
            buffer += sizeof(*lp_stats);
            buf_size -= sizeof(*lp_stats);
            sample_fbb.add_lp(lp_stats, sample_md->peid);
        }
    }

    if (sample_md->has_model)
    {
        // TODO if VTS, save model data separately from sim engine data
        //cout << "analysis-tasks has_model\n";
        //cout << "peid: " << sample_md->peid << " vts: " << sample_md->vts << " rts: " << sample_md->rts
        //    << " last_gvt: " << sample_md->vts << endl;
        for (int i = 0; i < sample_md->num_model_lps; i++)
        {
            st_model_data *model_lp = reinterpret_cast<st_model_data*>(buffer);
            buffer += sizeof(*model_lp);
            buf_size -= sizeof(*model_lp);
            buffer = sample_fbb.add_model_lp(model_lp, buffer, buf_size, sample_md->peid);
        }

    }

    if (buf_size != 0)
        cout << "create_flatbuffer buf_size = " << buf_size << endl;

}

void initial_data_processing(void *arguments)
{
    int my_rank;
    ABT_xstream_self_rank(&my_rank);
    //cout << "initial_data_processing my_rank = " << my_rank << endl;
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
    //cout << "initial data processing: step " << step << endl;
    //printf("initial_data_processing() called at step %d, pool_id %d\n", step, pool_id);
    //TODO could just move all this to the argobots manager instead of creating a task to create
    //potentially 3 new tasks
    const InstMode *modes = EnumValuesInstMode();
    for (int mode = InstMode_GVT; mode <= InstMode_RT; mode++)
    {
        if (!sim_config->inst_mode_sim(modes[mode]) && !sim_config->inst_mode_model(modes[mode]))
            continue;

        initial_task_args *samp_args = (initial_task_args*)malloc(sizeof(initial_task_args));
        samp_args->step = step;
        samp_args->mode = mode;
        ABT_task_create(pool, sample_processing_task, samp_args, NULL);
        //cout << "created abt sample_processing task for mode " << mode << " step " << step << endl;
    }

    free(args);
}

void sample_processing_task(void *arguments)
{
    int my_rank;
    ABT_xstream_self_rank(&my_rank);
    //cout << "sample_processing my_rank = " << my_rank << endl;
    initial_task_args *args;
    args = (initial_task_args*)arguments;
    auto v1 = damaris::VariableManager::Search(inst_buffer_names[args->mode]);
    //cout << inst_buffer_names[args->mode];
    //cout << " number of blocks " << v1->CountLocalBlocks(args->step) << endl;

    map<double, SampleFBBuilder> fb_map;
    switch (args->mode)
    {
        case InstMode_GVT:
            fb_map = sample_processing_gvt(args->mode, args->step);
            break;
        case InstMode_VT:
            sample_processing_vts(args->mode, args->step);
            break;
        case InstMode_RT:
            fb_map = sample_processing_rts(args->mode, args->step);
            break;
        default:
            //cout << "sample_processing: wrong mode!\n";
            break;
    }

    //cout << "fb_map size = " << fb_map.size() << endl;
    for (auto it = fb_map.begin(); it != fb_map.end(); ++it)
    {
        SampleFBBuilder& sample_fbb = it->second;
        sample_fbb.finish();
        size_t size, offset;
        uint8_t* raw = sample_fbb.release_raw(size, offset);
        stream_client->enqueue_data(raw, &raw[offset], size);
    }


    // GC this variable for this time step
    std::shared_ptr<damaris::Variable> v = damaris::VariableManager::Search(inst_buffer_names[args->mode]);
    v->ClearIteration(args->step);

    free(args);
}

//TODO return a map of flatbuffers (one for each sampling point)
map<double, SampleFBBuilder> sample_processing_gvt(int mode, int step)
{
    int my_rank;
    ABT_xstream_self_rank(&my_rank);
    //cout << "sample_processing_gvt my_rank = " << my_rank << endl;
    damaris::BlocksByIteration::iterator begin, end;
    DUtil::get_damaris_iterators(inst_buffer_names[mode], step, begin, end);
    //cout << "sample_processing_gvt mode " << mode << " step " << step << endl;;
    if (begin == end)
    {
        // TODO there's potentially a bug here if this task is not done quickly enough,
        // the task may be too late to access the data
        // might be garbage collection acting too quickly?
        // What we should do is after we process this data,
        // set up garbage collection for this time step
        // we don't need it in damaris format anymore
        //cout << "sample_processing_gvt: begin == end\n";
    }

    map<double, SampleFBBuilder> fb_map;
    const InstMode *modes = EnumValuesInstMode();

    while (begin != end)
    {
        // each iterator is to a buffer sample from ROSS
        // For GVT and RTS, this is one buffer per PE per sampling point
        // for VTS, this is one buffer per KP per sampling point
        // for VTS and RTS, we may have more than one sampling point

        damaris::DataSpace<damaris::Buffer> ds((*begin)->GetDataSpace());
        size_t ds_size = ds.GetSize();
        table_builder->save_data(reinterpret_cast<char*>(ds.GetData()), ds_size);
        //char* dbuf_cur = reinterpret_cast<char*>(ds.GetData());
        //sample_metadata *sample_md = reinterpret_cast<sample_metadata*>(dbuf_cur);
        //dbuf_cur += sizeof(*sample_md);
        //ds_size -= sizeof(*sample_md);

        ////cout << "peid: " << sample_md->peid << " vts: " << sample_md->vts << " rts: " << sample_md->rts
        ////    << " last_gvt: " << sample_md->last_gvt << endl;
        //auto sample_fbb = fb_map.find(sample_md->last_gvt);
        //if (sample_fbb == fb_map.end())
        //{
        //    //cout << "not found in fb_map for time " << sample_md->last_gvt << endl;
        //    auto ret = fb_map.insert(make_pair(sample_md->last_gvt, SampleFBBuilder(sample_md->vts,
        //                    sample_md->rts, sample_md->last_gvt, modes[mode])));
        //    sample_fbb = ret.first;
        //}

        //create_flatbuffer(modes[mode], dbuf_cur, ds_size, sample_md, sample_fbb->second);

        ++begin;
    }

    return fb_map;
}

map<double, SampleFBBuilder> sample_processing_rts(int mode, int step)
{
    damaris::BlocksByIteration::iterator begin, end;
    DUtil::get_damaris_iterators(inst_buffer_names[mode], step, begin, end);
    //cout << "sample_processing_rts mode " << mode << " step " << step << endl;;
    if (begin == end)
    {
        // TODO there's potentially a bug here if this task is not done quickly enough,
        // the task may be too late to access the data
        // might be garbage collection acting too quickly?
        // What we should do is after we process this data,
        // set up garbage collection for this time step
        // we don't need it in damaris format anymore
        //cout << "sample_processing_rts: begin == end\n";
    }

    map<double, SampleFBBuilder> fb_map;
    const InstMode *modes = EnumValuesInstMode();

    while (begin != end)
    {
        // each iterator is to a buffer sample from ROSS
        // For GVT and RTS, this is one buffer per PE per sampling point
        // for VTS, this is one buffer per KP per sampling point
        // for VTS and RTS, we may have more than one sampling point

        damaris::DataSpace<damaris::Buffer> ds((*begin)->GetDataSpace());
        size_t ds_size = ds.GetSize();
        char* dbuf_cur = reinterpret_cast<char*>(ds.GetData());
        sample_metadata *sample_md = reinterpret_cast<sample_metadata*>(dbuf_cur);
        dbuf_cur += sizeof(*sample_md);
        ds_size -= sizeof(*sample_md);

        //cout << "peid: " << sample_md->peid << " vts: " << sample_md->vts << " rts: " << sample_md->rts
        //    << " last_gvt: " << sample_md->vts << endl;
        auto sample_fbb = fb_map.find(sample_md->rts);
        if (sample_fbb == fb_map.end())
        {
            auto ret = fb_map.insert(make_pair(sample_md->rts, SampleFBBuilder(sample_md->vts,
                            sample_md->rts, sample_md->last_gvt, modes[mode])));
            sample_fbb = ret.first;
        }

        create_flatbuffer(modes[mode], dbuf_cur, ds_size, sample_md, sample_fbb->second);

        ++begin;
    }

    return fb_map;
}

// TODO for the time being, we are only handling model data with VTS
void sample_processing_vts(int mode, int step)
{
    //cout << "[sp_vts] step " << step << endl;
    damaris::BlocksByIteration::iterator begin, end;
    DUtil::get_damaris_iterators(inst_buffer_names[mode], step, begin, end);
    if (begin == end)
    {
        // TODO there's potentially a bug here if this task is not done quickly enough,
        // the task may be too late to access the data
        // might be garbage collection acting too quickly?
        // What we should do is after we process this data,
        // set up garbage collection for this time step
        // we don't need it in damaris format anymore
        //cout << "sample_processing_vts: begin == end\n";
        return;
    }

    // need to make 2 flatbuffers; 1 for sim engine and 1 for model
    // sim engine can be forwarded now, but model data may be incorrect
    // model data needs to be stored until notified that either rollback or commit occurred

    map<double, SampleFBBuilder> fb_map;
    const InstMode *modes = EnumValuesInstMode();

    // TODO should combine the KPs from the same sampling period into one flatbuffer
    while (begin != end)
    {
        // each iterator is to a buffer sample from ROSS
        // For GVT and RTS, this is one buffer per PE per sampling point
        // for VTS, this is one buffer per KP per sampling point
        // for VTS and RTS, we may have more than one sampling point

        damaris::DataSpace<damaris::Buffer> ds((*begin)->GetDataSpace());
        size_t ds_size = ds.GetSize();
        char* dbuf_cur = reinterpret_cast<char*>(ds.GetData());
        sample_metadata *sample_md = reinterpret_cast<sample_metadata*>(dbuf_cur);
        dbuf_cur += sizeof(*sample_md);
        ds_size -= sizeof(*sample_md);

        auto it = vtsamples.get<by_kp_vt_rt>().find(make_tuple(sample_md->kp_gid, sample_md->vts,
                    sample_md->rts));
        SampleFBBuilder* sample_fbb;
        if (it != vtsamples.get<by_kp_vt_rt>().end())
        {
            // this should actually be an error, because RTS should be unique for a given KP, VTS pair
            //cout << "ERROR: sample processing vts, this sample already exists!\n";
            //cout << "pe " << sample_md->peid << " kp " << sample_md->kp_gid << " rts "
            //    << sample_md->rts << " vts " << sample_md->vts << endl;
        }
        else
        {
            //cout << "[vtsamples]: add pe " << sample_md->peid << " kp " << sample_md->kp_gid << " vts "
            //    << sample_md->vts << " rts " << sample_md->rts << endl;
            auto samp = vtsamples.insert(std::make_shared<VTSample>(sample_md->kp_gid, sample_md->vts,
                        sample_md->rts, sample_md->last_gvt));
            sample_fbb = (*(samp.first))->get_sample_fbb();
            create_flatbuffer(modes[mode], dbuf_cur, ds_size, sample_md, *sample_fbb);
        }


        ++begin;
    }
    //cout << "sizeof vtsamples " << vtsamples.size() << endl;
}

void remove_invalid_data(void *arguments)
{
    initial_task_args *args;
    args = (initial_task_args*)arguments;

    damaris::BlocksByIteration::iterator begin, end;
    DUtil::get_damaris_iterators("ross/vt_rb/vts", args->step, begin, end);
    while (begin != end)
    {
        double vts = *(static_cast<double*>((*begin)->GetDataSpace().GetData()));
        int kpid = *(static_cast<int*>(DUtil::get_value_from_damaris("ross/vt_rb/kp_gid",
                    (*begin)->GetSource(), (*begin)->GetIteration(), (*begin)->GetID())));
        double rts = *(static_cast<double*>(DUtil::get_value_from_damaris("ross/vt_rb/rts",
                    (*begin)->GetSource(), (*begin)->GetIteration(), (*begin)->GetID())));

        auto sample_it = vtsamples.get<by_kp_vt_rt>().find(make_tuple(kpid, vts, rts));
        if (sample_it != vtsamples.get<by_kp_vt_rt>().end())
        {
            vtsamples.get<by_kp_vt_rt>().erase(sample_it);
            //cout << "[vtsamples]: del kp " << kpid << " vts "
            //    << vts << " rts " << rts << endl;
        }

        ++begin;
    }

    //cout << "sizeof vtsamples " << vtsamples.size() << endl;
    free(args);
}

void forward_vts_task(void* arguments)
{
    static double last_processed_vts = 0.0;
    forward_task_args *args;
    args = (forward_task_args*)arguments;
    //cout <<"[forward] lower " << args->lower << " last_gvt " << args->last_gvt 
    //    << " last_processed_vts " << last_processed_vts << endl;

    if (last_processed_vts + sim_config->vt_interval() >= args->last_gvt)
    {
        free(args);
        return;
    }

    double cur_vts = last_processed_vts + sim_config->vt_interval();

    //cout << "forward data starting at " << cur_vts << endl;
    while (cur_vts <= args->last_gvt)
    {
        //cout << "Forwarding data for VTS: " << cur_vts << endl;
        //cout << "sizeof vtsamples " << vtsamples.size() << endl;
        auto begin = vtsamples.get<by_vts>().find(cur_vts);
        if (begin == vtsamples.get<by_vts>().end())
        {
            //cout << "ERROR: forwarding no samples found!\n";
            free(args);
            return;
        }
        DamarisDataSampleT combined;
        (*begin)->get_sample_fbb()->finish();
        fb::FlatBufferBuilder* fbb = (*begin)->get_sample_fbb()->get_fbb();
        auto ds = GetDamarisDataSample(fbb->GetBufferPointer());
        ds->UnPackTo(&combined);
        ++begin;

        //cout << "model_data vector size " << combined.model_data.size() << endl;
        while (begin != vtsamples.get<by_vts>().end())
        {
            //SampleFBBuilder* sample_fbb = (*begin)->get_sample_fbb();
            (*begin)->get_sample_fbb()->finish();
            fb::FlatBufferBuilder* fbb = (*begin)->get_sample_fbb()->get_fbb();
            auto ds = GetDamarisDataSample(fbb->GetBufferPointer());
            DamarisDataSampleT* cur_obj = ds->UnPack();
            std::move(cur_obj->model_data.begin(), cur_obj->model_data.end(),
                    std::back_inserter(combined.model_data));
            delete cur_obj;
            //cout << "model_data vector size " << combined.model_data.size() << endl;

            ++begin;
        }
        stream_client->enqueue_data(&combined);
        cur_vts += sim_config->vt_interval();
    }

    last_processed_vts = cur_vts - sim_config->vt_interval();
    free(args);
}

void feature_extraction_task(void* arguments)
{
    // TODO figure out how to get only the last 10 steps
    int *num_steps = reinterpret_cast<int*>(arguments);
    vtkSmartPointer<FeatureExtractor> extractor = FeatureExtractor::New();
    extractor->SetInputData(toUType(Port::PE_DATA), table_builder->pe_pds);
    extractor->SetNumSteps(*num_steps);
    extractor->Update();
    free(num_steps);
}
