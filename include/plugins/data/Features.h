#ifndef FEATURES_H
#define FEATURES_H

#include <unordered_map>
#include <string>
#include <plugins/flatbuffers/data_sample_generated.h>

namespace ross_damaris {
namespace data {

struct FeatureTypeMap : public std::unordered_map<std::string, sample::FeatureType>
{
    FeatureTypeMap()
    {
        this->operator[]("min") = sample::FeatureType_MIN_VAL;
        this->operator[]("max") = sample::FeatureType_MAX_VAL;
        this->operator[]("sum") = sample::FeatureType_SUM;
        this->operator[]("mean") = sample::FeatureType_MEAN;
        this->operator[]("m2") = sample::FeatureType_M2;
        this->operator[]("m3") = sample::FeatureType_M3;
        this->operator[]("m4") = sample::FeatureType_M4;
        this->operator[]("stddev") = sample::FeatureType_STD_DEV;
        this->operator[]("var") = sample::FeatureType_VAR;
        this->operator[]("skew") = sample::FeatureType_SKEW;
        this->operator[]("kurtosis") = sample::FeatureType_KURTOSIS;
    }
};

struct MetricTypeMap : public std::unordered_map<std::string, sample::MetricType>
{
    MetricTypeMap()
    {
        this->operator[]("event_proc") = sample::MetricType_EVENT_PROC;
        this->operator[]("event_abort") = sample::MetricType_EVENT_ABORT;
        this->operator[]("event_rbs") = sample::MetricType_EVENT_RB;
        this->operator[]("rb_total") = sample::MetricType_RB_TOTAL;
        this->operator[]("rb_primary") = sample::MetricType_RB_PRIM;
        this->operator[]("rb_secondary") = sample::MetricType_RB_SEC;
        this->operator[]("fc_attempts") = sample::MetricType_FC_ATTEMPTS;
        this->operator[]("pq_size") = sample::MetricType_PQ_QSIZE;
        this->operator[]("net_send") = sample::MetricType_NET_SEND;
        this->operator[]("net_recv") = sample::MetricType_NET_RECV;
        this->operator[]("num_gvt") = sample::MetricType_NUM_GVT;
        this->operator[]("pe_event_ties") = sample::MetricType_EVENT_TIES;
        this->operator[]("all_red_count") = sample::MetricType_ALLRED;
        this->operator[]("net_read_time") = sample::MetricType_NET_READ_TIME;
        this->operator[]("net_other_time") = sample::MetricType_NET_OTHER_TIME;
        this->operator[]("gvt_time") = sample::MetricType_GVT_TIME;
        this->operator[]("fc_time") = sample::MetricType_FC_TIME;
        this->operator[]("ev_abort_time") = sample::MetricType_EVENT_ABORT_TIME;
        this->operator[]("ev_proc_time") = sample::MetricType_EVENT_PROC_TIME;
        this->operator[]("pq_time") = sample::MetricType_PQ_TIME;
        this->operator[]("rb_time") = sample::MetricType_RB_TIME;
        this->operator[]("cancelq_time") = sample::MetricType_CANCEL_Q_TIME;
        this->operator[]("avl_time") = sample::MetricType_AVL_TIME;
        this->operator[]("buddy_time") = sample::MetricType_BUDDY_TIME;
        this->operator[]("lz4_time") = sample::MetricType_LZ4_TIME;
        this->operator[]("virtual_time_diff") = sample::MetricType_VIRT_TIME_DIFF;
    }
};

enum class PrimaryFeatures
{
    MIN,
    MAX,
    SUM,
    MEAN,
    M2,
    M3,
    M4,
    NUM_FEAT,
    START = MIN,
    END = M4
};

inline const char * const *EnumNamesPrimaryFeatures()
{
    static const char * const names[] =
    {
        "min",
        "max",
        "sum",
        "mean",
        "m2",
        "m3",
        "m4"
    };
    return names;
}

inline const char *EnumNamePrimaryFeatures(PrimaryFeatures e)
{
    if (e < PrimaryFeatures::START || e > PrimaryFeatures::END)
        return "";
    const size_t index = static_cast<int>(e);
    return EnumNamesPrimaryFeatures()[index];
}

enum class DerivedFeatures
{
    STD_DEV,
    VAR,
    SKEW,
    KURTOSIS,
    NUM_FEAT,
    START = STD_DEV,
    END = KURTOSIS
};

inline const char * const *EnumNamesDerivedFeatures()
{
    static const char * const names[] =
    {
        "stddev",
        "var",
        "skew",
        "kurtosis"
    };
    return names;
}

inline const char *EnumNameDerivedFeatures(DerivedFeatures e)
{
    if (e < DerivedFeatures::START || e > DerivedFeatures::END)
        return "";
    const size_t index = static_cast<int>(e);
    return EnumNamesDerivedFeatures()[index];
}

enum class Port
{
    PE_DATA,
    KP_DATA,
    LP_DATA
};

inline const char * const *EnumNamesPort()
{
    static const char * const names[] =
    {
        "pe",
        "kp",
        "lp"
    };
    return names;
}

inline const char *EnumNamePort(Port e)
{
    if (e < Port::PE_DATA || e > Port::LP_DATA)
        return "";
    const size_t index = static_cast<int>(e);
    return EnumNamesPort()[index];
}

enum class PEMetrics
{
    EVENT_PROC,
    EVENT_ABORT,
    EVENT_RB,
    RB_TOTAL,
    RB_PRIM,
    RB_SEC,
    FC_ATTEMPTS,
    PQ_QSIZE,
    NET_SEND,
    NET_RECV,
    NUM_GVT,
    EVENT_TIES,
    ALLRED,
    NET_READ_TIME,
    NET_OTHER_TIME,
    GVT_TIME,
    FC_TIME,
    EVENT_ABORT_TIME,
    EVENT_PROC_TIME,
    PQ_TIME,
    RB_TIME,
    CANCEL_Q_TIME,
    AVL_TIME,
    BUDDY_TIME,
    LZ4_TIME,
    NUM_METRICS,
    START = EVENT_PROC,
    END = LZ4_TIME
};

inline const char * const *EnumNamesPEMetrics()
{
    static const char * const names[] =
    {
        "event_proc",
        "event_abort",
        "event_rbs",
        "rb_total",
        "rb_primary",
        "rb_secondary",
        "fc_attempts",
        "pq_size",
        "net_send",
        "net_recv",
        "num_gvt",
        "pe_event_ties",
        "all_red_count",
        "net_read_time",
        "net_other_time",
        "gvt_time",
        "fc_time",
        "ev_abort_time",
        "ev_proc_time",
        "pq_time",
        "rb_time",
        "cancelq_time",
        "avl_time",
        "buddy_time",
        "lz4_time"
    };
    return names;
}

enum class KPMetrics
{
    EVENT_PROC,
    EVENT_ABORT,
    EVENT_RB,
    RB_TOTAL,
    RB_PRIM,
    RB_SEC,
    NET_SEND,
    NET_RECV,
    VIRT_TIME_DIFF,
    NUM_METRICS,
    START = EVENT_PROC,
    END = VIRT_TIME_DIFF
};

inline const char * const *EnumNamesKPMetrics()
{
    static const char * const names[] =
    {
        "event_proc",
        "event_abort",
        "event_rbs",
        "rb_total",
        "rb_primary",
        "rb_secondary",
        "net_send",
        "net_recv",
        "virtual_time_diff"
    };
    return names;
}

enum class LPMetrics
{
    EVENT_PROC,
    EVENT_ABORT,
    EVENT_RB,
    NET_SEND,
    NET_RECV,
    NUM_METRICS,
    START = EVENT_PROC,
    END = NET_RECV
};

inline const char * const *EnumNamesLPMetrics()
{
    static const char * const names[] =
    {
        "event_proc",
        "event_abort",
        "event_rbs",
        "net_send",
        "net_recv",
    };
    return names;
}

template <typename E>
inline const char *EnumNameMetrics(E e) {  }

template <>
inline const char *EnumNameMetrics(PEMetrics e)
{
    if (e < PEMetrics::START || e > PEMetrics::END)
        return "";
    const size_t index = static_cast<int>(e);
    return EnumNamesPEMetrics()[index];
}

template <>
inline const char *EnumNameMetrics(KPMetrics e)
{
    if (e < KPMetrics::START || e > KPMetrics::END)
        return "";
    const size_t index = static_cast<int>(e);
    return EnumNamesKPMetrics()[index];
}

template <>
inline const char *EnumNameMetrics(LPMetrics e)
{
    if (e < LPMetrics::START || e > LPMetrics::END)
        return "";
    const size_t index = static_cast<int>(e);
    return EnumNamesLPMetrics()[index];
}

// from Effective Modern C++ by Scott Meyers
// convert any enum to its underlying type
template<typename E>
constexpr typename std::underlying_type<E>::type
toUType(E enumerator) noexcept
{
    return static_cast<typename std::underlying_type<E>::type>(enumerator);
}

template <typename E>
constexpr typename std::underlying_type<E>::type
get_num_metrics()
{
    return toUType(E::NUM_METRICS);
}

} // end namespace ross_damaris
} // end namespace data

#endif // end FEATURES_H
