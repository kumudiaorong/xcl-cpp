#ifndef XSL_XCL_H
#define XSL_XCL_H
#include <fstream>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <variant>
namespace xcl {
  class Section {
  public:
    std::string _full_path;
    std::string _name;
    std::unordered_map<std::string, std::variant<std::string, long, unsigned long, float, double>> kv;
    std::unordered_map<std::string, Section> sections;
  public:
    Section();
    Section(Section&& other) = default;
    Section(std::string_view full_path, std::string_view name);
    ~Section();
    Section& operator=(Section&& other) = default;
    void set_name(std::string_view name);
    std::string_view get_name() const;
  protected:
    Section(std::string_view full_path, std::string& name, std::ifstream& ifs);
    std::string prase_kv(std::ifstream& ifs);
    std::tuple<std::string_view::size_type, std::string, std::unordered_map<std::string, Section>::iterator> prase_path(
      std::string_view name);
    void prase(std::string& next, std::ifstream& ifs);
  public:
    std::string get_full_name() const;
    std::optional<std::reference_wrapper<Section>> find(std::string_view name);
    auto find(const char *name) -> decltype(find(std::string_view()));
    template <typename T>
    std::optional<std::reference_wrapper<T>> find(std::string_view name);

    std::pair<std::reference_wrapper<Section>, bool> try_insert(std::string_view sec);
    auto try_insert(const char *sec) -> decltype(try_insert(std::string_view()));
    template <typename T, typename... Args>
      requires std::constructible_from<T, Args...> bool
    try_insert(std::string_view path, Args&&...args) {
      auto seq = path.rfind('\'');
      if(seq == std::string::npos) {
        return this->kv.try_emplace(std::string(path), T(std::forward<Args>(args)...)).second;
      }
      return this->try_insert(path.substr(0, seq))
        .first.get()
        .try_insert<T>(path.substr(seq + 1), std::forward<Args>(args)...);
    }
    template <typename T, typename... Args>
      requires std::constructible_from<T, Args...> bool
    try_insert(const char *path, Args&&...args) {
      return this->try_insert<T>(std::string_view(path), std::forward<Args>(args)...);
    }
    friend std::ostream& operator<<(std::ostream& os, const Section& sec);
  };
  class Xcl : public Section {
  public:
    Xcl();
    Xcl(std::string_view path);
    ~Xcl();
  };
}  // namespace xcl
#endif