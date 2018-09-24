#include <plugins/flatbuffers/data_sample_generated.h>

namespace ross_damaris {
namespace util {

namespace fb = flatbuffers;
namespace rds = ross_damaris::sample;

class ModelFlatBuffer {
    private:
        fb::FlatBufferBuilder fbb_;
        std::vector<fb::Offset<rds::ModelLP>> model_lps_;
        std::vector<fb::Offset<rds::ModelVariable>> cur_lp_vars_;
        std::string cur_lp_type_;
        int cur_lpid_;
        double virtual_ts_;
        double real_ts_;
        double gvt_;
        rds::InstMode mode_;

        template <typename T>
        rds::VariableType get_var_value_type() { return rds::VariableType_NONE; }

    public:
        ModelFlatBuffer() :
            fbb_(),
            cur_lp_type_(""),
            cur_lpid_(-1),
            virtual_ts_(0.0),
            real_ts_(0.0),
            gvt_(0.0),
            mode_(rds::InstMode_GVT) { }

        void set_cur_lp_type(const std::string& lp_type) { cur_lp_type_ = lp_type; }
        void set_cur_lp_type(const char* lp_type) { cur_lp_type_ = lp_type; }
        std::string get_cur_lp_type() { return cur_lp_type_; }

        void set_cur_lpid(int id) { cur_lpid_ = id; }
        int get_cur_lpid() { return cur_lpid_; }

        void start_lp_sample(int lpid);
        void finish_lp_sample();
        void start_sample(double vts, double rts, double gvt, rds::InstMode mode);
        void finish_sample();

        template <typename T>
        fb::Offset<void> create_data_type_holder(fb::Offset<fb::Vector<T>> data_vec) {  }

        template <typename T>
        void save_model_variable(int lpid, const char* lp_type, const char* var_name, T *data, size_t num_elements)
        {
            //if (lpid != cur_lpid_)
            //    std::cout << "ERROR in save_model_variable!\n";
            //cur_lpid_ = lpid;
            cur_lp_type_ = lp_type;

            auto name = fbb_.CreateString(var_name);
            auto data_vec = fbb_.CreateVector(data, num_elements);
            auto var_union = create_data_type_holder(data_vec);
            auto variable = rds::CreateModelVariable(fbb_, name, get_var_value_type<T>(), var_union);
            cur_lp_vars_.push_back(variable);
        }

};



} // end namespace util
} // end namespace ross_damaris
