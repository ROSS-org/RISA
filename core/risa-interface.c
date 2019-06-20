#include <sys/stat.h>
#include <core/risa-interface.h>
#include <plugins/util/config-c.h>
#include <instrumentation/st-instrumentation-internal.h>
#include <instrumentation/st-sim-engine.h>
#include <instrumentation/st-model-data.h>

int g_st_ross_rank = 0;
int g_st_risa_enabled = 0;

static int damaris_initialized = 0;
static int rt_block_counter = 0;
static int vt_block_counter = 0;
static int vt_invalid_block_counter = 0;
static int vt_valid_block_counter = 0;
static int iterations = 0;
static char damaris_xml[4096];

static const char* inst_var_name[] = {
    "ross/inst_sample/gvt_inst",
    "ross/inst_sample/rts_inst",
    "ross/inst_sample/vts_inst"
};

static const tw_optdef risa_options[] = {
    TWOPT_GROUP("RISA - ROSS In Situ Analysis"),
    TWOPT_UINT("enable-risa", g_st_risa_enabled, "Turn on (1) or off (0) RISA"),
    TWOPT_CHAR("damaris-xml", damaris_xml, "Path to XML file for describing data to Damaris"),
    TWOPT_END()
};

const tw_optdef *risa_opts(void)
{
    return risa_options;
}

void risa_init(void)
{
    int err, my_rank;
    MPI_Comm ross_comm;

    if (!g_st_risa_enabled)
    {
        g_st_ross_rank = 1;
        return;
    }

    struct stat buffer;
    int file_check = stat(damaris_xml, &buffer);
    if (file_check != 0)
        tw_error(TW_LOC, "Need to provide the appropriate XML metadata file for Damaris using"
                "--damaris-xml!");

    if ((err = damaris_initialize(damaris_xml, MPI_COMM_WORLD)) != DAMARIS_OK)
        risa_damaris_error(TW_LOC, err, NULL);
    damaris_initialized = 1;

    // g_st_ross_rank > 0: ROSS MPI rank
    // g_st_ross_rank == 0: Damaris MPI rank
    if ((err = damaris_start(&g_st_ross_rank)) != DAMARIS_OK)
        risa_damaris_error(TW_LOC, err, NULL);
    if(g_st_ross_rank)
    {
        //get ROSS sub comm, sets to MPI_COMM_ROSS
        if ((err = damaris_client_comm_get(&ross_comm)) != DAMARIS_OK)
            risa_damaris_error(TW_LOC, err, NULL);
        tw_comm_set(ross_comm);

        // now make sure we have the correct rank number for ROSS ranks
        if (MPI_Comm_rank(MPI_COMM_ROSS, &my_rank) != MPI_SUCCESS)
            tw_error(TW_LOC, "Cannot get MPI_Comm_rank (MPI_COMM_ROSS)");

        g_tw_mynode = my_rank;
    }
}

void risa_parse_config(const char *config_file)
{
    parse_file(config_file);
}

