#include <algorithm>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

#include "xcl/xcl.h"
namespace xcl {

  Section::Section()
    : _full_path()
    , _name()
    , _kvs()
    , sections()
    , _update_flag() {
  }
  Section::Section(std::string_view full_path, std::string& name, std::ifstream& ifs)
    : _full_path(full_path)
    , _name()
    , _kvs()
    , sections()
    , _update_flag() {
    auto dot = name.find('\'');
    if(dot != std::string::npos) {
      this->set_name(name.substr(0, dot));
      name = name.substr(dot + 1);
      auto sub = Section{this->get_full_name(), name, ifs};
      this->sections.emplace(sub.get_name(), std::move(sub));
      return;
    }
    this->set_name(name);
    name = prase_kv(ifs);
  }
  Section::Section(std::string_view full_path, std::string_view name)
    : _full_path(full_path)
    , _name(name)
    , _kvs()
    , sections()
    , _update_flag() {
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
        auto kstart = next.find_first_not_of(' ');
        if(kstart == std::string::npos)
          continue;
        auto eq = next.find('=', kstart + 1);
        if(eq == std::string::npos)
          continue;
        auto kend = next.find_last_not_of(' ', eq - 1);
        if(kend == std::string::npos)
          continue;
        auto vtstart = next.find_first_not_of(' ', eq + 1);
        if(vtstart == std::string::npos || next.length() - vtstart < 1
           || (next[vtstart + 1] != '"' && next[vtstart + 1] != '\''))  // at least 2 characters, e.g. s"
          continue;
        value_type value;
        auto vtv = std::string_view(next);
        switch(vtv[vtstart]) {
          case 'i' :
            value = std::stol(next.substr(vtstart + 2));
            break;
          case 'u' :
            value = std::stoul(next.substr(vtstart + 2));
            break;
          case 'f' :
            value = std::stof(next.substr(vtstart + 2));
            break;
          case 'd' :
            value = std::stod(next.substr(vtstart + 2));
            break;
          case 's' :
            value = next.substr(vtstart + 2);
            break;
          default :
            break;
        }
        this->_kvs.emplace(next.substr(kstart, kend + 1 - kstart), std::move(value));
        this->_update_flag = false;
      }
      next.clear();
    }
    return next;
  }

  void Section::prase(std::string& next, std::ifstream& ifs) {
    auto [seq, name, sec] = prase_path(next);
    if(sec == sections.end()) {
      this->sections.emplace(std::move(name), Section{this->get_full_name(), next, ifs});
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
    return !this->_update_flag || std::any_of(this->sections.begin(), this->sections.end(), [](const auto& sec) {
      return sec.second.need_update();
    });
  }

  Section::~Section() {
  }
  std::tuple<std::string_view::size_type, std::string, std::unordered_map<std::string, Section>::iterator>
  Section::prase_path(std::string_view path) {
    auto seq = path.find('\'');
    auto name = std::string(path.substr(0, seq));
    return {seq, name, sections.find(name)};
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
    if(sec == this->sections.end()) {
      return std::nullopt;
    } else if(seq == std::string::npos) {
      return sec->second;
    } else {
      return sec->second.find(path.substr(seq + 1));
    }
  }

  void Section::clear() {
    this->_kvs.clear();
    this->sections.clear();
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
    for(auto& [key, value] : sec.sections) {
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
    : Section()
    , _full_path()
    , _last_write_time() {
    try {
      this->_full_path = std::filesystem::absolute(path);
    } catch(...) {
      return;
    }
    auto status = std::filesystem::status(this->_full_path);
    if(status.type() == std::filesystem::file_type::regular || status.type() == std::filesystem::file_type::symlink) {
      // recursive_create(std::filesystem::path(path).parent_path());
      this->_last_write_time = std::filesystem::last_write_time(this->_full_path);
      this->prase_file();
    }
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
      }
    }
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
