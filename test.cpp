#include "xcl/xcl.h"
#include <iostream>
void sec_print(xcl::Section &sec, int indent = 0) {
    for (int i = 0; i < indent; i++) {
        std::cout << "  ";
    }
    std::cout << "[" << sec.get_name() << "]" << std::endl;
    for (auto &[key, value] : sec.kv) {
        for (int i = 0; i < indent; i++) {
        std::cout << "  ";
        }
        std::cout << key << " = ";
        if (key == "string") {
        std::cout << std::get<std::string>(value) << std::endl;
        } else if (key == "long") {
        std::cout << std::get<long>(value) << std::endl;
        } else if (key == "unsigned") {
        std::cout << std::get<unsigned long>(value) << std::endl;
        } else if (key == "float") {
        std::cout << std::get<float>(value) << std::endl;
        } else if (key == "double") {
        std::cout << std::get<double>(value) << std::endl;
        }
    }
    for (auto &[key, value] : sec.sections) {
        sec_print(value, indent + 1);
    }
}
int main() {
  xcl::Xcl xcl("config.xcl");
  for (auto &[key, value] : xcl.top.kv) {
    std::cout << key << " = ";
    if (key == "string") {
      std::cout << std::get<std::string>(value) << std::endl;
    } else if (key == "long") {
      std::cout << std::get<long>(value) << std::endl;
    } else if (key == "unsigned") {
      std::cout << std::get<unsigned long>(value) << std::endl;
    } else if (key == "float") {
      std::cout << std::get<float>(value) << std::endl;
    } else if (key == "double") {
      std::cout << std::get<double>(value) << std::endl;
    }
  }
  for (auto &[key, value] : xcl.sections) {
    sec_print(value);
  }
}