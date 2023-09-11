#ifndef XSL_XCL_H
#define XSL_XCL_H
#include <concepts>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <variant>
namespace xcl {
  class Section {
  public:
    using mutex_type = std::shared_mutex;
    using read_lock = std::shared_lock<mutex_type>;
    using write_lock = std::unique_lock<mutex_type>;

    typedef std::variant<std::string, long, unsigned long, float, double> value_type;
    std::string _full_path;
    std::string _name;
    std::unordered_map<std::string, value_type> kv;
    std::unordered_map<std::string, Section> sections;
    mutable mutex_type _m;
  public:
    Section();
    Section(Section&& other);
    Section(std::string_view full_path, std::string_view name);
    ~Section();
    Section& operator=(Section&& other);
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
      requires std::constructible_from<T, Args...>
    std::pair<std::reference_wrapper<value_type>, bool> try_insert(std::string_view path, Args&&...args) {
      auto seq = path.rfind('\'');
      if(seq == std::string::npos) {
        auto [kv, ok] = this->kv.try_emplace(std::string(path), T(std::forward<Args>(args)...));
        return {std::ref(kv->second), ok};
      }
      return this->try_insert(path.substr(0, seq))
        .first.get()
        .try_insert<T>(path.substr(seq + 1), std::forward<Args>(args)...);
    }
    template <typename T, typename... Args>
      requires std::constructible_from<T, Args...>
    auto try_insert(const char *path, Args&&...args)
      -> decltype(try_insert(std::string_view{}, std::forward<Args>(args)...)) {
      return this->try_insert<T>(std::string_view(path), std::forward<Args>(args)...);
    }

    void recursive_create(std::filesystem::path path);

    friend std::ostream& operator<<(std::ostream& os, const Section& sec);
    Section& operator>>(const char path[]);
  };
  class Xcl : public Section {
    std::string _full_path;
  public:
    Xcl();
    Xcl(std::string_view path);
    void save();
    ~Xcl();
  };
}  // namespace xcl
#endif