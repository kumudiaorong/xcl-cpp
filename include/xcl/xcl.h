#ifndef XSL_XCL_H
#define XSL_XCL_H
#include <concepts>
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
    typedef std::variant<std::string, long, unsigned long, float, double> sym_type;
  private:
    std::string _full_path{};
    std::string _name{};
    std::unordered_map<std::string, sym_type> _kvs{};
    std::unordered_map<std::string, Section> _sections{};
  public:
    Section();
    Section(Section&& other) = default;
    Section(std::string_view full_path, std::string_view name);
    ~Section();
    Section& operator=(Section&& other) = default;
    void set_name(std::string_view name);
    std::string_view get_name() const;
  protected:
    mutable bool _update_flag{true};
    Section(std::string_view full_path, std::string& name, std::ifstream& ifs);
    std::string prase_kv(std::ifstream& ifs);
    std::optional<sym_type> prase_sym(std::string str);
    std::tuple<std::string_view::size_type, std::string, std::unordered_map<std::string, Section>::iterator> prase_path(
      std::string_view name);
    std::tuple<std::string_view::size_type, std::string, std::unordered_map<std::string, Section>::const_iterator>
    prase_path(std::string_view name) const;
    void prase(std::string& next, std::ifstream& ifs);

    bool need_update() const;
  public:
    std::string get_full_name() const;
    std::optional<std::reference_wrapper<Section>> find(std::string_view name);
    auto find(const char name[]) -> decltype(find(std::string_view()));

    std::optional<std::reference_wrapper<const Section>> find(std::string_view name) const;
    auto find(const char name[]) const -> decltype(find(std::string_view()));

    template <typename T>
    std::optional<std::reference_wrapper<const T>> find(std::string_view path) const {
      auto seq = path.rfind('\'');
      if(seq == std::string::npos) {
        auto kv = this->_kvs.find(path.data());
        if(kv == this->_kvs.end()) {
          return std::nullopt;
        } else {
          return std::get<T>(kv->second);
        }
      }
      auto sec = this->find(path.substr(0, seq));
      if(sec == std::nullopt) {
        return std::nullopt;
      }
      return sec->get().find<T>(path.substr(seq + 1));
    }
    template <typename T>
    decltype(auto) find(const char name[]) const {
      return this->find<T>(std::string_view(name));
    }

    std::pair<std::reference_wrapper<Section>, bool> try_insert(std::string_view path) {
      auto [seq, name, sec] = prase_path(path);  //
      if(sec == this->_sections.end()) {
        auto [it, ok] = this->_sections.emplace(name, Section{this->get_full_name(), name});
        this->_update_flag = false;
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
    decltype(auto) try_insert(const char *sec) {
      return this->try_insert(std::string_view(sec));
    }

    template <typename T, typename... Args>
      requires std::constructible_from<T, Args...>
    std::pair<std::reference_wrapper<T>, bool> try_insert(std::string_view path, Args&&...args) {
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
    std::filesystem::path _full_path{};
    std::filesystem::file_time_type _last_write_time{};
  public:
    Xcl();
    Xcl(std::string_view path);
    void save(bool force = false);
    void reload(bool force = false);
    void load(std::filesystem::path path);
    ~Xcl();
  private:
    bool prase_file();
  };
}  // namespace xcl
#endif