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

/**
 * A bank of registers.
 * @tparam S Size of the bank.
 */

class RegisterBank {
 private:
  /// The bank`s arr.
  std::array<Cell, REGISTER_BANK_SIZE> arr;
 public:
  /**
   * Constructs a new register bank. All registers are initialized to 0.
   */
  RegisterBank() noexcept {
    std::fill(arr.begin(), arr.end(), Cell{});
  }

  /**
   * Returns the value of a register.
   * @param id The register`s id.
   * @return The value if the operation is successful, Error otherwise.
   */
  auto read(size_t id) const noexcept -> result<Cell> {
    if (id >= REGISTER_BANK_SIZE) {
      return {Error::IllegalRegisterId, Cell{}};
    }
    const auto value = arr[id];
    return {Error::None, value};
  }

  /**
   * Sets the value of a register.
   * @param id The register`s id.
   * @param value The value to be set.
   * @return Success if the operation is successful, Error otherwise.
   */
  auto write(size_t id, Cell value) noexcept -> result<> {
    if (id >= REGISTER_BANK_SIZE) {
      return {Error::IllegalRegisterId, Unit{}};
    }
    arr[id] = value;
    return {Error::None, Unit{}};
  }

  /**
   * Clears the bank.
   */
  auto clear() noexcept -> void {
    std::fill(arr.begin(), arr.end(), Cell{});
  }

  /**
   * Gets a snapshot of the bank.
   * @return A snapshot of the bank.
   */
  auto snapshot() const noexcept -> RegisterBankSnapshot {
    return RegisterBankSnapshot(arr);
  }
};


#endif //ZAGROS_REGISTER