void risa_inst_init(const char *config_file)
{
    set_parameters(config_file);

    // get some metadata about model data to RISA
    int block = 0, err;
    if (g_st_model_stats)
    {
        for (int i = 0; i < g_tw_nlp; i++)
        {
            tw_lp* clp = g_tw_lp[i];
            // doesn't matter which instrumentation mode is being used for this
            if (!clp->model_types || clp->model_types->num_vars <= 0)
                continue;

            // for each LP, RISA needs to know the names of its variables and full model LP name
            size_t buf_size = sizeof(model_lp_metadata);
            buf_size += strlen(clp->model_types->lp_name) + 1;
            for (int j = 0; j < clp->model_types->num_vars; j++)
            {
                buf_size += sizeof(mlp_var_metadata);
                buf_size += strlen(clp->model_types->model_vars[j].var_name) + 1;
            }

            if ((err = damaris_parameter_set("md_size", &buf_size, sizeof(buf_size)))
                    != DAMARIS_OK)
                risa_damaris_error(TW_LOC, err, "md_size");

            void *dbuf_ptr;
            if ((err = damaris_alloc_block("model_map/lp_md", block, &dbuf_ptr))
                    != DAMARIS_OK)
                risa_damaris_error(TW_LOC, err, "model_map/lp_md");
            char *damaris_buf = (char*)dbuf_ptr;

            model_lp_metadata *md = (model_lp_metadata*)damaris_buf;
            damaris_buf += sizeof(*md);
            buf_size -= sizeof(*md);
            md->peid = (int)clp->pe->id;
            md->kpid = (int)clp->kp->id;
            md->lpid = (int)clp->gid;
            md->num_vars = clp->model_types->num_vars;
            md->name_sz = strlen(clp->model_types->lp_name) + 1;
            memcpy(damaris_buf, clp->model_types->lp_name, md->name_sz);
            damaris_buf += md->name_sz;
            buf_size -= md->name_sz;

            for (int j = 0; j < md->num_vars; j++)
            {
                mlp_var_metadata *var_md = (mlp_var_metadata*)damaris_buf;
                damaris_buf += sizeof(*var_md);
                buf_size -= sizeof(*var_md);
                var_md->id = j;
                var_md->name_sz = strlen(clp->model_types->model_vars[j].var_name) + 1;
                memcpy(damaris_buf, clp->model_types->model_vars[j].var_name, var_md->name_sz);
                damaris_buf += var_md->name_sz;
                buf_size -= var_md->name_sz;
            }
            if (buf_size != 0)
                printf("risa_inst_init buf_size = %lu\n", buf_size);

            if ((err = damaris_commit_block_iteration("model_map/lp_md", block, 0))
                    != DAMARIS_OK)
                risa_damaris_error(TW_LOC, err, "model_map/lp_md");
            if ((err = damaris_clear_block_iteration("model_map/lp_md", block, 0))
                    != DAMARIS_OK)
                risa_damaris_error(TW_LOC, err, "model_map/lp_md");
            block++;
        }
//        printf("PE %ld: num blocks %d\n", g_tw_mynode, block);
    }

    signal_init();
}
void set_parameters(const char *config_file)
{
    int err;
    int num_pe = tw_nnodes();
    int num_kp = (int) g_tw_nkp;
    int num_lp = (int) g_tw_nlp;
    //int model_sample_size = 524288;
    //printf("PE %ld num pe %d, num kp %d, num lp %d\n", g_tw_mynode, num_pe, num_kp, num_lp);

    if ((err = damaris_parameter_set("num_lp", &num_lp, sizeof(num_lp))) != DAMARIS_OK)
        risa_damaris_error(TW_LOC, err, "num_lp");

    if ((err = damaris_parameter_set("num_pe", &num_pe, sizeof(num_pe))) != DAMARIS_OK)
        risa_damaris_error(TW_LOC, err, "num_pe");

    if ((err = damaris_parameter_set("num_kp", &num_kp, sizeof(num_kp))) != DAMARIS_OK)
        risa_damaris_error(TW_LOC, err, "num_kp");

    //if ((err = damaris_parameter_set("sample_size", &model_sample_size, sizeof(model_sample_size))) != DAMARIS_OK)
    //    risa_damaris_error(TW_LOC, err, "sample_size");

    if ((err = damaris_write("ross/nlp", &num_lp)) != DAMARIS_OK)
        risa_damaris_error(TW_LOC, err, "num_lp");

    if ((err = damaris_write("ross/nkp", &num_kp)) != DAMARIS_OK)
        risa_damaris_error(TW_LOC, err, "num_kp");

    if (config_file)
    {
        if ((err = damaris_write("ross/inst_config", &config_file[0])) != DAMARIS_OK)
            risa_damaris_error(TW_LOC, err, "ross/inst_config");
    }
}

void signal_init()
{
    int err;
    if ((err = damaris_signal("damaris_rank_init")) != DAMARIS_OK)
        risa_damaris_error(TW_LOC, err, "damaris_rank_init");
}

void risa_init_print()
{
    int total_size;
    MPI_Comm_size(MPI_COMM_WORLD, &total_size);

    printf("\nDamaris Configuration: \n");
    printf("\t%-50s %11u\n", "Total Ranks", total_size - tw_nnodes());
}

