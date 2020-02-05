// Copyright (c) 2019 fortiss GmbH, Julian Bernhard, Klemens Esterle, Patrick Hart, Tobias Kessler
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.


#ifndef MODULES_COMMONS_PARAMS_SETTER_PARAMS_HPP_
#define MODULES_COMMONS_PARAMS_SETTER_PARAMS_HPP_

#include <string>
#include <unordered_map>
#include "modules/commons/params/params.hpp"
#include "modules/commons/util/util.hpp"
namespace modules {
namespace commons {

// This class is mainly useful for test definitions in C++
class SetterParams : public Params {
 public:
  SetterParams(bool log_if_default=false) : params_bool_(), params_real_(), params_int_(),
                         params_listlist_float_(), log_if_default_(log_if_default) {}

  virtual ~SetterParams() {}

  // get and set parameters as in python
  virtual bool GetBool(const std::string &param_name,
                        const std::string &description,
                        const bool &default_value) { return get_parameter(params_bool_, param_name, default_value);}
                        
  virtual float GetReal(const std::string &param_name,
                         const std::string &description,
                         const float &default_value)  { return get_parameter(params_real_, param_name, default_value);}

  virtual int GetInt(const std::string &param_name,
                      const std::string &description,
                      const int &default_value)  { return get_parameter(params_int_, param_name, default_value);}

  virtual std::vector<std::vector<float>> GetListListFloat(const std::string &param_name,
                      const std::string &description,
                      const std::vector<std::vector<float>> &default_value)  { return get_parameter(params_listlist_float_, param_name, default_value);}

  // not used atm
  virtual void SetBool(const std::string &param_name, const bool &value) {params_bool_[param_name]=value;}
  virtual void SetReal(const std::string &param_name, const float &value) {params_real_[param_name]=value;}
  virtual void SetInt(const std::string &param_name, const int &value) {params_int_[param_name]=value;}
  virtual void SetListListFloat(const std::string &param_name,
                      const std::vector<std::vector<float>> &value) {params_listlist_float_[param_name]=value;}

  virtual int operator[](const std::string &param_name) { throw; } //< not supported atm 

  virtual Params *AddChild(const std::string &name) { throw; } //< not supported atm

  private: 
    template<typename M, typename T>
    T get_parameter(M map, std::string param_name, T default_value) {
      const auto it = map.find(param_name);
      if(it != map.end()) {
        return it->second;
      } else {
        if(log_if_default_) {
          LOG(INFO) << "Using default " << default_value <<" for param \"" << param_name << "\"";
        }
        return default_value;
      }
    }
    std::unordered_map<std::string, bool> params_bool_;
    std::unordered_map<std::string, float> params_real_;
    std::unordered_map<std::string, int> params_int_;
    std::unordered_map<std::string, std::vector<std::vector<float>>> params_listlist_float_;

    bool log_if_default_;

};

}  // namespace commons
}  // namespace modules

#endif  // MODULES_COMMONS_PARAMS_SETTER_PARAMS_HPP_
