//
// Created by Arian Dashti on 6/27/22.
//

#ifndef ZAGROS_CELL
#define ZAGROS_CELL

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <utility>
#include "result.hpp"
#include "instruction_mode.hpp"

namespace zagros {
/**
 * A representation of a register's value.
 */
class Cell {
 private:
  /// The value of the register as a byte array.
  std::array<uint8_t, 4> bs;

 public:

  // region Constructors
  /**
   * Default constructor.
   */
  Cell() noexcept: bs{0, 0, 0, 0} {}

  /**
   * A constructor that initializes the register with given bytes.
   */
  Cell(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3) noexcept: bs{b0, b1, b2, b3} {}

  /**
   * A constructor that initializes the register with given bytes.
   */
  explicit Cell(std::array<uint8_t, 4> bs) noexcept: bs{bs} {}

  // Following functions write to all 4 bytes of the `bs` array therefore don't need to initialize the `bs` array.
#pragma clang diagnostic push
#pragma ide diagnostic ignored "cppcoreguidelines-pro-type-member-init"
  /**
  * A constructor to initialize the register with an int32_t.
   * @param value The value to initialize the register with.
  */
  explicit Cell(const int32_t value) noexcept {
    memcpy(bs.data(), &value, 4);
  }

  /**
  * A constructor to initialize the register with an uint32_t.
   * @param value The value to initialize the register with.
  */
  explicit Cell(const uint32_t value) noexcept {
    memcpy(bs.data(), &value, 4);
  }

  /**
  * A constructor that initializes the register with a float.
  * *NOTE*: This constructor is not  because it`s not possible to initialize a float with a .
   * @param value The value to initialize the register with.
  */
  explicit Cell(const float value) noexcept {
    memcpy(bs.data(), &value, 4);
  }

  /**
  * A constructor that initializes the register with a bool value.
   * @param value The value to initialize the register with.
  */
  explicit Cell(const bool value) noexcept {
    std::fill_n(bs.begin(), 4, value ? 0xFF : 0x00);
  }

  /**
   * Copy constructor.
   * @param rhs The rhs cell to copy from.
   */
  Cell(const Cell &rhs) noexcept = default;
#pragma clang diagnostic pop
// endregion

  // region Accessors
  /**
  * Gets the value of the register as an int32_t.
  * @return Value of the register as an int32_t.
  */

  int32_t to_int32() const noexcept {
    return bs[0] | (bs[1] << 8) | (bs[2] << 16) | (bs[3] << 24);
  }

  /**
  * Gets the value of the register as an uint32_t.
  * @return Value of the register as an uint32_t.
  */

  uint32_t to_uint32() const noexcept {
    return bs[0] | (bs[1] << 8) | (bs[2] << 16) | (bs[3] << 24);
  }

  /**
  * Gets the value of the register as a float.
  * @return Value of the register as a float.
  */

  float to_float() const noexcept {
    float value;
    memcpy(&value, bs.data(), 4);
    return value;
  }

  /**
   * Gets the value of the register as a size_t
   * @return Value of the register as a size_t
   */
  size_t to_size() const noexcept {
    return static_cast<size_t>(this->to_uint32());
  }

  /**
  * Gets the value of the register as a bool.
  * @return Value of the register as a bool.
  */

  bool to_bool() const noexcept {
    return std::all_of(bs.begin(), bs.end(), [](uint8_t b) { return b == 0xFF; });
  }

  /**
   * Gets the value of register as bytes.
   */

  std::array<uint8_t, 4> to_bytes() const noexcept {
    return bs;
  }

  /**
   * Gets the value of register as byte.
   */

  uint8_t to_byte() const noexcept {
    return bs[0];
  }

// endregion

  // region Comparison operations
  /**
   * Equality operation.
   * @param rhs The right hand side of the comparison.
   * @param op_mode The operation mode to use.
   * @return True if the register is equal the other register in provided operation mode.
   */
  Cell equal(const Cell rhs) const noexcept {
    return Cell(this->to_uint32() == rhs.to_uint32());
  }

  /**
   * Inequality operation.
   * @param rhs The right hand side of the comparison.
   * @param op_mode The operation mode to use.
   * @return True if the register is not equal the rhs register in provided operation mode.
   */
  Cell not_equal(const Cell rhs) const noexcept {
    return Cell(this->to_uint32() != rhs.to_uint32());
  }

