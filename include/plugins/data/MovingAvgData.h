#ifndef MOVING_AVG_DATA_H
#define MOVING_AVG_DATA_H

#include <map>
#include <iostream>

namespace ross_damaris {
namespace data {

class MovingAvgData
{
public:
    int peid;
    int kp_gid;
    int kp_lid;

    typedef std::map<std::string, std::pair<double, double>> metric_map;

    metric_map kp_avgs;
    std::map<int, metric_map> lp_avgs;
    //sim_config->lp_map map of int -> <int, int>
    // ie lpid -> peid, kp lid

    MovingAvgData(int peid, int gid, int lid) :
        peid(peid),
        kp_gid(gid),
        kp_lid(lid) {  }
};

} // end namespace ross_damaris
} // end namespace data

#endif // MOVING_AVG_DATA_H