void risa_finalize()
{
    if (!damaris_initialized)
        return;
    int err;
    if (g_st_ross_rank)
    {
        double gvt = DBL_MAX;
        if ((err = damaris_write("ross/last_gvt", &gvt)) != DAMARIS_OK)
            risa_damaris_error(TW_LOC, err, "ross/last_gvt");

        if ((err = damaris_end_iteration()) != DAMARIS_OK)
            risa_damaris_error(TW_LOC, err, "end iteration");
        //printf("[%f] PE %ld damaris_end_iteration %d num_blocks: %d\n",
        //        g_tw_pe[0]->GVT, g_tw_mynode, iterations, vt_block_counter);
        if ((err = damaris_signal("end_iteration")) != DAMARIS_OK)
            risa_damaris_error(TW_LOC, err, "end_iteration");
        if ((err = damaris_signal("damaris_rank_finalize")) != DAMARIS_OK)
            risa_damaris_error(TW_LOC, err, "damaris_rank_finalize");
        damaris_stop();
    }
    if ((err = damaris_finalize()) != DAMARIS_OK)
        risa_damaris_error(TW_LOC, err, NULL);
    if (MPI_Finalize() != MPI_SUCCESS)
        tw_error(TW_LOC, "Failed to finalize MPI");
}

void risa_expose_data_gvt(tw_pe *me, int inst_type)
{
    //printf("PE %ld: damaris_expose_data type: %d\n", g_tw_mynode, inst_type);
    int err;
    int block = 0;
    int data_sampled = 0;
    size_t buf_size = 0;
    char* damaris_buf;
    sample_metadata* sample_md;

    if (engine_modes[inst_type] || model_modes[inst_type])
    {
        buf_size = sizeof(*sample_md);
        if (engine_modes[inst_type])
            buf_size += engine_data_sizes[inst_type];

        if (model_modes[inst_type])
            buf_size += model_data_sizes[inst_type];
        //printf("st_inst_sample: buf_size %lu\n", buf_size);

        if (buf_size > sizeof(sample_metadata))
        {
            data_sampled = 1;
            if ((err = damaris_parameter_set("sample_size", &buf_size, sizeof(buf_size)))
                    != DAMARIS_OK)
                risa_damaris_error(TW_LOC, err, "sample_size");

            //printf("getting pointer to shared memory from Damaris for size %lu\n", buf_size);
            void* dbuf_ptr;
            if ((err = damaris_alloc_block(inst_var_name[inst_type], block, &dbuf_ptr))
                    != DAMARIS_OK)
                risa_damaris_error(TW_LOC, err, inst_var_name[inst_type]);
            damaris_buf = (char*)dbuf_ptr;

            sample_md = (sample_metadata*)damaris_buf;
            damaris_buf += sizeof(*sample_md);
            buf_size -= sizeof(*sample_md);

            bzero(sample_md, sizeof(*sample_md));
            sample_md->last_gvt = me->GVT;
            sample_md->rts = tw_clock_read() / (double)g_tw_clock_rate;

            sample_md->peid = (unsigned int)g_tw_mynode;
            sample_md->num_model_lps = model_num_lps[inst_type];
            sample_md->kp_gid = -1;
        }
        else
        {
            buf_size = 0;
            //printf("%lu: Not writing data!\n", g_tw_mynode);
        }
    }

    if (buf_size && engine_modes[inst_type] && g_tw_synchronization_protocol != SEQUENTIAL)
    {
        st_collect_engine_data(me, inst_type, damaris_buf, engine_data_sizes[inst_type], sample_md, NULL);
        damaris_buf += engine_data_sizes[inst_type];
        buf_size -= engine_data_sizes[inst_type];
    }
    if (buf_size && model_modes[inst_type])
    {
        sample_md->has_model = 1;
        st_collect_model_data(me, inst_type, damaris_buf, model_data_sizes[inst_type]);
        buf_size -= model_data_sizes[inst_type];
    }

    if (buf_size != 0)
        tw_error(TW_LOC, "buf_size = %lu", buf_size);

    if (data_sampled)
    {
        //printf("risa-interface: peid: %u kp_gid: %d vts: %f rts: %f last_gvt: %f\n", sample_md->peid,
        //        sample_md->kp_gid, sample_md->vts, sample_md->rts, sample_md->last_gvt);
        if ((err = damaris_commit_block(inst_var_name[inst_type], block))
                != DAMARIS_OK)
            risa_damaris_error(TW_LOC, err, inst_var_name[inst_type]);
        if ((err = damaris_clear_block(inst_var_name[inst_type], block))
                != DAMARIS_OK)
            risa_damaris_error(TW_LOC, err, inst_var_name[inst_type]);
    }

}

