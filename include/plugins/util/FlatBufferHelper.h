#include <plugins/flatbuffers/data_sample_generated.h>
#include <iostream>

namespace ross_damaris {
namespace util {

namespace fb = flatbuffers;
namespace rds = ross_damaris::sample;

class FlatBufferHelper {
public:
    FlatBufferHelper() :
        fbb_(),
        cur_lp_type_(""),
        cur_lpid_(-1),
        virtual_ts_(0.0),
        real_ts_(0.0),
        gvt_(0.0),
        mode_(rds::InstMode_GVT),
        entity_id_(-1),
        event_id_(-1),
        max_sample_size_(524288),
        rt_block_(0),
        vt_block_(0) { }

    void start_sample(double vts, double rts, double gvt, rds::InstMode mode);
    void start_sample(double vts, double rts, double gvt, rds::InstMode mode,
            int entity_id, int event_id);
    uint8_t* finish_sample(size_t* offset);

    void pe_sample(tw_pe *pe, tw_statistics *s, tw_statistics *last_pe_stats, int inst_type);
    void kp_sample(tw_kp *kp, int inst_type);
    void lp_sample(tw_lp *lp, int inst_type);

    void set_cur_lp_type(const std::string& lp_type) { cur_lp_type_ = lp_type; }
    void set_cur_lp_type(const char* lp_type) { cur_lp_type_ = lp_type; }
    std::string get_cur_lp_type() { return cur_lp_type_; }

    void set_cur_lpid(int id) { cur_lpid_ = id; }
    int get_cur_lpid() { return cur_lpid_; }

    void start_lp_model_sample(int lpid);
    void finish_lp_model_sample();

    void set_rc_data(uint8_t *raw, size_t offset);
    void delete_rc_data();

    void reset_block_counters();

    template <typename T>
    const T* save_model_variable(int lpid, const char* lp_type, const char* var_name, T *data, size_t num_elements)
    {
        cur_lp_type_ = lp_type;

        //TODO use CreateSharedString?
        auto name = fbb_.CreateSharedString(var_name);
        auto data_vec = fbb_.CreateVector(data, num_elements);
        auto var_union = get_variable_offset(data_vec);
        auto variable = rds::CreateModelVariable(fbb_, name, get_var_value_type<T>(),
                var_union);
        cur_lp_vars_.push_back(variable);
    }

    template <typename T>
    const T* get_model_variable(int lpid, const char* lp_type, const char* var_name,
            size_t *num_elements)
    {
        // Note: In general, there should be a reasonable number of LPs per
        // KP (and thus Analysis LP), as well as a small number of model variables,
        // so even though this seems costly, in practice, it should be okay.
        auto model_lps = cur_rc_fb_->model_data();
        auto num_model_lps = model_lps->Length();
        for (int i = 0; i < num_model_lps; i++)
        {
            if (lpid == model_lps->Get(i)->lpid())
            {
                auto vars = model_lps->Get(i)->variables();
                auto num_vars = vars->Length();
                for (int j = 0; j < num_vars; j++)
                {
                    if (strcmp(var_name, vars->Get(j)->var_name()->c_str()) == 0)
                        return get_variable_data<T>(vars->Get(j), num_elements);
                }
            }
        }

    }

private:
    fb::FlatBufferBuilder fbb_;
    std::vector<fb::Offset<rds::PEData>> pes_;
    std::vector<fb::Offset<rds::KPData>> kps_;
    std::vector<fb::Offset<rds::LPData>> lps_;

    std::vector<fb::Offset<rds::ModelLP>> model_lps_;
    std::vector<fb::Offset<rds::ModelVariable>> cur_lp_vars_;
    std::string cur_lp_type_;
    int cur_lpid_;

    const sample::DamarisDataSample* cur_rc_fb_;
    uint8_t *cur_rc_raw_;

    double virtual_ts_;
    double real_ts_;
    double gvt_;
    rds::InstMode mode_;
    int entity_id_;
    int event_id_;
    int max_sample_size_;
    int rt_block_;
    int vt_block_;

    template <typename T>
    rds::VariableType get_var_value_type() { return rds::VariableType_NONE; }

    template <typename T>
    fb::Offset<void> get_variable_offset(fb::Offset<fb::Vector<T>> data_vec) {  }

    template <typename T>
    const T* get_variable_data(const sample::ModelVariable *v, size_t *num_elements)
    { std::cout << "didn't get correct template\n"; return nullptr; }

};



} // end namespace util
} // end namespace ross_damaris
