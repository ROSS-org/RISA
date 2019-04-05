#include <plugins/data/ArgobotsManager.h>
#include <plugins/data/analysis-tasks.h>
#include <iostream>

using namespace ross_damaris::data;
using namespace ross_damaris::sample;

ArgobotsManager* ArgobotsManager::instance = nullptr;

ArgobotsManager* ArgobotsManager::create_instance()
{
    if (instance)
        std::cout << "ArgobotsMananger error!\n";
    instance = new ArgobotsManager();
    return instance;
}

ArgobotsManager* ArgobotsManager::get_instance()
{
    if (!instance)
        std::cout << "ArgobotsMananger error!\n";
    return instance;
}

void ArgobotsManager::free_instance()
{
    if (instance)
        delete instance;
}

ArgobotsManager::ArgobotsManager() :
    last_processed_gvt_(0.0),
    last_processed_rts_(0.0),
    last_processed_vts_(0.0),
    stream_client_(streaming::StreamClient::get_instance()),
    sim_config_(config::SimConfig::get_instance())
{
    init_analysis_tasks();
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

void ArgobotsManager::create_initial_data_task(int32_t step)
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

    free_instance();
}