void risa_expose_data_rts(tw_pe *me, int inst_type)
{
    static double rts_count = 0.0;
    //printf("PE %ld: damaris_expose_data type: %d\n", g_tw_mynode, inst_type);
    int err;
    int block = 0;
    if (inst_type == RT_INST)
    {
        block = rt_block_counter;
        rt_block_counter++;
    }

    int data_sampled = 0;
    size_t buf_size = 0;
    char* damaris_buf;
    sample_metadata* sample_md;

    if (engine_modes[inst_type] || model_modes[inst_type])
    {
        buf_size = sizeof(*sample_md);
        if (engine_modes[inst_type])
            buf_size += engine_data_sizes[inst_type];

        if (model_modes[inst_type])
            buf_size += model_data_sizes[inst_type];

        if (buf_size > sizeof(sample_metadata))
        {
            data_sampled = 1;
            if ((err = damaris_parameter_set("sample_size", &buf_size, sizeof(buf_size)))
                    != DAMARIS_OK)
                risa_damaris_error(TW_LOC, err, "sample_size");

            //printf("getting pointer to shared memory from Damaris for size %lu\n", buf_size);
            void* dbuf_ptr;
            if ((err = damaris_alloc_block(inst_var_name[inst_type], block, &dbuf_ptr))
                    != DAMARIS_OK)
                risa_damaris_error(TW_LOC, err, inst_var_name[inst_type]);
            damaris_buf = (char*)dbuf_ptr;

            sample_md = (sample_metadata*)damaris_buf;
            damaris_buf += sizeof(*sample_md);
            buf_size -= sizeof(*sample_md);

            bzero(sample_md, sizeof(*sample_md));
            sample_md->last_gvt = me->GVT;
            if (inst_type == RT_INST)
            {
                sample_md->rts = rts_count;
                rts_count += g_st_rt_int_ms;
            }

            sample_md->peid = (unsigned int)g_tw_mynode;
            sample_md->num_model_lps = model_num_lps[inst_type];
            sample_md->kp_gid = -1;
        }
        else
        {
            buf_size = 0;
            //printf("%lu: Not writing data!\n", g_tw_mynode);
        }
    }

    if (buf_size && engine_modes[inst_type] && g_tw_synchronization_protocol != SEQUENTIAL)
    {
        st_collect_engine_data(me, inst_type, damaris_buf, engine_data_sizes[inst_type], sample_md, NULL);
        damaris_buf += engine_data_sizes[inst_type];
        buf_size -= engine_data_sizes[inst_type];
    }
    if (buf_size && model_modes[inst_type])
    {
        sample_md->has_model = 1;
        st_collect_model_data(me, inst_type, damaris_buf, model_data_sizes[inst_type]);
        buf_size -= model_data_sizes[inst_type];
    }

    if (buf_size != 0)
        tw_error(TW_LOC, "buf_size = %lu", buf_size);

    if (data_sampled)
    {
        //printf("risa-interface: peid: %u kp_gid: %d vts: %f rts: %f last_gvt: %f\n", sample_md->peid,
        //        sample_md->kp_gid, sample_md->vts, sample_md->rts, sample_md->last_gvt);
        if ((err = damaris_commit_block(inst_var_name[inst_type], block))
                != DAMARIS_OK)
            risa_damaris_error(TW_LOC, err, inst_var_name[inst_type]);
        if ((err = damaris_clear_block(inst_var_name[inst_type], block))
                != DAMARIS_OK)
            risa_damaris_error(TW_LOC, err, inst_var_name[inst_type]);
    }

}

