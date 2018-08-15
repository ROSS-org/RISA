#include "Event.h"

namespace opt_debug {

Event::Event(int src, int dest, float send, float recv, int num_rng)
{
    src_lp = src;
    dest_lp = dest;
    send_ts = send;
    recv_ts = recv;

    rng_count = (long*)calloc(num_rng, sizeof(long));
}

}
