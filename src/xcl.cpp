#include "xcl/xcl.h"
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
namespace xcl {
Section::Section() {}

Section::Section(std::string &name, std::ifstream &ifs) {
  auto dot = name.find('.');
  if (dot != std::string::npos) {
    this->set_name(name.substr(0, dot));
    name = name.substr(dot + 1);
    auto sub = Section{name, ifs};
    this->sections.emplace(sub.get_name(), std::move(sub));
    return;
  }
  this->set_name(name);
  name = prase(ifs);
}
std::string Section::prase(std::ifstream &ifs) {
  std::string next;
  while (std::getline(ifs, next)) {
    if (next.empty())
      continue;
    if (next[0] == '[') {
      next = next.substr(1, next.size() - 2);
      break;
    } else {
      auto eseq = next.find('=');
      if (eseq == std::string::npos)
        break;
      std::string key;
      if (auto tmp = next.find(' '); tmp < eseq) {
        key = next.substr(0, tmp);
      } else {
        key = next.substr(0, eseq);
      }
      auto vtstart = next.find_first_not_of(' ', eseq + 1);
      if (vtstart == std::string::npos || next.length() - vtstart < 1 ||
          (next[vtstart + 1] != '"' &&
           next[vtstart + 1] != '\'')) // at least 2 characters, e.g. s"
        break;
      std::variant<std::string, long, unsigned long, float, double> value;
      auto vtv = std::string_view(next);
      switch (vtv[vtstart]) {
      case 'i':
        value = std::stol(next.substr(vtstart + 2));
        break;
      case 'u':
        value = std::stoul(next.substr(vtstart + 2));
        break;
      case 'f':
        value = std::stof(next.substr(vtstart + 2));
        break;
      case 'd':
        value = std::stod(next.substr(vtstart + 2));
        break;
      case 's':
        value = next.substr(vtstart + 2);
        break;
      default:
        break;
      }
      this->kv.emplace(std::move(key), std::move(value));
    }
  }
  return next;
}

void Section::insert(std::string &next, std::ifstream &ifs) {
  if (next.empty()) {
    next = prase(ifs);
    return;
  }
  next.erase(0, 1);
  auto seq = next.find('.');
  auto name = next.substr(0, seq);
  auto sec = sections.find(name);
  if (sec == sections.end()) {
    this->sections.emplace(name, Section(next, ifs));
  } else {
    next = next.substr(seq + 1);
    sec->second.insert(next, ifs);
  }
}
Section::~Section() {}

void Section::set_name(std::string_view name) { this->_name = name; }

std::string_view Section::get_name() const { return _name; }

Xcl::Xcl() {}

Xcl::Xcl(std::string_view path) {
  std::ifstream ifs(path.data());
  std::string next = "main";
  this->top = Section(next, ifs);
  while (!next.empty()) {
    auto seq = next.find('.');
    auto name = next.substr(0, seq);
    auto sec = sections.find(name);
    if (sec == sections.end()) {
      this->sections.emplace(name, Section(next, ifs));
    } else {
      next.erase(0, seq);
      sec->second.insert(next, ifs);
    }
  }
}
Xcl::~Xcl() {}
} // namespace xcl
