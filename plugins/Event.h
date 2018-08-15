#ifndef EVENT_H
#define EVENT_H

#include <cstdlib>
#include <iostream>

namespace opt_debug{

class Event
{
    private:

    int src_lp;
    int dest_lp;
    float send_ts;
    float recv_ts;
    float gvt; // gvt when this event was committed
    long *rng_count;
    long *prev_rng_count;

    int sync_type;
    int damaris_iteration;
    std::string event_name;


    public:

    // constructor
    Event(int src, int dest, float send, float recv, int num_rng);

    // destructor
    ~Event() {}

    virtual int get_src_lp() const { return src_lp; }
    virtual int get_dest_lp() const { return dest_lp; }
    virtual float get_send_ts() const { return send_ts; }
    virtual float get_recv_ts() const { return recv_ts; }

    virtual void set_gvt(float cur_gvt) { gvt = cur_gvt; }
    virtual float get_gvt() const { return gvt; }

    virtual void set_sync_type(int sync) { sync_type = sync; }
    virtual int get_sync_type() const { return sync_type; }

    virtual void set_damaris_iteration(int i) { damaris_iteration = i; }
    virtual int get_damaris_iteration() const { return damaris_iteration; }

    virtual void set_event_name(std::string& name) { event_name = name; }
    virtual std::string get_event_name() const { return event_name; }

    virtual void set_rng_count(int pos, long count) { rng_count[pos] = count; }
    virtual long get_rng_count(int pos) { return rng_count[pos]; }
    virtual void set_prev_rng_count(int pos, long count) { prev_rng_count[pos] = count; }
    virtual long get_prev_rng_count(int pos) { return prev_rng_count[pos]; }
};


}

#endif // EVENT_H
