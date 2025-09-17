#pragma once

#include <iostream>
#include <format>
#include <sstream>

const char* file_name(const char* path);
std::string get_time();
std::string get_thread();

template<typename... U>
std::ostringstream& println(std::ostringstream& os, U... u) {
  int i = 0;
  auto printer = [&os, &i]<typename Arg>(Arg arg) {
    os << arg;
    //if (i == sizeof...(u)) {
    //  os << std::endl;
    //}
  };
  (printer(u), ...);
  os.flush();
  return os;
}

#define debug(...) \
  std::ostringstream os; \
  os << format("[{}] [Thread-{}] [{}:{}:{}] - ", get_time(), get_thread(), file_name(__FILE__), __LINE__, __func__); \
  println(os, __VA_ARGS__); \
  os << std::endl; \
  std::cout << os.str();
