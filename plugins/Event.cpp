#include "Event.h"

namespace opt_debug {

Event::Event(int src, int dest, float send, float recv)
{
    src_lp = src;
    dest_lp = dest;
    send_ts = send;
    recv_ts = recv;
}

}