  /**
   * Less than operation.
   * @param rhs The right hand side of the comparison.
   * @param op_mode The operation mode to use.
   * @return True if the register is less than the other register in provided operation mode.
   */
  Cell less_than(const Cell rhs, OpMode op_mode) const noexcept {
    switch (op_mode) {
      case OpMode::SIGNED: {
        return Cell(this->to_int32() < rhs.to_int32());
      }
      case OpMode::UNSIGNED: {
        return Cell(this->to_uint32() < rhs.to_uint32());
      }
      case OpMode::FLOAT: {
        return Cell(this->to_float() < rhs.to_float());
      }
    }
  }

  /**
   * Greater than operation.
   * @param rhs The right hand side of the comparison.
   * @param op_mode The operation mode to use.
   * @return True if the register is greater than than the other register in provided operation mode.
   */
  Cell greater_than(const Cell rhs, OpMode op_mode) const noexcept {
    switch (op_mode) {
      case OpMode::SIGNED: {
        return Cell(this->to_int32() > rhs.to_int32());
      }
      case OpMode::UNSIGNED: {
        return Cell(this->to_uint32() > rhs.to_uint32());
      }
      case OpMode::FLOAT: {
        return Cell(this->to_float() > rhs.to_float());
      }
    }
  }
// endregion

  // region Arithmetic operations
  /**
   * Addition operation.
   * @param rhs The right hand side of the addition.
   * @param op_mode The operation mode to use.
   * @return The result of the addition.
   */
  Cell add(const Cell rhs, OpMode op_mode) const noexcept {
    switch (op_mode) {
      case OpMode::SIGNED: {
        return Cell(this->to_int32() + rhs.to_int32());
      }
      case OpMode::UNSIGNED: {
        return Cell(this->to_uint32() + rhs.to_uint32());
      }
      case OpMode::FLOAT: {
        return Cell(this->to_float() + rhs.to_float());
      }
    }
  }

  /**
   * Subtraction operation.
   * @param rhs The right hand side of the subtraction.
   * @param op_mode The operation mode to use.
   * @return The result of the subtraction.
   */
  Cell subtract(const Cell rhs, OpMode op_mode) const noexcept {
    switch (op_mode) {
      case OpMode::SIGNED: {
        return Cell(this->to_int32() - rhs.to_int32());
      }
      case OpMode::UNSIGNED: {
        return Cell(this->to_uint32() - rhs.to_uint32());
      }
      case OpMode::FLOAT: {
        return Cell(this->to_float() - rhs.to_float());
      }
    }
  }

  /**
   * Multiplication operation.
   * @param rhs The right hand side of the multiplication.
   * @param op_mode The operation mode to use.
   * @return The result of the multiplication.
   */
  Cell multiply(const Cell rhs, OpMode op_mode) const noexcept {
    switch (op_mode) {
      case OpMode::SIGNED: {
        return Cell(this->to_int32() * rhs.to_int32());
      }
      case OpMode::UNSIGNED: {
        return Cell(this->to_uint32() * rhs.to_uint32());
      }
      case OpMode::FLOAT: {
        return Cell(this->to_float() * rhs.to_float());
      }
    }
  }

  /**
   * Division and remainder operation.
   * @param rhs The right hand side of the division.
   * @param op_mode The operation mode to use.
   * @return a tuple of (Error::DivisionByZero, _, _) if the dominator is zero,
   * otherwise a tuple of (Error::None, Modulo, Quotient).
   */
  std::tuple<Error, Cell, Cell> divide_remainder(const Cell rhs,
                                                      OpMode op_mode) const noexcept {
    switch (op_mode) {
      case OpMode::SIGNED: {
        if (rhs.to_int32() == 0) {
          return std::make_tuple(Error::DivisionByZero, Cell(0), Cell(0));
        }
        return std::make_tuple(Error::None,
                               Cell(this->to_int32() % rhs.to_int32()),
                               Cell(this->to_int32() / rhs.to_int32()));
      }
      case OpMode::UNSIGNED: {
        if (rhs.to_uint32() == 0) {
          return std::make_tuple(Error::DivisionByZero, Cell(0), Cell(0));
        }
        return std::make_tuple(Error::None,
                               Cell(this->to_uint32() % rhs.to_uint32()),
                               Cell(this->to_uint32() / rhs.to_uint32()));
      }
      case OpMode::FLOAT: {
        if (rhs.to_float() == 0.0) {
          return std::make_tuple(Error::DivisionByZero, Cell(0), Cell(0));
        }
        return std::make_tuple(Error::None,
                               Cell(fmod(this->to_float(), rhs.to_float())),
                               Cell(this->to_float() / rhs.to_float()));
      }
    }
  }

