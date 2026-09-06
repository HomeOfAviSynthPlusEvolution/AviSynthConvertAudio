#pragma once

#include "support/plane_view.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <type_traits>

namespace avsut::test {

struct FloatTolerance {
  float absolute;
  float relative;
};

template <typename T>
::testing::AssertionResult compare_exact(PlaneView<const T> expected, PlaneView<const T> actual) {
  static_assert(std::is_integral_v<T>);
  if (expected.width() != actual.width() || expected.height() != actual.height()) {
    return ::testing::AssertionFailure() << "dimension mismatch";
  }
  for (std::size_t y = 0; y < expected.height(); ++y) {
    for (std::size_t x = 0; x < expected.width(); ++x) {
      if (expected.row(y)[x] != actual.row(y)[x]) {
        return ::testing::AssertionFailure()
               << "row=" << y << " col=" << x << " expected=" << +expected.row(y)[x]
               << " actual=" << +actual.row(y)[x];
      }
    }
  }
  return ::testing::AssertionSuccess();
}

inline ::testing::AssertionResult compare_float(PlaneView<const float> expected,
                                                PlaneView<const float> actual,
                                                FloatTolerance tolerance) {
  if (expected.width() != actual.width() || expected.height() != actual.height()) {
    return ::testing::AssertionFailure() << "dimension mismatch";
  }
  for (std::size_t y = 0; y < expected.height(); ++y) {
    for (std::size_t x = 0; x < expected.width(); ++x) {
      const float lhs = expected.row(y)[x];
      const float rhs = actual.row(y)[x];
      if (std::isnan(lhs) || std::isnan(rhs)) {
        if (!(std::isnan(lhs) && std::isnan(rhs))) {
          return ::testing::AssertionFailure() << "NaN mismatch row=" << y << " col=" << x;
        }
        continue;
      }
      if (lhs == rhs) {
        continue;
      }
      const float difference = std::abs(lhs - rhs);
      const float limit =
          std::max(tolerance.absolute, tolerance.relative * std::max(std::abs(lhs), std::abs(rhs)));
      if (difference > limit) {
        return ::testing::AssertionFailure()
               << "row=" << y << " col=" << x << " expected=" << lhs << " actual=" << rhs
               << " difference=" << difference << " limit=" << limit;
      }
    }
  }
  return ::testing::AssertionSuccess();
}

}  // namespace avsut::test
