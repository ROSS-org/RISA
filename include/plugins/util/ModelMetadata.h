#ifndef MODEL_METADATA_H
#define MODEL_METADATA_H

#include <unordered_map>

namespace ross_damaris {
namespace util {

class ModelLPMetadata {
public:
    ModelLPMetadata(int peid, int kpid, int lpid, std::string name) :
        peid_(peid),
        kpid_(kpid),
        lpid_(lpid),
        name_(name) {  }

    virtual int get_peid() const { return peid_; }
    virtual int get_kpid() const { return kpid_; }
    virtual int get_lpid() const { return lpid_; }
    virtual std::string get_name() const { return name_; }

    void add_variable(std::string var_name, int id)
    {
        var_map_.insert(std::make_pair(id, var_name));
    }

    std::string get_var_name(int id)
    {
        return var_map_[id];
    }

private:
    int peid_;
    int kpid_;
    int lpid_;
    std::string name_;
    std::unordered_map<int, std::string> var_map_;

};

} // end namespace util
} // end namespace ross_damaris

#endif // MODEL_METADATA_H
