#include <filesystem>
#include <fstream>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
#include <variant>

#include "xcl/xcl.h"
namespace xcl {

  Section::Section() {
  }
  Section::Section(Section&& other) {
    read_lock rl(other._m);
    this->_full_path = std::move(other._full_path);
    this->_name = std::move(other._name);
    this->kv = std::move(other.kv);
    this->sections = std::move(other.sections);
  }
  Section::Section(std::string_view full_path, std::string& name, std::ifstream& ifs)
    : _full_path(full_path) {
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
    , _name(name) {
  }
  Section& Section::operator=(Section&& other) {
    read_lock rl(other._m);
    _full_path = std::move(other._full_path);
    _name = std::move(other._name);
    kv = std::move(other.kv);
    sections = std::move(other.sections);
    return *this;
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
        auto eseq = next.find('=');
        if(eseq == std::string::npos)
          continue;
        std::string key;
        if(auto tmp = next.find(' '); tmp < eseq) {
          key = next.substr(0, tmp);
        } else {
          key = next.substr(0, eseq);
        }
        auto vtstart = next.find_first_not_of(' ', eseq + 1);
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
        write_lock wl(this->_m);
        this->kv.emplace(std::move(key), std::move(value));
      }
      next.clear();
    }
    return next;
  }

  void Section::prase(std::string& next, std::ifstream& ifs) {
    auto [seq, name, sec] = prase_path(next);
    if(sec == sections.end()) {
      write_lock wl(this->_m);
      this->sections.emplace(std::move(name), Section{this->get_full_name(), next, ifs});
    } else {
      if(seq == std::string::npos) {
        next = sec->second.prase_kv(ifs);
      } else {
        next.erase(0, seq + 1);
        sec->second.prase(next, ifs);
      }
    }
  }
  Section::~Section() {
  }
  std::tuple<std::string_view::size_type, std::string, std::unordered_map<std::string, Section>::iterator>
  Section::prase_path(std::string_view path) {
    auto seq = path.find('\'');
    auto name = std::string(path.substr(0, seq));
    read_lock rl(this->_m);
    return {seq, name, sections.find(name)};
  }
  void Section::set_name(std::string_view name) {
    write_lock wl(this->_m);
    this->_name = name;
  }

  std::string_view Section::get_name() const {
    read_lock rl(this->_m);
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

  template <typename T>
  std::optional<T> Section::find(std::string_view name) const {
    read_lock rl(this->_m);
    auto kv = this->kv.find(name.data());
    if(kv == this->kv.end()) {
      return std::nullopt;
    } else {
      return std::get<T>(kv->second);
    }
  }
  template <typename T>
  decltype(auto) Section::find(const char name[]) const {
    return this->find<T>(std::string_view(name));
  }

  std::pair<std::reference_wrapper<Section>, bool> Section::try_insert(std::string_view path) {
    auto [seq, name, sec] = prase_path(path);  //
    if(sec == this->sections.end()) {
      write_lock wl(this->_m);
      auto [it, ok] = this->sections.emplace(name, Section{this->get_full_name(), name});
      wl.release();
      if(seq == std::string::npos) {
        return {it->second, ok};
      } else {
        sec = it;
      }
    } else if(seq == std::string::npos) {
      return {sec->second, false};
    }
    return sec->second.try_insert(path.substr(seq + 1));
  }
  auto Section::try_insert(const char *sec) -> decltype(try_insert(std::string_view())) {
    return this->try_insert(std::string_view(sec));
  }

  std::ostream& operator<<(std::ostream& os, const Section& sec) {
    Section::read_lock rl(sec._m);
    if(!sec._name.empty()) {
      os << "[" << sec.get_full_name() << "]" << std::endl;
    }
    for(auto& [key, value] : sec.kv) {
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
    rl.release();
    for(auto& [key, value] : sec.sections) {
      os << value;
    }
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

  Xcl::Xcl(std::string_view path) {
    this->_full_path = std::filesystem::absolute(path);
    if(std::filesystem::exists(this->_full_path)) {
      // recursive_create(std::filesystem::path(path).parent_path());
      std::ifstream ifs(this->_full_path);
      std::string next = this->prase_kv(ifs);
      while(!next.empty()) {
        this->prase(next, ifs);
      }
      ifs.close();
    }
  }
  Xcl::~Xcl() {
    // std::ofstream ofs(this->get_name());
  }
  void Xcl::save() {
    std::ofstream ofs(this->_full_path);
    ofs << *this;
    ofs.close();
  }
}  // namespace xcl
