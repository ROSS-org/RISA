#include "Event.h"
#include <iostream>

using namespace std;
namespace opt_debug {

Event::Event(int src, int dest, float send, float recv, int num_rng)
{
    src_lp = src;
    dest_lp = dest;
    send_ts = send;
    recv_ts = recv;

    rng_count_before = (long*)calloc(num_rng, sizeof(long));
    rng_count_end = (long*)calloc(num_rng, sizeof(long));
}

void Event::print_event_data(const std::string& varname)
{
    cout << varname << ": (" << source_pe << ", " <<event_id << ") ("
        << src_lp << ", " << dest_lp << ", " << send_ts << ", " << recv_ts << ") "
        << rng_count_before[0] << ", " << rng_count_end[0] << " event_name: " << event_name << endl;
}

}
