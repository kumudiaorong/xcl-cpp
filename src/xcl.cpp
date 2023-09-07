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

Section::Section(std::string_view name, std::ifstream &ifs, std::string &next) {
  this->set_name(name);
  std::string line;
  while (std::getline(ifs, line)) {
    if (line.empty())
      continue;
    if (line[0] == '[') {
      next = line.substr(1, line.size() - 2);
      break;
    } else {
      auto eseq = line.find('=');
      if (eseq == std::string::npos)
        break;
      std::string key;
      if (auto tmp = line.find(' '); tmp < eseq) {
        key = line.substr(0, tmp);
      } else {
        key = line.substr(0, eseq);
      }
      auto vtstart = line.find_first_not_of(' ', eseq + 1);
      if (vtstart == std::string::npos ||
          line.length() - vtstart < 2) // at least 3 characters, e.g. s""
        break;
      std::variant<std::string, int8_t, int16_t, int32_t, int64_t, uint8_t,
                   uint16_t, uint32_t, uint64_t, float, double>
          value;
      auto vtv = std::string_view(line);
      auto vstart = vtv.find_first_of("\'\"", vtstart);
      if (vstart == vtstart) {
        continue;
      }
      if (vstart - vtstart != 1) {
        continue;
      }
      auto vend = vtv.find(vtv[vstart], vstart + 1);
      ++vstart;
      switch (vtv[vtstart]) {
      case 'i':
        value = std::stol(line.substr(vstart, vend - vstart));
        break;
      case 'u':
        value = std::stoul(line.substr(vstart, vend - vstart));
        break;
      case 'f':
        value = std::stof(line.substr(vstart, vend - vstart));
        break;
      case 'd':
        value = std::stod(line.substr(vstart, vend - vstart));
        break;
      case 's':
        value = line.substr(vstart, vend - vstart);
        break;
      default:
        break;
      }
      this->kv.emplace(std::move(key), std::move(value));
    }
  }
}
Section::~Section() {}

void Section::set_name(std::string_view name) { this->_name = name; }

std::string_view Section::get_name() const { return _name; }

Xcl::Xcl() {}

Xcl::Xcl(std::string_view path) {
  std::ifstream ifs(path.data());
  std::string next{};
  this->top = Section("main", ifs, next);
//   while (!next.empty()) {

//   }
}
Xcl::~Xcl() {}
} // namespace xcl
