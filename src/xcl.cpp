#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include "xcl/xcl.h"

namespace xcl {

  Section::Section() {
  }
  Section::Section(std::string_view full_path, std::string& name, std::ifstream& ifs)
    : _full_path(full_path) {
    auto dot = name.find('\'');
    if(dot != std::string::npos) {
      this->set_name(name.substr(0, dot));
      name = name.substr(dot + 1);
      auto sub = Section{this->get_full_name(), name, ifs};
      this->_sections.emplace(sub.get_name(), std::move(sub));
      return;
    }
    this->set_name(name);
    name = prase_kv(ifs);
  }
  Section::Section(std::string_view full_path, std::string_view name)
    : _full_path(full_path)
    , _name(name) {
  }
  std::string Section::prase_kv(std::ifstream& ifs) {
    std::string next;
    while(std::getline(ifs, next)) {
      if(next.empty())
        continue;
      if(next[0] == '[') {
        next = next.substr(1, next.size() - 2);
        break;
      } else {
        auto eq = next.find('=');
        if(eq == std::string::npos)
          continue;
        auto key = this->prase_sym(next.substr(0, eq));
        if(!key.has_value() || !std::holds_alternative<std::string>(*key))
          continue;
        auto value = this->prase_sym(next.substr(eq + 1));
        if(!value.has_value())
          continue;
        this->_kvs.emplace(std::get<std::string>(*key), *value);
        this->_update_flag = false;
      }
      next.clear();
    }
    return next;
  }
  std::optional<Section::sym_type> Section::prase_sym(std::string str) {
    if(str.empty()) {
      return std::nullopt;
    }
    auto ss = str.find_first_not_of(' ');
    if(ss == std::string::npos || ss == str.size() - 1 || str[ss] == '\'') {
      return std::nullopt;
    }
    auto se = str.find_last_not_of(' ', str.size() - 1);
    auto seq = str.find('\'', ss + 1);
    if(seq == std::string::npos) {
      return std::string(str.substr(ss, se + 1 - ss));
    }
    if(seq != ss + 1) {
      return std::nullopt;
    }
    auto sym = std::string(str.substr(seq + 1, se - seq));
    switch(str[ss]) {
      case 'i' :
        return std::stol(sym);
      case 'u' :
        return std::stoul(sym);
      case 'f' :
        return std::stof(sym);
      case 'd' :
        return std::stod(sym);
      case 's' :
        return sym;
      default :
        return std::nullopt;
    }
  }

  void Section::prase(std::string& next, std::ifstream& ifs) {
    auto [seq, name, sec] = prase_path(next);
    if(sec == _sections.end()) {
      this->_sections.emplace(std::move(name), Section{this->get_full_name(), next, ifs});
      this->_update_flag = false;
    } else {
      if(seq == std::string::npos) {
        next = sec->second.prase_kv(ifs);
      } else {
        next.erase(0, seq + 1);
        sec->second.prase(next, ifs);
      }
    }
  }

  bool Section::need_update() const {
    return !this->_update_flag || std::any_of(this->_sections.begin(), this->_sections.end(), [](const auto& sec) {
      return sec.second.need_update();
    });
  }

  Section::~Section() {
  }
  std::tuple<std::string_view::size_type, std::string, std::unordered_map<std::string, Section>::iterator>
  Section::prase_path(std::string_view path) {
    auto seq = path.find('\'');
    auto name = std::string(path.substr(0, seq));
    return {seq, name, _sections.find(name)};
  }
  std::tuple<std::string_view::size_type, std::string, std::unordered_map<std::string, Section>::const_iterator>
  Section::prase_path(std::string_view path) const {
    auto seq = path.find('\'');
    auto name = std::string(path.substr(0, seq));
    return {seq, name, _sections.find(name)};
  }
  void Section::set_name(std::string_view name) {
    this->_name = name;
    this->_update_flag = false;
  }

  std::string_view Section::get_name() const {
    return this->_name;
  }
  std::string Section::get_full_name() const {
    return this->_full_path.empty() ? this->_name : this->_full_path + '\'' + this->_name;
  }