  /**
   * Multiply, then Division and remainder operation. ( (this * mul) / div )
   * @param mid The value to multiply the this by.
   * @param rhs The right hand side of the division.
   * @param op_mode The operation mode to use.
   * @return a tuple of (Error::DivisionByZero, _, _) if the dominator is zero,
   * otherwise a tuple of (Error::None, Modulo, Quotient).
   */
  std::tuple<Error, Cell, Cell> multiply_divide_remainder(
      const Cell mul, const Cell rhs, OpMode op_mode) const noexcept {
    switch (op_mode) {
      case OpMode::SIGNED: {
        if (rhs.to_int32() == 0) {
          return std::make_tuple(Error::DivisionByZero, Cell(0), Cell(0));
        }
        return {Error::None,
                Cell((this->to_int32() * mul.to_int32()) % rhs.to_int32()),
                Cell((this->to_int32() * mul.to_int32()) / rhs.to_int32())};
      }
      case OpMode::UNSIGNED: {
        if (rhs.to_uint32() == 0) {
          return std::make_tuple(Error::DivisionByZero, Cell(0), Cell(0));
        }
        return {Error::None,
                Cell((this->to_uint32() * mul.to_uint32()) % rhs.to_uint32()),
                Cell((this->to_uint32() * mul.to_uint32()) / rhs.to_uint32())};
      }
      case OpMode::FLOAT: {
        if (rhs.to_float() == 0.0) {
          return {Error::DivisionByZero, Cell(0), Cell(0)};
        }
        return {Error::None,
                Cell(fmod((this->to_float() * mul.to_float()), rhs.to_float())),
                Cell((this->to_float() * mul.to_float()) / rhs.to_float())};
      }
    }
  }
// endregion

  // region Bit operations
  /**
   * Bitwise AND operation.
   * @param rhs The right hand side of the operation.
   * @return The result of the operation.
   */
  Cell bitwise_and(const Cell rhs) const noexcept {
    return Cell(this->to_uint32() & rhs.to_uint32());
  }

  /**
   * Bitwise OR operation.
   * @param rhs The right hand side of the operation.
   * @return The result of the operation.
   */
  Cell bitwise_or(const Cell rhs) const noexcept {
    uint32_t left;
    memcpy(&left, this->bs.data(), sizeof(int32_t));
    uint32_t right;
    memcpy(&right, rhs.bs.data(), sizeof(int32_t));
    int32_t result = left | right;
    return Cell(result);
  }

  /**
   * Bitwise XOR operation.
   * @param rhs The right hand side of the operation.
   * @return The result of the operation.
   */
  Cell bitwise_xor(const Cell rhs) const noexcept {
    return Cell(this->to_uint32() ^ rhs.to_uint32());
  }

  /**
   * Bitwise NOT operation.
   * @return The result of the operation.
   */
  Cell bitwise_not() const noexcept {
    return Cell(~this->to_uint32());
  }

  /**
   * Bitwise left shift operation.
   * @param rhs The right hand side of the operation.
   * @param op_mode The operation mode.
   * @return a pair of (Error::InvalidFloatOperation, _) if any of the arguments is a float,
   * otherwise a pair of (Error::None, Result).
   */
  result<Cell> bitwise_shift_left(const Cell rhs, OpMode op_mode) const noexcept {
    switch (op_mode) {
      case OpMode::SIGNED: {
        return {Error::None, Cell(this->to_int32() << rhs.to_int32())};
      }
      case OpMode::UNSIGNED: {
        return {Error::None, Cell(this->to_uint32() << rhs.to_uint32())};
      }
      case OpMode::FLOAT: {
        return {Error::InvalidFloatOperation, Cell(0)};
      }
    }
  }

  /**
   * Bitwise right shift operation.
   * @param rhs The right hand side of the operation.
   * @param op_mode The operation mode.
   * @return a pair of (Error::InvalidFloatOperation, _) if any of the arguments is a float,
   * otherwise a pair of (Error::None, Result).
   */
  result<Cell> bitwise_shift_right(const Cell rhs, OpMode op_mode) const noexcept {
    switch (op_mode) {
      case OpMode::SIGNED: {
        return {Error::None, Cell(this->to_int32() >> rhs.to_int32())};
      }
      case OpMode::UNSIGNED: {
        return {Error::None, Cell(this->to_uint32() >> rhs.to_uint32())};
      }
      case OpMode::FLOAT: {
        return {Error::InvalidFloatOperation, Cell(0)};
      }
    }
  }
// endregion

  // region Operators operations
  /**
   * Assignment operator.
   * @param rhs The right hand side of the operation.
   */
  Cell &operator=(const Cell &rhs) noexcept = default;

  /**
   * Equality operator.
   * @param rhs The other cell to compare with.
   */
  bool operator==(const Cell &rhs) const noexcept {
    return this->to_uint32() == rhs.to_uint32();
  }
  // endregion
};
}

#endif //ZAGROS_CELL
