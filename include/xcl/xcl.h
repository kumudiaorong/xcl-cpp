#ifndef XSL_XCL_H
#define XSL_XCL_H
#include <chrono>
#include <concepts>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <variant>
namespace xcl {
  const char value_prefix[] = "slufd";
  class Section {
  public:
    typedef std::variant<std::string, long, unsigned long, float, double> value_type;
  private:
    std::string _full_path;
    std::string _name;
    std::unordered_map<std::string, value_type> _kvs;
    std::unordered_map<std::string, Section> sections;
    mutable bool _update_flag;
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

    bool need_update() const;
  public:
    std::string get_full_name() const;
    std::optional<std::reference_wrapper<Section>> find(std::string_view name);
    auto find(const char name[]) -> decltype(find(std::string_view()));

    template <typename T>
    std::optional<T> find(std::string_view name) const;
    template <typename T>
    decltype(auto) find(const char name[]) const;

    std::pair<std::reference_wrapper<Section>, bool> try_insert(std::string_view sec);
    auto try_insert(const char *sec) -> decltype(try_insert(std::string_view()));

    template <typename T, typename... Args>
      requires std::constructible_from<T, Args...>
    std::pair<T, bool> try_insert(std::string_view path, Args&&...args) {
      auto seq = path.rfind('\'');
      if(seq == std::string::npos) {
        auto [kv, ok] = this->_kvs.try_emplace(std::string(path), T(std::forward<Args>(args)...));
        return {std::get<T>(kv->second), ok};
      }
      return this->try_insert(path.substr(0, seq))
        .first.get()
        .try_insert<T>(path.substr(seq + 1), std::forward<Args>(args)...);
    }
    template <typename T, typename... Args>
      requires std::constructible_from<T, Args...> decltype(auto)
    try_insert(const char *path, Args&&...args) {
      return this->try_insert<T>(std::string_view(path), std::forward<Args>(args)...);
    }

    template <typename T, typename... Args>
      requires std::constructible_from<T, Args...>
    void insert_or_assign(std::string_view path, Args&&...args) {
      auto seq = path.rfind('\'');
      if(seq == std::string::npos) {
        this->_kvs.insert_or_assign(std::string(path), T(std::forward<Args>(args)...));
      } else {
        this->try_insert(path.substr(0, seq))
          .first.get()
          .insert_or_assign<T>(path.substr(seq + 1), std::forward<Args>(args)...);
      }
    }
    template <typename T, typename... Args>
      requires std::constructible_from<T, Args...> decltype(auto)
    insert_or_assign(const char *path, Args&&...args) {
      return this->insert_or_assign<T>(std::string_view(path), std::forward<Args>(args)...);
    }

    void clear();
    void recursive_create(std::filesystem::path path);

    friend std::ostream& operator<<(std::ostream& os, const Section& sec);
    Section& operator>>(const char path[]);
  };
  class Xcl : public Section {
    std::string _full_path;
    std::filesystem::file_time_type _last_write_time;
  public:
    Xcl();
    Xcl(std::string_view path);
    void save(bool force = false);
    void reload(bool force = false);
    ~Xcl();
  private:
    bool prase_file();
  };
}  // namespace xcl
#endif