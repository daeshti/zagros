//
// Created by Arian Dashti on 6/28/22.
//

#ifndef ZAGROS_STACK
#define ZAGROS_STACK

namespace zagros {
/**
* A stack type for arr. Unsafe because `guard` method must be called and
* have it`s result checked before pushing or popping.
* Otherwise push and stack_pop will have undefined behavior.
*/
class DataStack {
 private:
  /// The stack`s data.
  std::__1::array<zagros::Cell, zagros::DATA_STACK_SIZE> arr;

  /// The stack`s top index.
  size_t top = 0;
 public:

  /**
   * Constructor
   */
  DataStack() noexcept: arr(), top(0) {}

  /**
   * Guarantees that stack is safe for n `pops` first and then m `pushes` later.
   * @param pops The number of pops to be performed.
   * @param pushes The number of pushes to be performed.
   * @return Success if the stack is safe, Error otherwise.
   */
  auto guard(size_t pops, size_t pushes) const noexcept -> zagros::result<> {
    if (top + pushes > zagros::DATA_STACK_SIZE) {
      return {zagros::Error::DataStackOverflow, zagros::Unit{}};
    }
    if (top < pops) {
      return {zagros::Error::DataStackUnderflow, zagros::Unit{}};
    }
    return {zagros::Error::None, zagros::Unit{}};
  }

  /**
   * Pushes a value onto the stack.
   * @param value The value to be pushed.
   */
  auto push(zagros::Cell value) noexcept -> void {
    arr[top++] = value;
  }

  /**
   * Pops a value off the stack.
   * @return The value popped off the stack.
   */
  auto pop() noexcept -> zagros::Cell {
    return arr[--top];
  }

  /**
   * Clears the stack.
   */
  auto clear() noexcept -> void {
    top = 0;
  }

  /**
   * Gets a snapshot of the stack.
   * @return A snapshot of the stack.
   */
  auto snapshot() const noexcept -> zagros::DataStackSnapshot {
    return zagros::DataStackSnapshot{arr, top};
  }
};
/**
 * A stack type for addresses. Safe because all unsafe operations return `result`.
 */
class AddressStack {
 private:
  /// The stack`s data.
  std::__1::array<zagros::Cell, zagros::ADDRESS_STACK_SIZE> arr;

  /// The stack`s top index.
  size_t top = 0;
 public:

  /**
  * Constructor
  */
  AddressStack() noexcept: arr(), top(0) {}

  /**
   * Pushes a value onto the stack.
   * @param value The value to be pushed.
   * @return Success if the operation is successful, Error otherwise.
   */
  auto push(zagros::Cell value) noexcept -> zagros::result<> {
    if (top >= zagros::ADDRESS_STACK_SIZE) {
      return {zagros::Error::AddressStackOverflow, zagros::Unit{}};
    }
    arr[top++] = value;
    return {zagros::Error::None, zagros::Unit{}};
  }

  /**
   * Pops a value off the stack.
   * @return The value if the operation is successful, Error otherwise.
   */
  auto pop() noexcept -> zagros::result<zagros::Cell> {
    if (top == 0) {
      return {zagros::Error::AddressStackUnderflow, zagros::Cell{}};
    }
    const auto value = arr[--top];
    return {zagros::Error::None, value};
  }

  /**
   * Clears the stack.
   */
  auto clear() noexcept -> void {
    top = 0;
  }

  /**
   * Gets a snapshot of the stack.
   * @return A snapshot of the stack.
   */
  auto snapshot() const noexcept -> zagros::AddressStackSnapshot {
    return AddressStackSnapshot{arr, top};
  }
};
}
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <utility>
#include "result.hpp"
#include "cell.hpp"
#include "zagros_configuration.h"
#include "instruction_mode.hpp"
#include "snapshot.hpp"
#endif //ZAGROS_STACK
