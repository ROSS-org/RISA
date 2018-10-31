#include <plugins/data/ArgobotsManager.h>
#include <plugins/data/analysis-tasks.h>

using namespace ross_damaris::data;
using namespace ross_damaris::sample;

ArgobotsManager::ArgobotsManager() :
    last_processed_gvt_(0.0),
    last_processed_rts_(0.0),
    last_processed_vts_(0.0),
    data_manager_(nullptr),
    stream_client_(nullptr),
    sim_config_(nullptr)
{
    primary_xstream_ = (ABT_xstream*)malloc(sizeof(ABT_xstream));
    proc_xstream_ = (ABT_xstream*)malloc(sizeof(ABT_xstream));
    pool_ = (ABT_pool*)malloc(sizeof(ABT_pool));
    scheduler_ = (ABT_sched*)malloc(sizeof(ABT_sched));

    // initialize Argobots library
    ABT_init(0, NULL);

    // create shared pool
    ABT_pool_create_basic(ABT_POOL_FIFO, ABT_POOL_ACCESS_MPMC, ABT_TRUE, pool_);

    ABT_xstream_self(primary_xstream_);
    ABT_xstream_create_basic(ABT_SCHED_DEFAULT, 1, pool_, ABT_SCHED_CONFIG_NULL, proc_xstream_);
    ABT_xstream_start(*proc_xstream_);
    int pool_id = -1;
    ABT_pool_get_id(*pool_, &pool_id);
    //cout << "[ArgobotsManager] shared pool " << pool_id << " created\n";
}

ArgobotsManager::ArgobotsManager(ArgobotsManager&& m) :
    last_processed_gvt_(m.last_processed_gvt_),
    last_processed_rts_(m.last_processed_rts_),
    last_processed_vts_(m.last_processed_vts_),
    data_manager_(std::move(m.data_manager_)),
    stream_client_(std::move(m.stream_client_)),
    sim_config_(std::move(m.sim_config_)),
    primary_xstream_(m.primary_xstream_),
    proc_xstream_(m.proc_xstream_),
    pool_(m.pool_),
    scheduler_(m.scheduler_)
{
    m.primary_xstream_ = nullptr;
    m.proc_xstream_ = nullptr;
    m.pool_ = nullptr;
    m.scheduler_ = nullptr;
}

ArgobotsManager::~ArgobotsManager()
{
    if (primary_xstream_)
        free(primary_xstream_);
    if (proc_xstream_)
        free(proc_xstream_);
    if (pool_)
        free(pool_);
    if (scheduler_)
        free(scheduler_);
}

void ArgobotsManager::set_shared_ptrs(boost::shared_ptr<DataManager>& dm_ptr,
            boost::shared_ptr<streaming::StreamClient>& sc_ptr,
            boost::shared_ptr<config::SimConfig>& conf_ptr)
{
    data_manager_ = boost::shared_ptr<DataManager>(dm_ptr);
    stream_client_ = boost::shared_ptr<streaming::StreamClient>(sc_ptr);
    sim_config_ = boost::shared_ptr<config::SimConfig>(conf_ptr);

    init_args *args = (init_args*)malloc(sizeof(init_args));
    args->data_manager = reinterpret_cast<void*>(&data_manager_);
    args->sim_config = reinterpret_cast<void*>(&sim_config_);
    args->stream_client = reinterpret_cast<void*>(&stream_client_);
    ABT_task_create(*pool_, initialize_task, args, NULL);
}

void ArgobotsManager::forward_model_data()
{

}

void ArgobotsManager::create_insert_data_mic_task(int32_t step)
{
    initial_task_args *args = (initial_task_args*)malloc(sizeof(initial_task_args));
    args->step = step;
    ABT_task_create(*pool_, initial_data_processing, args, NULL);
}

void ArgobotsManager::finalize()
{
    // Can't put these calls in the destructor, else they might get called before
    // they're supposed to
    if (ABT_initialized() == ABT_SUCCESS)
    {
        ABT_xstream_join(*proc_xstream_);
        ABT_xstream_free(proc_xstream_);
        ABT_finalize();
    }
}
