#include <plugins/util/fb-util.h>
#include <plugins/util/damaris-util.h>

using namespace ross_damaris::util;
using namespace ross_damaris::sample;
using namespace std;

template <typename T>
void FBUtil::add_metric(SimEngineMetricsT *obj, const std::string& var_name, T value)
{
    if (var_name.compare("nevent_processed") == 0)
        obj->nevent_processed = value;
    else if (var_name.compare("nevent_abort") == 0)
        obj->nevent_abort = value;
    else if (var_name.compare("nevent_rb") == 0)
        obj->nevent_rb = value;
    else if (var_name.compare("rb_total") == 0)
        obj->rb_total = value;
    else if (var_name.compare("rb_prim") == 0)
        obj->rb_prim = value;
    else if (var_name.compare("rb_sec") == 0)
        obj->rb_sec = value;
    else if (var_name.compare("fc_attempts") == 0)
        obj->fc_attempts = value;
    else if (var_name.compare("pq_qsize") == 0)
        obj->pq_qsize = value;
    else if (var_name.compare("network_send") == 0)
        obj->network_send = value;
    else if (var_name.compare("network_recv") == 0)
        obj->network_recv = value;
    else if (var_name.compare("num_gvt") == 0)
        obj->num_gvt = value;
    else if (var_name.compare("event_ties") == 0)
        obj->event_ties = value;
    else if (var_name.compare("efficiency") == 0)
        obj->efficiency = value;
    else if (var_name.compare("net_read_time") == 0)
        obj->net_read_time = value;
    else if (var_name.compare("net_other_time") == 0)
        obj->net_other_time = value;
    else if (var_name.compare("gvt_time") == 0)
        obj->gvt_time = value;
    else if (var_name.compare("fc_time") == 0)
        obj->fc_time = value;
    else if (var_name.compare("event_abort_time") == 0)
        obj->event_abort_time = value;
    else if (var_name.compare("event_proc_time") == 0)
        obj->event_proc_time = value;
    else if (var_name.compare("pq_time") == 0)
        obj->pq_time = value;
    else if (var_name.compare("rb_time") == 0)
        obj->rb_time = value;
    else if (var_name.compare("cancel_q_time") == 0)
        obj->cancel_q_time = value;
    else if (var_name.compare("avl_time") == 0)
        obj->avl_time = value;
    else if (var_name.compare("virtual_time_diff") == 0)
        obj->virtual_time_diff = value;
    else
        cout << "ERROR in get_metric_fn_ptr() var " << var_name << "not found!" << endl;
}

void FBUtil::collect_metrics(SimEngineMetricsT *metrics_objects, const std::string& var_prefix,
        int32_t src, int32_t step, int32_t block_id, int32_t num_entities)
{
    (void) block_id;

    const flatbuffers::TypeTable *tt = SimEngineMetricsTypeTable();
    for (unsigned int i = 0; i < tt->num_elems; i++)
    {
        flatbuffers::ElementaryType type = static_cast<flatbuffers::ElementaryType>(tt->type_codes[i].base_type);
        switch (type)
        {
            case flatbuffers::ET_INT:
            {
                auto *val = static_cast<int*>(DUtil::get_value_from_damaris(var_prefix + tt->names[i], src, step, 0));
                if (val)
                {
                    for (int j = 0; j < num_entities; j++)
                        add_metric(&metrics_objects[j], tt->names[i], val[j]);
                }
                break;
            }
            case flatbuffers::ET_FLOAT:
            {
                auto *val = static_cast<float*>(DUtil::get_value_from_damaris(var_prefix + tt->names[i], src, step, 0));
                if (val)
                {
                    for (int j = 0; j < num_entities; j++)
                        add_metric(&metrics_objects[j], tt->names[i], val[j]);
                }
                break;
            }
            case flatbuffers::ET_DOUBLE:
            {
                auto *val = static_cast<double*>(DUtil::get_value_from_damaris(var_prefix + tt->names[i], src, step, 0));
                if (val)
                {
                    for (int j = 0; j < num_entities; j++)
                        add_metric(&metrics_objects[j], tt->names[i], val[j]);
                }
                break;
            }
            default:
            {
                cout << "collect_metrics(): this type hasn't been implemented!" << endl;
                break;
            }
        }
    }
}

