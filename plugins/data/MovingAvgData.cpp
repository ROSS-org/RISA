#include <plugins/data/MovingAvgData.h>
#include <math.h>

using namespace ross_damaris::data;
using namespace std;

void MovingAvgData::set_kp_metric(const char* metric)
{
    kp_avgs[metric] = make_pair(0.0, 0.0);
}

void MovingAvgData::set_lp_metric(int lpid, const char* metric)
{
    auto lp_it = lp_avgs.find(lpid);
    if (lp_it == lp_avgs.end())
    {
        MovingAvgData::metric_map m_map;
        auto rv = lp_avgs.insert(make_pair(lpid, move(m_map)));
        lp_it = rv.first;
    }

    lp_it->second.insert(make_pair(metric, make_pair(0.0, 0.0)));
}

std::pair<double, double> MovingAvgData::calc_moving_avg(double EMA_prev, double EMV_prev, double value)
{
    double delta = value - EMA_prev;
    double EMA = EMA_prev + alpha * delta;
    double EMV = (1 - alpha) * (EMV_prev + alpha * pow(delta, 2));
}

void MovingAvgData::update_kp_moving_avg(const char* metric, double value, bool init_val)
{
    if (init_val)
    {
        kp_avgs[metric].first = value;
        kp_avgs[metric].second = 0;
        return;
    }

    auto results = calc_moving_avg(kp_avgs[metric].first, kp_avgs[metric].second, value);
    kp_avgs[metric].first = results.first;
    kp_avgs[metric].second = results.second;
}

void MovingAvgData::update_lp_moving_avg(int lpid, const char* metric, double value, bool init_val)
{
    auto lp_it = lp_avgs.find(lpid);
    if (lp_it == lp_avgs.end())
        return;

    if (init_val)
    {
        lp_it->second[metric].first = value;
        lp_it->second[metric].second = 0;
        return;
    }

    double EMA_prev = lp_it->second[metric].first;
    double EMV_prev = lp_it->second[metric].second;
    auto results = calc_moving_avg(EMA_prev, EMV_prev, value);
    lp_it->second[metric].first = results.first;
    lp_it->second[metric].second = results.second;
}
