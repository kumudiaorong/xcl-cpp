#ifndef XSL_XCL_H
#define XSL_XCL_H
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
  Section();
  Section(std::string_view name, std::ifstream &ifs, std::string &next);
  Section(std::ifstream &ifs);
  ~Section();
  void set_name(std::string_view name);
  std::string_view get_name() const;
};
class Xcl {
public:
  Section top;
  std::unordered_map<std::string, std::variant<std::string, int, double>> kv;

  Xcl();
  Xcl(std::string_view path);
  ~Xcl();
};
} // namespace xcl
#endif