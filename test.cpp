#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>

#include "xcl/xcl.h"
// void sec_print(xcl::Section& sec, int indent = 0) {
//   for(int i = 0; i < indent; i++) {
//     std::cout << "  ";
//   }
//   std::cout << "[" << sec.get_full_name() << "]:" << sec.kv.size() << " sec:" << sec.sections.size() << std::endl;
//   for(auto& [key, value] : sec.kv) {
//     for(int i = 0; i < indent; i++) {
//       std::cout << "  ";
//     }
//     std::cout << key << " = ";
//     if(key == "string") {
//       std::cout << std::get<std::string>(value) << std::endl;
//     } else if(key == "long") {
//       std::cout << std::get<long>(value) << std::endl;
//     } else if(key == "unsigned") {
//       std::cout << std::get<unsigned long>(value) << std::endl;
//     } else if(key == "float") {
//       std::cout << std::get<float>(value) << std::endl;
//     } else if(key == "double") {
//       std::cout << std::get<double>(value) << std::endl;
//     }
//   }
//   for(auto& [key, value] : sec.sections) {
//     sec_print(value, indent + 1);
//   }
// }
int main() {
  xcl::Xcl xcl(std::string(std::getenv("HOME")) + "/.config/xcl/config.xcl");
  xcl.try_insert<long>("ac'th", 1);
  xcl.try_insert<std::string>("ac'oth", "asdf");
  xcl.try_insert<std::string>("oth", "asdf");
  xcl.try_insert<std::string>("oth'asd", "asdf");
  xcl.try_insert<std::string>("oth'asdf", "asdf");
  xcl.try_insert<std::string>("oth'asdf'as", "asdf");
  // std::function<void(long&)> f = [](long& v) { std::cout << v << std::endl; };
  xcl.find_and<long>("ac'th", [](long& v) { std::cout << v << std::endl; });
  xcl.save();
  // std::cout << xcl;
  // std::cout << std::string(std::getenv("HOME")) + "/.config/xcl/config.xcl";
  // sec_print(xcl);
  // std::cout << "====================" << std::endl;
  // auto sub = xcl.find("ac'th");
  // if(sub.has_value()) {
  //   sec_print(sub.value());
  // }
  // std::cout << "====================" << std::endl;
  // xcl.try_insert("ac'th'new");
  // sec_print(xcl);
  // std::cout << "====================" << std::endl;
  // xcl.try_insert<long>("ac'th'new'long", 1);
  // xcl.try_insert<float>("ac'th'new'float");
  // sec_print(xcl);
  // std::cout << "====================" << std::endl;
  // std::cout << xcl;
}