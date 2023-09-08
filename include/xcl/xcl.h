#ifndef XSL_XCL_H
#define XSL_XCL_H
#include <fstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
namespace xcl {
class Section {
public:
  std::string _name;
  std::unordered_map<std::string, std::variant<std::string, long, unsigned long,
                                               float, double>>
      kv;
  std::unordered_map<std::string, Section> sections;
  Section();
  Section(Section &&other) = default;
  Section(std::string &name, std::ifstream &ifs);
  Section(std::ifstream &ifs);
  ~Section();
  Section &operator=(Section &&other) = default;
  void set_name(std::string_view name);
  std::string_view get_name() const;
  void insert(std::string &next, std::ifstream &ifs);
  std::string prase(std::ifstream &ifs);
};
class Xcl {
public:
  Section top;
  std::unordered_map<std::string, Section> sections;
  Xcl();
  Xcl(std::string_view path);
  ~Xcl();
};
} // namespace xcl
#endif