// TODO may be able to simplfiy this and just grab the buffer pointer and return it to
// the call from inst_sample
void risa_expose_data_vts(tw_pe *me, int inst_type, tw_lp* lp, int vts_commit)
{
    if (vts_commit)
        tw_error(TW_LOC, "should not happen at commit!");
    //printf("PE %ld: damaris_expose_data type: %d\n", g_tw_mynode, inst_type);
    int err;
    int block = 0;
    if (inst_type == VT_INST && !vts_commit)
    {
        block = vt_block_counter;
        vt_block_counter++;
    }

    int data_sampled = 0;
    size_t buf_size = 0;
    char* damaris_buf;
    sample_metadata* sample_md;

    if (inst_type == VT_INST)
    {
        // For now we're not worried about getting sim engine data in VTS
        engine_modes[inst_type] = 0;
    }
    if (engine_modes[inst_type] || model_modes[inst_type])
    {
        buf_size = sizeof(*sample_md);
        // TODO handle VTS differently when using damaris?
        if (engine_modes[VT_INST])
            engine_data_sizes[VT_INST] = calc_sim_engine_sample_size_vts(lp);
        if (!vts_commit && engine_modes[inst_type])
            buf_size += engine_data_sizes[inst_type];

        if (model_modes[inst_type] && !vts_commit)
        {
            // when using damaris, we only want to send when it isn't commit time
            // at commit or RC we will just send some basic info saying this sample is valid/invalid
            model_data_sizes[inst_type] = get_model_data_size(lp->cur_state, &model_num_lps[inst_type]);
            buf_size += model_data_sizes[inst_type];
            //printf("%lu: size = %lu\n", g_tw_mynode, model_data_sizes[inst_type]);
        }

        if (buf_size > sizeof(sample_metadata))
        {
            data_sampled = 1;
            if ((err = damaris_parameter_set("sample_size", &buf_size, sizeof(buf_size)))
                    != DAMARIS_OK)
                risa_damaris_error(TW_LOC, err, "sample_size");

            //printf("getting pointer to shared memory from Damaris for size %lu\n", buf_size);
            void* dbuf_ptr;
            if ((err = damaris_alloc_block(inst_var_name[inst_type], block, &dbuf_ptr))
                    != DAMARIS_OK)
                risa_damaris_error(TW_LOC, err, inst_var_name[inst_type]);
            damaris_buf = (char*)dbuf_ptr;

            sample_md = (sample_metadata*)damaris_buf;
            damaris_buf += sizeof(*sample_md);
            buf_size -= sizeof(*sample_md);
            //printf("sizeof sample_md %lu\n", sizeof(*sample_md));

            bzero(sample_md, sizeof(*sample_md));
            sample_md->last_gvt = me->GVT;
            //if(inst_type == VT_INST)
            //{
            //    // TODO this happens only at commit time, so this is incorrect
            //    //sample_md->vts = tw_now(lp);
            //    //printf("sample_md->vts %f\n", sample_md->vts);
            //}

            sample_md->peid = (unsigned int)g_tw_mynode;
            sample_md->num_model_lps = model_num_lps[inst_type];
            sample_md->kp_gid = (int)(g_tw_mynode * g_tw_nkp) + (int)lp->kp->id;
            sample_md->vts = tw_now(lp);
        }
        else
        {
            buf_size = 0;
            //printf("%lu: Not writing data! vts_commit = %d\n", g_tw_mynode, vts_commit);
        }
    }

    if (!vts_commit && buf_size && engine_modes[inst_type] && g_tw_synchronization_protocol != SEQUENTIAL)
    {
        st_collect_engine_data(me, inst_type, damaris_buf, engine_data_sizes[inst_type], sample_md, lp);
        damaris_buf += engine_data_sizes[inst_type];
        buf_size -= engine_data_sizes[inst_type];
    }
    if (!vts_commit && buf_size && model_modes[inst_type])
    {
        sample_md->has_model = 1;
        st_collect_model_data_vts(me, lp, inst_type, damaris_buf, sample_md, model_data_sizes[inst_type]);
        buf_size -= model_data_sizes[inst_type];
    }

    if (buf_size != 0)
        tw_error(TW_LOC, "buf_size = %lu", buf_size);

    if (data_sampled)
    {
        //printf("risa-interface: peid: %u kp_gid: %d vts: %f rts: %f last_gvt: %f\n", sample_md->peid,
        //        sample_md->kp_gid, sample_md->vts, sample_md->rts, sample_md->last_gvt);
        if ((err = damaris_commit_block(inst_var_name[inst_type], block))
                != DAMARIS_OK)
            risa_damaris_error(TW_LOC, err, inst_var_name[inst_type]);
        if ((err = damaris_clear_block(inst_var_name[inst_type], block))
                != DAMARIS_OK)
            risa_damaris_error(TW_LOC, err, inst_var_name[inst_type]);
    }

}

void risa_end_iteration(tw_stime gvt)
{
    int err;
    //if ((err = damaris_signal("damaris_gc")) != DAMARIS_OK)
    //    risa_damaris_error(TW_LOC, err, "damaris_gc");

    if (g_tw_gvt_done % g_st_num_gvt == 0)
    {
        if ((err = damaris_write("ross/last_gvt", &gvt)) != DAMARIS_OK)
            risa_damaris_error(TW_LOC, err, "ross/last_gvt");

        if ((err = damaris_end_iteration()) != DAMARIS_OK)
            risa_damaris_error(TW_LOC, err, "end iteration");
        //printf("[%f] PE %ld damaris_end_iteration %d num_blocks: %d\n",
        //        g_tw_pe[0]->GVT, g_tw_mynode, iterations, vt_block_counter);
        if ((err = damaris_signal("end_iteration")) != DAMARIS_OK)
            risa_damaris_error(TW_LOC, err, "end_iteration");
    }

    iterations++;
    rt_block_counter = 0;
    vt_block_counter = 0;
    vt_invalid_block_counter = 0;
    vt_valid_block_counter = 0;
}

