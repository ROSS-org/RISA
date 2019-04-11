#include <plugins/data/ArgobotsManager.h>
#include <plugins/data/analysis-tasks.h>
#include <plugins/util/damaris-util.h>
#include <iostream>

using namespace ross_damaris::data;
using namespace ross_damaris::sample;
using namespace ross_damaris::util;
using namespace std;

ArgobotsManager* ArgobotsManager::get_instance()
{
    static ArgobotsManager instance;
    return &instance;
}

ArgobotsManager::ArgobotsManager() :
    last_processed_gvt_(0.0),
    last_processed_rts_(0.0),
    last_processed_vts_(0.0),
    primary_rank_(new int(-1)),
    proc_rank_(new int(-1))
{
    init_analysis_tasks();
    primary_xstream_ = (ABT_xstream*)malloc(sizeof(ABT_xstream));
    proc_xstream_ = (ABT_xstream*)malloc(sizeof(ABT_xstream));
    pool_ = (ABT_pool*)malloc(sizeof(ABT_pool));
    scheduler_ = (ABT_sched*)malloc(sizeof(ABT_sched));

    // initialize Argobots library
    ABT_init(0, NULL);
    //if (ABT_initialized() == ABT_SUCCESS)
    //    cout << "argobots successfully started!\n";

    // create shared pool
    ABT_pool_create_basic(ABT_POOL_FIFO, ABT_POOL_ACCESS_MPMC, ABT_TRUE, pool_);

    ABT_xstream_self(primary_xstream_);
    ABT_xstream_create_basic(ABT_SCHED_DEFAULT, 1, pool_, ABT_SCHED_CONFIG_NULL, proc_xstream_);
    //int rc = ABT_xstream_start(*proc_xstream_);
    //if (rc == ABT_SUCCESS)
    //    cout << "processing ES successfully started!\n";
    ABT_xstream_get_rank(*primary_xstream_, primary_rank_);
    ABT_xstream_get_rank(*proc_xstream_, proc_rank_);
    //cout << "main rank " << *primary_rank_ << ",  processing rank " << *proc_rank_ << endl;
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
    int my_rank;
    ABT_xstream_self_rank(&my_rank);
    //cout << "create_initial_data_task my_rank = " << my_rank << endl;
    initial_task_args *args = (initial_task_args*)malloc(sizeof(initial_task_args));
    args->step = step;
    ABT_task_create(*pool_, initial_data_processing, args, NULL);
    // TODO could add a check for if we're even collecting for VTS
    check_for_rb_data(step);
}

void ArgobotsManager::check_for_rb_data(int32_t step)
{
    int num_blocks = DUtil::get_num_blocks("ross/vt_rb/vts", step);
    if (num_blocks > 0)
    {
        initial_task_args *args = (initial_task_args*)malloc(sizeof(initial_task_args));
        args->step = step;
        ABT_task_create(*pool_, remove_invalid_data, args, NULL);
    }
}

void ArgobotsManager::forward_vts_data(double gvt)
{
    //cout << "[ArgobotsManager] forward data before gvt: " << gvt << endl;
    //cout << "[ArgobotsManager] last_processed_gvt_: " << last_processed_gvt_ << endl;
    forward_task_args *args = (forward_task_args*)malloc(sizeof(forward_task_args));
    args->lower = last_processed_gvt_;
    args->last_gvt = gvt;
    ABT_task_create(*pool_, forward_vts_task, args, NULL);
    last_processed_gvt_ = gvt;
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