  auto Section::find(const char name[]) -> decltype(find(std::string_view())) {
    return this->find(std::string_view(name));
  }
  std::optional<std::reference_wrapper<Section>> Section::find(std::string_view path) {
    auto [seq, _, sec] = prase_path(path);
    if(sec == this->_sections.end()) {
      return std::nullopt;
    } else if(seq == std::string::npos) {
      return sec->second;
    } else {
      return sec->second.find(path.substr(seq + 1));
    }
  }
  auto Section::find(const char name[]) const -> decltype(find(std::string_view())) {
    return this->find(std::string_view(name));
  }
  std::optional<std::reference_wrapper<const Section>> Section::find(std::string_view path) const {
    auto [seq, _, sec] = prase_path(path);
    if(sec == this->_sections.end()) {
      return std::nullopt;
    } else if(seq == std::string::npos) {
      return sec->second;
    } else {
      return sec->second.find(path.substr(seq + 1));
    }
  }
  void Section::clear() {
    if(this->_kvs.empty() && this->_sections.empty()) {
      return;
    }
    this->_kvs.clear();
    this->_sections.clear();
    this->_update_flag = false;
  }
  std::ostream& operator<<(std::ostream& os, const Section& sec) {
    if(!sec._name.empty()) {
      os << "[" << sec.get_full_name() << "]" << std::endl;
    }
    for(auto& [key, value] : sec._kvs) {
      os << key << " = ";
      switch(value.index()) {
        case 0 :
          os << "s'" << std::get<std::string>(value);
          break;
        case 1 :
          os << "i'" << std::get<long>(value);
          break;
        case 2 :
          os << "u'" << std::get<unsigned long>(value);
          break;
        case 3 :
          os << "f'" << std::get<float>(value);
          break;
        case 4 :
          os << "d'" << std::get<double>(value);
          break;
        default :
          break;
      }
      os << std::endl;
    }
    os << std::endl;
    for(auto& [key, value] : sec._sections) {
      os << value;
    }
    sec._update_flag = true;
    return os;
  }
  Section& Section::operator>>(const char *path) {
    auto p = std::filesystem::path(path);
    if(!std::filesystem::exists(p)) {
      recursive_create(p.parent_path());
    }
    std::ofstream ofs(path);
    ofs << *this;
    return *this;
  }
  // Section& Section::operator<<(const char *path) {
  //   auto p = std::filesystem::path(path);
  //   if(!std::filesystem::exists(p)) {
  //     recursive_create(p.parent_path());
  //   }
  //   std::ofstream ofs(path);
  //   ofs << *this;
  //   return *this;
  // }
  void Section::recursive_create(std::filesystem::path abs_path) {
    if(abs_path.empty()) {
      return;
    }
    if(!std::filesystem::exists(abs_path.parent_path())) {
      this->recursive_create(abs_path.parent_path());
    }
    std::filesystem::create_directory(abs_path);
  }
  Xcl::Xcl() {
  }

  Xcl::Xcl(std::string_view path)
    : Section() {
    this->load(path);
  }
  Xcl::~Xcl() {
    // std::ofstream ofs(this->get_name());
  }
  void Xcl::save(bool force) {
    if(force || this->need_update()) {
      if(auto p = this->_full_path.parent_path(); !std::filesystem::exists(p)) {
        this->recursive_create(p);
      }
      if(auto status = std::filesystem::status(this->_full_path);
         status.type() == std::filesystem::file_type::regular || status.type() == std::filesystem::file_type::symlink
         || status.type() == std::filesystem::file_type::not_found) {
        std::ofstream ofs(this->_full_path);
        ofs << *this;
        ofs.close();
      }
    }
  }
  void Xcl::reload(bool force) {
    if(auto status = std::filesystem::status(this->_full_path);
       status.type() == std::filesystem::file_type::regular || status.type() == std::filesystem::file_type::symlink) {
      auto last_write_time = std::filesystem::last_write_time(this->_full_path);
      if(force || last_write_time != this->_last_write_time) {
        this->_last_write_time = last_write_time;
        this->clear();
        this->prase_file();
        this->_update_flag = true;
      }
    }
  }
  void Xcl::load(std::filesystem::path path) {
    try {
      this->_full_path = std::filesystem::absolute(path);
    } catch(...) {
      return;
    }
    this->reload(true);
  }
  bool Xcl::prase_file() {
    std::ifstream ifs(this->_full_path);
    if(!ifs.is_open()) {
      return false;
    }
    std::string next = this->prase_kv(ifs);
    while(!next.empty()) {
      this->prase(next, ifs);
    }
    ifs.close();
    return true;
  }
}  // namespace xcl