// TODO update rts being a count and not system time
void risa_invalidate_sample(double vts, double rts, int kp_gid)
{
    //printf("invalidating sample vts %f, rts %f, kpgid %d\n", vts, rts, kp_gid);
    int err;

    if ((err = damaris_write_block("ross/vt_rb/vts", vt_invalid_block_counter, &vts)) != DAMARIS_OK)
        risa_damaris_error(TW_LOC, err, "ross/vt_rb/vts");
    if ((err = damaris_write_block("ross/vt_rb/rts", vt_invalid_block_counter, &rts)) != DAMARIS_OK)
        risa_damaris_error(TW_LOC, err, "ross/vt_rb/rts");
    if ((err = damaris_write_block("ross/vt_rb/kp_gid", vt_invalid_block_counter, &kp_gid)) != DAMARIS_OK)
        risa_damaris_error(TW_LOC, err, "ross/vt_rb/kp_gid");
    vt_invalid_block_counter++;
}

void risa_validate_sample(double vts, double rts, int kp_gid)
{
    int err;

    if ((err = damaris_write_block("ross/vt_commit/vts", vt_valid_block_counter, &vts)) != DAMARIS_OK)
        risa_damaris_error(TW_LOC, err, "ross/vt_commit/vts");
    if ((err = damaris_write_block("ross/vt_commit/rts", vt_valid_block_counter, &rts)) != DAMARIS_OK)
        risa_damaris_error(TW_LOC, err, "ross/vt_commit/rts");
    if ((err = damaris_write_block("ross/vt_commit/kp_gid", vt_valid_block_counter, &kp_gid)) != DAMARIS_OK)
        risa_damaris_error(TW_LOC, err, "ross/vt_commit/kp_gid");
    vt_valid_block_counter++;
}

void risa_damaris_error(const char *file, int line, int err, const char *variable)
{
    switch(err)
    {
        case DAMARIS_ALLOCATION_ERROR:
            tw_warning(file, line, "Damaris allocation error for variable %s at GVT %f\n", variable, g_tw_pe[0]->GVT);
            break;
        case DAMARIS_ALREADY_INITIALIZED:
            tw_warning(file, line, "Damaris was already initialized\n");
            break;
        case DAMARIS_BIND_ERROR:
            tw_error(file, line, "Damaris bind error for Damaris event:  %s\n", variable);
            break;
        case DAMARIS_BLOCK_NOT_FOUND:
            tw_error(file, line, "Damaris not able to find block for variable:  %s\n", variable);
            break;
        case DAMARIS_CORE_IS_SERVER:
            tw_error(file, line, "This node is not a client.\n");
            break;
        case DAMARIS_DATASPACE_ERROR:
            tw_error(file, line, "Damaris dataspace error for variable:  %s\n", variable);
            break;
        case DAMARIS_INIT_ERROR:
            tw_error(file, line, "Error calling damaris_init().\n");
            break;
        case DAMARIS_FINALIZE_ERROR:
            tw_error(file, line, "Error calling damaris_finalize().\n");
            break;
        case DAMARIS_INVALID_BLOCK:
            tw_error(file, line, "Damaris invalid block for variable:  %s\n", variable);
            break;
        case DAMARIS_NOT_INITIALIZED:
            tw_warning(file, line, "Damaris has not been initialized\n");
            break;
        case DAMARIS_UNDEFINED_VARIABLE:
            tw_error(file, line, "Damaris variable %s is undefined\n", variable);
            break;
        case DAMARIS_UNDEFINED_ACTION:
            tw_error(file, line, "Damaris action %s is undefined\n", variable);
            break;
        case DAMARIS_UNDEFINED_PARAMETER:
            tw_error(file, line, "Damaris parameter %s is undefined\n", variable);
            break;
        default:
            tw_error(file, line, "Damaris error unknown. %s \n", variable);
    }
}

