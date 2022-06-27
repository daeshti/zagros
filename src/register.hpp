//
// Created by Arian Dashti on 6/28/22.
//

#ifndef ZAGROS_REGISTER
#define ZAGROS_REGISTER

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
#include "stack.hpp"

namespace zagros {
/**
 * A bank of registers.
 * @tparam S Size of the bank.
 */

class RegisterBank {
 private:
  /// The bank`s arr.
  std::__1::array<zagros::Cell, zagros::REGISTER_BANK_SIZE> arr;
 public:
  /**
   * Constructs a new register bank. All registers are initialized to 0.
   */
  RegisterBank() noexcept {
    std::fill(arr.begin(), arr.end(), zagros::Cell{});
  }

  /**
   * Returns the value of a register.
   * @param id The register`s id.
   * @return The value if the operation is successful, Error otherwise.
   */
  auto read(size_t id) const noexcept -> zagros::result<zagros::Cell> {
    if (id >= zagros::REGISTER_BANK_SIZE) {
      return {zagros::Error::IllegalRegisterId, zagros::Cell{}};
    }
    const auto value = arr[id];
    return {zagros::Error::None, value};
  }

  /**
   * Sets the value of a register.
   * @param id The register`s id.
   * @param value The value to be set.
   * @return Success if the operation is successful, Error otherwise.
   */
  auto write(size_t id, zagros::Cell value) noexcept -> zagros::result<> {
    if (id >= zagros::REGISTER_BANK_SIZE) {
      return {zagros::Error::IllegalRegisterId, zagros::Unit{}};
    }
    arr[id] = value;
    return {zagros::Error::None, zagros::Unit{}};
  }

  /**
   * Clears the bank.
   */
  auto clear() noexcept -> void {
    std::fill(arr.begin(), arr.end(), zagros::Cell{});
  }

  /**
   * Gets a snapshot of the bank.
   * @return A snapshot of the bank.
   */
  auto snapshot() const noexcept -> zagros::RegisterBankSnapshot {
    return zagros::RegisterBankSnapshot(arr);
  }
};
}

#endif //ZAGROS_REGISTER
