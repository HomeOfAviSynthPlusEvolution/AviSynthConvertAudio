#pragma once

#include <string>

namespace avsut::test {

template <typename Function>
struct Variant {
  std::string name;
  Function function;
};

} // namespace avsut::test
