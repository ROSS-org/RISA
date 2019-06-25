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
        setup_lp_flag(lpid);
    }

    lp_it->second.insert(make_pair(metric, make_pair(0.0, 0.0)));
}

void MovingAvgData::setup_lp_flag(int lpid)
{
    flagged_lps.insert(make_pair(lpid, false));
}

void MovingAvgData::flag_lp(int lpid)
{
    if (!flagged_lps.at(lpid))
        num_flagged++;
    flagged_lps.at(lpid) = true;
}

void MovingAvgData::unflag_lp(int lpid)
{
    if (flagged_lps.at(lpid))
        num_flagged--;
    flagged_lps.at(lpid) = false;
}

bool MovingAvgData::lp_flagged(int lpid)
{
    return flagged_lps.at(lpid);
}

std::pair<double, double> MovingAvgData::calc_moving_avg(double EMA_prev, double EMV_prev, double value)
{
    //cout << "calc_moving_avg: EMA_prev " << EMA_prev << " EMV_prev " << EMV_prev << " value " << value << endl;
    double delta = value - EMA_prev;
    double EMA = EMA_prev + alpha * delta;
    double EMV = (1 - alpha) * (EMV_prev + alpha * pow(delta, 2));
    return make_pair(EMA, EMV);
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

pair<double, double> MovingAvgData::update_lp_moving_avg(int lpid, const char* metric, double value, bool init_val)
{
    //cout << "update_lp_moving_avg() lp " << lpid << " , " << metric << " = " << value << endl;
    auto lp_it = lp_avgs.find(lpid);
    if (lp_it == lp_avgs.end())
    {
        //cout << "lp_it == lp_avgs.end()\n";
        return make_pair(0.0, 0.0);
    }

    if (init_val)
    {
        //cout << "init_val\n";
        lp_it->second[metric].first = value;
        lp_it->second[metric].second = 0;
    }
    else
    {
        double EMA_prev = lp_it->second[metric].first;
        double EMV_prev = lp_it->second[metric].second;
        //cout << "EMA_prev " << EMA_prev << ", EMV_prev " << EMV_prev << endl;
        auto results = calc_moving_avg(EMA_prev, EMV_prev, value);
        //cout << "new EMA " << results.first << ", new EMV " << results.second << endl;
        lp_it->second[metric].first = results.first;
        lp_it->second[metric].second = results.second;
    }

    return make_pair(lp_it->second[metric].first, lp_it->second[metric].second);
}

pair<double, double> MovingAvgData::get_lp_moving_avg(int lpid, const char* metric)
{
    auto lp_it = lp_avgs.find(lpid);
    if (lp_it == lp_avgs.end())
        return make_pair(0.0, 0.0);

    return make_pair(lp_it->second[metric].first, lp_it->second[metric].second);
}

pair<double, double> MovingAvgData::get_kp_moving_avg(const char* metric)
{
    return make_pair(kp_avgs[metric].first, kp_avgs[metric].second);
}

void MovingAvgData::compare_lp_to_kp(const char* metric)
{
    double EMA_kp = kp_avgs[metric].first;
    double EMSD_kp = sqrt(kp_avgs[metric].second);

    for (auto it = lp_avgs.begin(); it != lp_avgs.end(); ++it)
    {
        if (lp_flagged(it->first))
            continue;

        double EMA_lp = it->second[metric].first;
        if (EMA_lp > EMA_kp + 4 * EMSD_kp)
            flag_lp(it->first);
    }
}
