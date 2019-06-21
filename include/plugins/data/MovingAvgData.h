#ifndef MOVING_AVG_DATA_H
#define MOVING_AVG_DATA_H

#include <map>
#include <iostream>

namespace ross_damaris {
namespace data {

class MovingAvgData
{
public:
    typedef std::map<std::string, std::pair<double, double>> metric_map;

    MovingAvgData(int peid, int gid, int lid) :
        peid(peid),
        kp_gid(gid),
        kp_lid(lid),
        alpha(0.2),
        num_flagged(0) {  }

    int get_peid() { return peid; }
    int get_kp_gid() { return kp_gid; }
    int get_kp_lid() { return kp_lid; }

    void set_kp_metric(const char* metric);
    void update_kp_moving_avg(const char* metric, double value, bool init_val);
    std::pair<double, double> get_kp_moving_avg(const char* metric);
    void compare_lp_to_kp(const char* metric);

    void set_lp_metric(int lpid, const char* metric);
    std::pair<double, double> update_lp_moving_avg(int lpid, const char* metric, double value, bool init_val);
    std::pair<double, double> get_lp_moving_avg(int lpid, const char* metric);
    void setup_lp_flag(int lpid);
    void flag_lp(int lpid);
    void unflag_lp(int lpid);
    bool lp_flagged(int lpid);

    std::pair<double, double> calc_moving_avg(double EMA_prev, double EMV_prev, double value);
    int get_num_flagged() { return num_flagged; }

    std::map<int, bool> flagged_lps;
private:
    int peid;
    int kp_gid;
    int kp_lid;
    double alpha;
    int num_flagged;

    metric_map kp_avgs;
    std::map<int, metric_map> lp_avgs;
    //sim_config->lp_map map of int -> <int, int>
    // ie lpid -> peid, kp lid
};

} // end namespace ross_damaris
} // end namespace data

#endif // MOVING_AVG_DATA_H
