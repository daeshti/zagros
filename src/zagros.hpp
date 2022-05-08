#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <utility>

namespace Zagros {
/**
 * A struct to represent an empty / void value.
 */
enum class Unit {
};

/**
 * An enum to represent the different reasons to stop the current interpreting flow.
 */
enum class Error {
  /// The operation was successful.
  None,

  /// The operation failed because it`s attempting to divide_remainder by zero.
  DivisionByZero,

  /// The operation failed because it`s attempting to do illegal instruction on float mode.
  InvalidFloatOperation,

  /// The operation failed because not enough memory was available.
  OutOfMemory,

  /// The operation failed because not enough space is available on the arr stack.
  DataStackOverflow,

  /// The operation failed because not enough elements are available on the arr stack.
  DataStackUnderflow,

  /// The operation failed because not enough space is available on the addrs stack.
  AddressStackOverflow,

  /// The operation failed because not enough elements are available on the addrs stack.
  AddressStackUnderflow,

  /// The operation failed because of illegal register id.
  IllegalRegisterId,

  /// The operation failed because of illegal memory addrs.
  IllegalMemoryAddress,

  /// The operation failed because of illegal interrupt id.
  IllegalInterruptId,

  /// System should successfully halted.
  SystemHalt
};


/**
 * A type alias for return values.
 * Since we`re not using exceptions we use a product type to represent the (result * error) of an operation.
 * Unit as type parameter will be optimized by the compiler because it`s an empty struct.
 */
template<class T=Unit>
using result = std::pair<Error, T>;

/// Private namespace for the implementation of the vm.
namespace {
/**
 * A convenience function to create a result with success value.
 * @tparam T The type of the success value.
 * @param value The success value.
 * @return Result with success value.
 */
template<class T>
constexpr result<T> success(T value) noexcept {
  return {Error::None, value};
}

/**
 * A convenience function to create a result with success value.
 * @return Result with success value.
 */
constexpr result<> success() noexcept {
  return success(Unit{});
}

/**
 * A convenience function to create a result with error value.
 * @tparam T The type of successful result
 * @param error The error value.
 * @return Result with error value.
 */
template<class T>
consteval result<T> fail(Error error) noexcept {
  return {error, T{}};
}

/**
 * A convenience function to create a result with error value.
 * @param error The error value.
 * @return Result with error value.
 */
consteval result<> fail(Error error) noexcept {
  return fail<Unit>(error);
}
}

/**
 * Operation mode for the Core.
 */
enum class OpMode {
  /// The core is in signed integer mode.
  SIGNED,

  /// The core is in unsigned integer mode.
  UNSIGNED,

  /// The core is in floating point mode.
  FLOAT
};

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
  constexpr Cell() noexcept: bs{0, 0, 0, 0} {}

  /**
   * A constructor that initializes the register with given bytes.
   */
  constexpr Cell(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3) noexcept: bs{b0, b1, b2, b3} {}

  /**
   * A constructor that initializes the register with given bytes.
   */
  constexpr explicit Cell(std::array<uint8_t, 4> bs) noexcept: bs{bs} {}

  // Following functions write to all 4 bytes of the `bs` array therefore don't need to initialize the `bs` array.
#pragma clang diagnostic push
#pragma ide diagnostic ignored "cppcoreguidelines-pro-type-member-init"
  /**
  * A constructor to initialize the register with an int32_t.
   * @param value The value to initialize the register with.
  */
  explicit Cell(const int32_t value) noexcept {
    if constexpr(std::endian::native == std::endian::little){
      std::memcpy(bs.data(), &value, 4);
    } else {
      bs[0] = value;
      bs[1] = value >> 8;
      bs[2] = value >> 16;
      bs[3] = value >> 24;
    }
  }

  /**
  * A constructor to initialize the register with an uint32_t.
   * @param value The value to initialize the register with.
  */
  explicit Cell(const uint32_t value) noexcept {
    if constexpr(std::endian::native == std::endian::little){
      std::memcpy(bs.data(), &value, 4);
    } else {
      bs[0] = value;
      bs[1] = value >> 8;
      bs[2] = value >> 16;
      bs[3] = value >> 24;
    }
  }

  /**
  * A constructor that initializes the register with a float.
  * *NOTE*: This constructor is not constexpr because it`s not possible to initialize a float with a constexpr.
   * @param value The value to initialize the register with.
  */
  explicit Cell(const float value) noexcept {
    std::memcpy(bs.data(), &value, 4);
  }

  /**
  * A constructor that initializes the register with a bool value.
   * @param value The value to initialize the register with.
  */
  constexpr explicit Cell(const bool value) noexcept {
    std::fill_n(bs.begin(), 4, value ? 0xFF : 0x00);
  }

  /**
   * Copy constructor.
   * @param rhs The rhs cell to copy from.
   */
  constexpr Cell(const Cell &rhs) noexcept = default;
#pragma clang diagnostic pop
// endregion

  // region Accessors
  /**
  * Gets the value of the register as an int32_t.
  * @return Value of the register as an int32_t.
  */
  [[nodiscard]]
  constexpr int32_t to_int32() const noexcept {
    return bs[0] | (bs[1] << 8) | (bs[2] << 16) | (bs[3] << 24);
  }

  /**
  * Gets the value of the register as an uint32_t.
  * @return Value of the register as an uint32_t.
  */
  [[nodiscard]]
  constexpr uint32_t to_uint32() const noexcept {
    return bs[0] | (bs[1] << 8) | (bs[2] << 16) | (bs[3] << 24);
  }

  /**
  * Gets the value of the register as a float.
  * @return Value of the register as a float.
  */
  [[nodiscard]]
  float to_float() const noexcept {
    float value;
    std::memcpy(&value, bs.data(), 4);
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
  [[nodiscard]]
  constexpr bool to_bool() const noexcept {
    return std::all_of(bs.begin(), bs.end(), [](auto b) { return b == 0xFF; });
  }

  /**
   * Gets the value of register as bytes.
   */
  [[nodiscard]]
  constexpr std::array<uint8_t, 4> to_bytes() const noexcept {
    return bs;
  }

  /**
   * Gets the value of register as byte.
   */
  [[nodiscard]]
  constexpr uint8_t to_byte() const noexcept {
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
  [[nodiscard]] constexpr Cell equal(const Cell rhs) const noexcept {
    return Cell(this->to_uint32() == rhs.to_uint32());
  }

  /**
   * Inequality operation.
   * @param rhs The right hand side of the comparison.
   * @param op_mode The operation mode to use.
   * @return True if the register is not equal the rhs register in provided operation mode.
   */
  [[nodiscard]] constexpr Cell not_equal(const Cell rhs) const noexcept {
    return Cell(this->to_uint32() != rhs.to_uint32());
  }

  /**
   * Less than operation.
   * @param rhs The right hand side of the comparison.
   * @param op_mode The operation mode to use.
   * @return True if the register is less than the other register in provided operation mode.
   */
  [[nodiscard]] Cell less_than(const Cell rhs, OpMode op_mode) const noexcept {
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
  [[nodiscard]] Cell greater_than(const Cell rhs, OpMode op_mode) const noexcept {
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
  [[nodiscard]] Cell add(const Cell rhs, OpMode op_mode) const noexcept {
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
  [[nodiscard]] Cell subtract(const Cell rhs, OpMode op_mode) const noexcept {
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
  [[nodiscard]] Cell multiply(const Cell rhs, OpMode op_mode) const noexcept {
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
  [[nodiscard]] std::tuple<Error, Cell, Cell> divide_remainder(const Cell rhs, OpMode op_mode) const noexcept {
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
                               Cell(std::fmod(this->to_float(), rhs.to_float())),
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
  [[nodiscard]] std::tuple<Error, Cell, Cell> multiply_divide_remainder(
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
                Cell(std::fmod((this->to_float() * mul.to_float()), rhs.to_float())),
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
  [[nodiscard]] Cell bitwise_and(const Cell rhs) const noexcept {
    return Cell(this->to_uint32() & rhs.to_uint32());
  }

  /**
   * Bitwise OR operation.
   * @param rhs The right hand side of the operation.
   * @return The result of the operation.
   */
  [[nodiscard]] Cell bitwise_or(const Cell rhs) const noexcept {
    uint32_t left;
    std::memcpy(&left, this->bs.data(), sizeof(int32_t));
    uint32_t right;
    std::memcpy(&right, rhs.bs.data(), sizeof(int32_t));
    int32_t result = left | right;
    return Cell(result);
  }

  /**
   * Bitwise XOR operation.
   * @param rhs The right hand side of the operation.
   * @return The result of the operation.
   */
  [[nodiscard]] Cell bitwise_xor(const Cell rhs) const noexcept {
    return Cell(this->to_uint32() ^ rhs.to_uint32());
  }

  /**
   * Bitwise NOT operation.
   * @return The result of the operation.
   */
  [[nodiscard]] Cell bitwise_not() const noexcept {
    return Cell(~this->to_uint32());
  }

  /**
   * Bitwise left shift operation.
   * @param rhs The right hand side of the operation.
   * @param op_mode The operation mode.
   * @return a pair of (Error::InvalidFloatOperation, _) if any of the arguments is a float,
   * otherwise a pair of (Error::None, Result).
   */
  [[nodiscard]] result<Cell> bitwise_shift_left(const Cell rhs, OpMode op_mode) const noexcept {
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
  [[nodiscard]] result<Cell> bitwise_shift_right(const Cell rhs, OpMode op_mode) const noexcept {
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
  constexpr Cell &operator=(const Cell &rhs) noexcept = default;

  /**
   * Equality operator.
   * @param rhs The other cell to compare with.
   */
  constexpr bool operator==(const Cell &rhs) const noexcept {
    return this->to_uint32() == rhs.to_uint32();
  }
  // endregion
};

/**
 * A snapshot of a stack.
 * @tparam S Size of the stack.
 */
template<size_t S>
class StackSnapshot {
 private:
  /// The stack`s data.
  std::array<Cell, S> arr;

  /// The stack`s top index.
  size_t top;

 public:
  /**
   * Default constructor
   */
  constexpr StackSnapshot() noexcept: arr{}, top{0} {}

  /**
   * Constructor.
   * @param arr The stack`s arr.
   * @param top The stack`s top index.
   */
  constexpr StackSnapshot(std::array<Cell, S> arr, size_t top) noexcept: arr(arr), top(top) {}

  /**
   * Copy Constructor
   */
  constexpr StackSnapshot(const StackSnapshot &rhs) noexcept: arr{rhs.arr}, top{rhs.top} {};

  /**
   * Assignment Operator
   */
  auto operator=(const StackSnapshot &rhs) noexcept -> StackSnapshot & {
    this->arr = rhs.arr;
    this->top = rhs.top;
    return *this;
  }

    /**
     * Get the stack`s data.
     * @return The stack`s data.
     */
  constexpr std::array<Cell, S> get_arr() const noexcept {
    return arr;
  }

  /**
   * Get the stack`s top index.
   * @return The stack`s top index.
   */
  [[nodiscard]] constexpr size_t get_top() const noexcept {
    return top;
  }
};

/**
* A stack type for arr. Unsafe because `guard` method must be called and
* have it`s result checked before pushing or popping.
* Otherwise push and stack_pop will have undefined behavior.
* @tparam S Size of the stack.
*/
template<size_t S>
class DataStack {
 private:
  /// The stack`s data.
  std::array<Cell, S> arr;

  /// The stack`s top index.
  size_t top = 0;
 public:

  /**
   * Constructor
   */
  constexpr DataStack() noexcept: arr(), top(0) {}

  /**
   * Guarantees that stack is safe for n `pops` first and then m `pushes` later.
   * @param pops The number of pops to be performed.
   * @param pushes The number of pushes to be performed.
   * @return Success if the stack is safe, Error otherwise.
   */
  [[nodiscard]] auto guard(size_t pops, size_t pushes) const noexcept -> result<> {
    if (top + pushes > S) {
      return fail(Error::DataStackOverflow);
    }
    if (top < pops) {
      return fail(Error::DataStackUnderflow);
    }
    return success();
  }

  /**
   * Pushes a value onto the stack.
   * @param value The value to be pushed.
   */
  auto push(Cell value) noexcept -> void {
    arr[top++] = value;
  }

  /**
   * Pops a value off the stack.
   * @return The value popped off the stack.
   */
  auto pop() noexcept -> Cell {
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
  [[nodiscard]] auto snapshot() const noexcept -> StackSnapshot<S> {
    return StackSnapshot<S>{arr, top};
  }
};

/**
 * A stack type for addresses. Safe because all unsafe operations return `result`.
 */
template<size_t S>
class AddressStack {
 private:
  /// The stack`s data.
  std::array<Cell, S> arr;

  /// The stack`s top index.
  size_t top = 0;
 public:

  /**
  * Constructor
  */
  constexpr AddressStack() noexcept: arr(), top(0) {}

  /**
   * Pushes a value onto the stack.
   * @param value The value to be pushed.
   * @return Success if the operation is successful, Error otherwise.
   */
  auto push(Cell value) noexcept -> result<> {
    if (top >= S) {
      return fail(Error::AddressStackOverflow);
    }
    arr[top++] = value;
    return success();
  }

  /**
   * Pops a value off the stack.
   * @return The value if the operation is successful, Error otherwise.
   */
  auto pop() noexcept -> result<Cell> {
    if (top == 0) {
      return fail<Cell>(Error::AddressStackUnderflow);
    }
    const auto value = arr[--top];
    return success(value);
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
  [[nodiscard]] auto snapshot() const noexcept -> StackSnapshot<S> {
    return StackSnapshot<S>{arr, top};
  }
};

/**
 * A snapshot of bank of registers.
 * @tparam S Size of the bank.
 */
template<size_t S>
class RegisterBankSnapshot {
 private:
  /// The bank`s data.
  std::array<Cell, S> arr;

 public:
  /**
   * Default Constructor
   */
  constexpr RegisterBankSnapshot() noexcept: arr() {}

  /**
   * Constructor
   * @param arr The bank`s data.
   * @param top The bank`s top index.
   */
  constexpr explicit RegisterBankSnapshot(std::array<Cell, S> arr) noexcept: arr(arr) {}

  /**
 * Copy Constructor
 */
  constexpr RegisterBankSnapshot(const RegisterBankSnapshot &rhs) noexcept: arr{rhs.arr} {};

  /**
   * Assignment Operator
   */
  auto operator=(const RegisterBankSnapshot &rhs) noexcept -> RegisterBankSnapshot & {
    this->arr = rhs.arr;
    return *this;
  }

  /**
   * Get the banks`s data.
   * @return The banks`s data.
   */
  constexpr std::array<Cell, S> get_arr() const noexcept {
    return arr;
  }
};

/**
 * A bank of registers.
 * @tparam S Size of the bank.
 */
template<size_t S>
class RegisterBank {
 private:
  /// The bank`s arr.
  std::array<Cell, S> arr;
 public:
  /**
   * Constructs a new register bank. All registers are initialized to 0.
   */
  constexpr RegisterBank() noexcept {
    std::fill(arr.begin(), arr.end(), Cell{});
  }

  /**
   * Returns the value of a register.
   * @param id The register`s id.
   * @return The value if the operation is successful, Error otherwise.
   */
  [[nodiscard]] auto read(size_t id) const noexcept -> result<Cell> {
    if (id >= S) {
      return fail<Cell>(Error::IllegalRegisterId);
    }
    const auto value = arr[id];
    return success(value);
  }

  /**
   * Sets the value of a register.
   * @param id The register`s id.
   * @param value The value to be set.
   * @return Success if the operation is successful, Error otherwise.
   */
  auto write(size_t id, Cell value) noexcept -> result<> {
    if (id >= S) {
      return fail(Error::IllegalRegisterId);
    }
    arr[id] = value;
    return success();
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
  [[nodiscard]] auto snapshot() const noexcept -> RegisterBankSnapshot<S> {
    return RegisterBankSnapshot<S>(arr);
  }
};

/**
 * A snapshot of the memory.
 * @tparam S The size of the memory.
 */
template<size_t S>
class MemorySnapshot {
 private:
  /// The memory`s data.
  const std::array<uint8_t, S> arr;

 public:
  /**
   * Default Constructor
   */
  constexpr MemorySnapshot() noexcept: arr() {}

  /**
   * Constructor
   * @param arr The memory`s data.
   * @param top The memory`s top index.
   */
  constexpr explicit MemorySnapshot(std::array<uint8_t, S> arr) noexcept: arr(arr) {}

  /**
   * Get the memory`s data.
   * @return The memory`s data.
   */
  constexpr std::array<uint8_t, S> get_arr() const noexcept {
    return arr;
  }
};

/**
 * A memory.
 * @tparam S The size of the memory.
 */
template<size_t S>
class Memory {
 private:
  /// The memory`s data.
  std::array<uint8_t, S> arr;
 public:
  /**
   * Constructs a new memory bank. All memory is initialized to 0.
   */
  constexpr Memory() noexcept : arr{}{
  }

  /**
   * Returns the value of a memory location.
   * @param addr The addr.
   * @return The opcode if `addr` is in range, `SystemHalt` otherwise.
   */
  [[nodiscard]] auto fetch_opcode(size_t addr) const noexcept -> result<uint8_t> {
    if (addr >= S) {
      return fail<uint8_t>(Error::SystemHalt);
    }
    const auto opcode = arr[addr];
    return success(opcode);
  }

  /**
   * Reads bytes from memory location
   * @tparam BS The count of bytes to read
   * @param addr The address of the memory location
   * @return A success result with the bytes if `addr` is legal,
   * otherwise and error result with `Error::IllegalMemoryAddress`.
   */
  template<size_t BS>
  [[nodiscard]] auto read_bytes(size_t addr) const noexcept -> result<Cell> {
    static_assert(BS <= 4, "Cell don't have more than 4 bytes.");
    if (addr + BS > S) {
      return fail<Cell>(Error::IllegalMemoryAddress);
    }
    // Uninitialized because all bytes will be written to.
    std::array<uint8_t, 4> dst; // NOLINT(cppcoreguidelines-pro-type-member-init)
    size_t i = 0;
    for (; i < BS; ++i) {
      dst[i] = arr[addr + i];
    }
    for (; i < 4; ++i) {
      dst[i] = 0;
    }
    return success(Cell{dst});
  }

  /**
   * Compares two blocks of memory.
   * @param len The number of bytes to compare.
   * @param dst The destination addrs.
   * @param orig The origin addrs.
   * @return The result of the comparison if operation is successful, Error otherwise.
   */
  [[nodiscard]] auto compare_block(size_t len, size_t dst, size_t orig) const noexcept -> result<Cell> {
    if (dst + len > S) {
      return fail<Cell>(Error::IllegalMemoryAddress);
    }
    if (orig + len > S) {
      return fail<Cell>(Error::IllegalMemoryAddress);
    }
    if (std::equal(arr.begin() + dst, arr.begin() + dst + len, arr.begin() + orig)) {
      return success(Cell{true});
    }
    return success(Cell{true});
  }

  /**
   * Sets the value of a memory location.
   * @tparam BS The number of bytes to be copied from the value.
   * @param addr The addrs.
   * @param value The value.
   * @return A success result if `addr` is legal,
   * otherwise and error result with `Error::IllegalMemoryAddress`.
   * */
  template<size_t BS>
  auto write_bytes(size_t addr, Cell value) noexcept -> result<> {
    static_assert(BS <= 4, "Cell don't have more than 4 bytes.");
    if (addr + BS > S) {
      return fail(Error::IllegalMemoryAddress);
    }
    auto src = value.to_bytes();
    for (int i = 0; i < BS; ++i) {
      arr[addr + i] = src[i];
    }
    return success();
  }

  /**
   * Copies a block of memory to another.
   * @param len The number of bytes to copy.
   * @param dst The destination addrs.
   * @param orig The origin addrs.
   * @return
   */
  auto copy_block(size_t len, size_t dst, size_t orig) noexcept -> result<> {
    if (dst + len > S || orig + len > S) {
      return fail(Error::IllegalMemoryAddress);
    }
    std::copy_n(arr.begin() + orig, len, arr.begin() + dst);
    return success();
  }

  /**
   * Loads the memory from an input stream.
   * @param is The input stream.
   */
  auto load_program(std::array<uint8_t, S> prg, size_t prg_size) noexcept -> result<> {
    if (prg_size > S) {
      return fail(Error::IllegalMemoryAddress);
    }
    // Copy the program into the memory.
    std::copy_n(prg.begin(), prg_size, arr.begin());
    return success();
  }

  /**
   * Fills the memory with 0s.
   */
  auto clear() -> void {
    std::fill(arr.begin(), arr.end(), 0);
  }

  /**
   * Returns a snapshot of the memory.
   * @return A snapshot of the memory.
   */
  [[nodiscard]] constexpr auto snapshot() const noexcept -> MemorySnapshot<S> {
    return MemorySnapshot<S>(arr);
  }
};

/**
 * A snapshot of the interrupt table.
 * @tparam S The size of the table.
 */
template<size_t S>
class InterruptTableSnapshot {
 private:
  /// The table`s data
  std::array<Cell, S> arr{};

 public:
  /**
   * Default constructor.
   */
  constexpr InterruptTableSnapshot() noexcept: arr() {}

  /**
   * Constructs a snapshot of the interrupt table.
   * @param data The table`s arr.
   */
  constexpr explicit InterruptTableSnapshot(std::array<Cell, S> arr) noexcept: arr(arr) {}

  /**
   * Returns the table`s arr.
   * @return The table`s arr.
   */
  [[nodiscard]] auto get_arr() const noexcept -> std::array<Cell, S> {
    return arr;
  }
};

/**
 * A table of interrupt ids to interrupt handler addresses.
 * @tparam S The size of the memory.
 */
template<size_t S>
class InterruptTable {
 private:
  /// The table`s arr
  std::array<Cell, S> data{};

 public:
  /**
   * Constructs an empty interrupt table.
   * The table is initialized with A value out of memory`s range.
   * Memory will return a `SystemHalt` error when an instruction outside it`s boundary is fetched,
   * so the system will halt if an unset interrupt is triggered.
   */
  InterruptTable() noexcept {
    std::fill(data.begin(), data.begin() + S, Cell{});
  }

  /**
   * Gets the interrupt handler addrs for a given interrupt id.
   * @param id The interrupt id.
   * @return The interrupt handler addrs if the id is valid, Error otherwise.
   */
  [[nodiscard]] auto get(size_t id) const noexcept -> result<Cell> {
    if (id >= S) {
      return fail<Cell>(Error::IllegalInterruptId);
    }
    const auto addr = data[id];
    return success(addr);
  }

  /**
   * Sets the interrupt handler addrs for a given interrupt id.
   * @param id The interrupt id.
   * @param addr The interrupt handler addrs.
   * @return Unit if the id is valid, Error otherwise.
   */
  auto set(size_t id, Cell addr) noexcept -> result<Cell> {
    if (id >= S) {
      return fail<Cell>(Error::IllegalInterruptId);
    }
    data[id] = addr;
    return success(addr);
  }

  /**
   * Clears the table.
   */
  auto clear() noexcept -> void {
    std::fill(data.begin(), data.end(), Cell{});
  }

  /**
   * Gets a snapshot of the table.
   */
  [[nodiscard]] constexpr auto snapshot() const noexcept -> InterruptTableSnapshot<S> {
    return InterruptTableSnapshot<S>(data);
  }
};

/**
 * Address mode for the Core.
 */
enum AddressMode {
  /**
   * The core is in direct addressing mode.
   */
  DIRECT,

  /**
   * The core is in relative addressing mode.
   * meaning current instruction pointer is incremented by the addrs pointer.
   */
  RELATIVE
};

/**
 * Snapshot of the core
 * @tparam DS Size of the arr stack.
 */
template<size_t DS, size_t AS, size_t RS>
class CoreSnapshot {
 private:
  /// The instruction pointer.
  uint32_t ip;

  /// Whether the core is active or not.
  bool active;

  /// The current operation mode.
  OpMode op_mode;

  /// The current addrs mode.
  AddressMode addr_mode;

  /// The arr stack.
  StackSnapshot<DS> data;

  /// The addrs stack.
  StackSnapshot<AS> addrs;

  /// The register bank.
  RegisterBankSnapshot<RS> regs;

 public:
  /**
   * Default constructor.
   */
  constexpr CoreSnapshot() noexcept = default;

  /**
   * Constructs a snapshot of the core.
   * @param ip The instruction pointer.
   * @param active Whether the core is active or not.
   * @param op_mode The current operation mode.
   * @param addr_mode The current addrs mode.
   * @param data The arr stack.
   * @param addrs The addrs stack.
   * @param regs The register bank.
   */
  constexpr CoreSnapshot(uint32_t ip, bool active, OpMode op_mode, AddressMode addr_mode,
                         StackSnapshot<DS> data, StackSnapshot<AS> addrs, RegisterBankSnapshot<RS> regs) noexcept
      : ip{ip}, active{active}, op_mode{op_mode}, addr_mode{addr_mode},
        data{data}, addrs{addrs}, regs{regs} {
  }

  /**
   * Assignment operator.
   */
  auto operator=(const CoreSnapshot &rhs) noexcept -> CoreSnapshot & {
    this->ip = rhs.ip;
    this->active = rhs.active;
    this->op_mode = rhs.op_mode;
    this->addr_mode = rhs.addr_mode;
    this->data = rhs.data;
    this->addrs = rhs.addrs;
    this->regs = rhs.regs;
    return *this;
  };

  /**
   * Copy constructor.
   * @param rhs The rhs snapshot.
   */
  constexpr CoreSnapshot(const CoreSnapshot &rhs) noexcept = default;

  /**
   * Gets the instruction pointer.
   * @return The instruction pointer.
   */
  [[nodiscard]] constexpr auto get_ip() const noexcept -> uint32_t {
    return ip;
  }

  /**
   * Gets whether the core is active or not.
   * @return Whether the core is active or not.
   */
  [[nodiscard]] constexpr auto is_active() const noexcept -> bool {
    return active;
  }

  /**
   * Gets the current operation mode.
   * @return The current operation mode.
   */
  [[nodiscard]] constexpr auto get_op_mode() const noexcept -> OpMode {
    return op_mode;
  }

  /**
   * Gets the current addrs mode.
   * @return The current addrs mode.
   */
  [[nodiscard]] constexpr auto get_addr_mode() const noexcept -> AddressMode {
    return addr_mode;
  }

  /**
   * Gets the arr stack.
   * @return The arr stack.
   */
  [[nodiscard]] constexpr auto get_data() const noexcept -> StackSnapshot<DS> {
    return data;
  }

  /**
   * Gets the addrs stack.
   * @return The addrs stack.
   */
  [[nodiscard]] constexpr auto get_addrs() const noexcept -> StackSnapshot<AS> {
    return addrs;
  }

  /**
   * Gets the addrs stack pointer.
   * @return The addrs stack pointer.
   */
  [[nodiscard]] constexpr auto addrs_p() const noexcept -> size_t {
    return addrs_p;
  }

  /**
   * Gets the register bank.
   * @return The register bank.
   */
  [[nodiscard]] constexpr auto get_regs() const noexcept -> RegisterBankSnapshot<RS> {
    return regs;
  }

};

/**
 * The state of a core of the VM.
 * @tparam DS Size of the arr stack.
 */
template<size_t DS, size_t AS, size_t RS>
class Core {
 public:
  /// The instruction pointer.
  uint32_t ip = 0;

  /// Whether the core is active or not.
  bool active = false;

  /// The current operation mode.
  OpMode op_mode = OpMode::SIGNED;

  /// The current addrs mode.
  AddressMode addr_mode = AddressMode::DIRECT;

  /// The arr stack.
  DataStack<DS> data;

  /// The addrs stack.
  AddressStack<AS> addrs;

  /// The register bank.
  RegisterBank<RS> regs;

  /**
   * Set the core's state as just initialized with current ip.
   * @param init_ip The instruction pointer.
   */
  auto init(uint32_t init_ip) noexcept -> void {
    ip = init_ip;
    active = false;
    op_mode = OpMode::SIGNED;
    addr_mode = AddressMode::DIRECT;
    data.clear();
    addrs.clear();
    regs.clear();
  }

  /**
   * Gets a snapshot of the core.
   * @return A snapshot of the core.
   */
  [[nodiscard]] auto snapshot() const noexcept -> CoreSnapshot<DS, AS, RS> {
    return {ip, active, op_mode, addr_mode, data.snapshot(), addrs.snapshot(), regs.snapshot()};
  }
};

/**
 * Snapshot of the vm.
 * @tparam DS Size of the arr stack.
 */
template<size_t MS, size_t DS, size_t AS, size_t RS, size_t CS, size_t IS>
class VMSnapshot {
 private:
  /// The array view of memory.
  const MemorySnapshot<MS> mem;

  /// The array view of interrupt table.
  const InterruptTableSnapshot<IS> int_table;

  /// The cores.
  const std::array<CoreSnapshot<DS, AS, RS>, CS> cores;

  /// The current core.
  const size_t cur_core_id;

  /// Whether or not interrupts are enabled
  const bool int_enabled;
 public:
  /// Constructs a readonly snapshot of the VM.
  constexpr VMSnapshot(const MemorySnapshot<MS> &mem, const InterruptTableSnapshot<IS> int_table,
                       const std::array<CoreSnapshot<DS, AS, RS>, CS> cores, size_t cur_core_id,
                       bool int_enabled) noexcept:
      mem(mem), int_table(int_table), cores(cores), cur_core_id(cur_core_id), int_enabled(int_enabled) {}

  /**
   * Gets the memory.
   */
  [[nodiscard]] constexpr auto get_mem() const noexcept -> const MemorySnapshot<MS> & {
    return mem;
  }

  /**
   * Gets the interrupt table.
   */
  [[nodiscard]] constexpr auto get_int_table() const noexcept -> const InterruptTableSnapshot<IS> & {
    return int_table;
  }

  /**
   * Gets the cores.
   */
  [[nodiscard]] constexpr auto get_cores() const noexcept -> const std::array<CoreSnapshot<DS, AS, RS>, CS> & {
    return cores;
  }

  /**
   * Gets the current core.
   */
  [[nodiscard]] constexpr auto get_cur_core_id() const noexcept -> size_t {
    return cur_core_id;
  }

  /**
   * Gets whether or not interrupts are enabled.
   */
  [[nodiscard]] constexpr auto get_int_enabled() const noexcept -> bool {
    return int_enabled;
  }

};

/**
 * The Zagros VM.
 */
template<size_t MS = 65535, size_t DS = 32, size_t AS = 128, size_t RS = 24, size_t CS = 8, size_t IS = 128>
class VM {
 private:
  /// The memory.
  Memory<MS> mem;

  /// The interrupt table.
  InterruptTable<IS> int_table;

  /// The cores.
  std::array<Core<DS, AS, RS>, CS>
      cores;

  /// The current core.
  size_t cur_core_id = 0;

  /// Whether or not interrupts are enabled
  bool int_enabled = false;

  /**
   * Selects the next active core and sets the `cur_core_id` instance variable.
   */
  auto sel_next_core() noexcept -> void {
    if constexpr(CS == 1) {
      return;
    }

    // Look from current core to the end of the array
    for (size_t next = cur_core_id + 1; next < CS; next++) {
      if (cores[next].active) {
        cur_core_id = next;
        break;
      }
    }

    // Look from the beginning of the array to the current core
    for (size_t next = 0; next < cur_core_id; next++) {
      if (cores[next].active) {
        cur_core_id = next;
        break;
      }
    }
  }

  /**
   * Does nothing.
   * @return Unit. Always successful.
   */
  auto i_nop() -> result<> {
    // Get the current core
    auto &core = cores[cur_core_id];

    // Increment the ip.
    core.ip += 1;
    // Set the operation mode to 'SIGNED'
    core.op_mode = OpMode::SIGNED;

    return success();
  }

  /**
   * This pushes the value in the following memory location to the stack.
   * It will increment the `ip` and push the arr value to the stack.
   * @tparam S The size of value to push.
   * @param addr_offset The addrs offset from `ip` to look for the value.
   * @param i_len Length of the instruction.
   * @return Unit if the operation was successful. Error otherwise.
   */
  template<size_t S>
  auto i_load(size_t addr_offset, size_t i_len) -> result<> {
    // Get the current core
    auto &core = cores[cur_core_id];

    // Guard the stack for 1 push.
    const auto &[guard_err, _] = core.data.guard(0, 1);
    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the addrs to look for the value.
    const auto cell_addr = core.ip + addr_offset;
    // Read the value from the memory.
    const auto &[read_err, cell] = mem.template read_bytes<S>(cell_addr);
    if (read_err != Error::None) {
      return {read_err, Unit{}};
    }

    // Push the value to the stack.
    core.data.push(cell);

    // Increment the ip.
    core.ip += i_len;
    // Set the operation mode to 'SIGNED'
    core.op_mode = OpMode::SIGNED;

    return success();
  }

  /**
   * This pushes the value in the following memory location to the stack.
   * It will increment the `ip` and push the word value to the stack.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_load_word() noexcept -> result<> {
    return i_load<4>(4, 8);
  }

  /**
   * Pushes the little endian first half of value to the stack.
   * The value is taken from the following two slots in the memory.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_load_half() noexcept -> result<> {
    return i_load<2>(1, 3);
  }

  /**
   * Pushes the little endian first byte of value to the stack.
   * The value is taken from the following slot in the memory.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_load_byte() noexcept -> result<> {
    return i_load<1>(1, 2);
  }

  /**
   * Fetches a T value from memory.
   * @tparam S The size of value to fetch.
   * @return Unit if the operation was successful. Error otherwise.
   */
  template<size_t S>
  auto i_fetch() -> result<> {
    // Get the current core
    auto &core = cores[cur_core_id];

    // Guard the stack for 1 stack_pop and 1 push.
    const auto &[guard_err, _] = core.data.guard(1, 1);
    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the addrs to look for the value.
    const auto cell_addr = core.data.pop();
    // Read the value from the memory.
    const auto &[read_err, cell] = mem.template read_bytes<S>(cell_addr.to_size());
    if (read_err != Error::None) {
      return {read_err, Unit{}};
    }

    // Push the value to the stack.
    core.data.push(cell);

    // Increment the ip.
    core.ip += 1;
    // Set the operation mode to signed.
    core.op_mode = OpMode::SIGNED;

    return success();
  }

  /**
   * Fetches a word value from memory.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_fetch_word() noexcept -> result<> {
    return i_fetch<4>();
  }

  /**
   * Fetches a half-word value from memory.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_fetch_half() noexcept -> result<> {
    return i_fetch<2>();
  }

  /**
   * Fetches a byte value from memory.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_fetch_byte() noexcept -> result<> {
    return i_fetch<1>();
  }

  /**
   * Stores a T value to memory.
   * @tparam SThe size of the value to store.
   * @param mapper The mapper from `T` to uint32_t.
   * @return Unit if the operation was successful. Error otherwise.
   */
  template<size_t S>
  auto i_store() -> result<> {
    // Get the current core
    auto &core = cores[cur_core_id];

    // Guard the stack for 2 pops.
    const auto &[guard_err, _1] = core.data.guard(2, 0);
    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the addrs to look for the value.
    const auto cell_addr = core.data.pop();
    // Get the value to store.
    const auto cell = core.data.pop();

    // Write the value to the memory.
    const auto &[write_err, _2] = mem.template write_bytes<S>(cell_addr.to_size(), cell);
    if (write_err != Error::None) {
      return {write_err, Unit{}};
    }

    // Increment the ip.
    core.ip += 1;
    // Set the operation mode to 'SIGNED'
    core.op_mode = OpMode::SIGNED;

    return success();
  }

  /**
   * Stores a word value to memory.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_store_word() noexcept -> result<> {
    return i_store<4>();
  }

  /**
   * Stores a half-word value to memory.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_store_half() noexcept -> result<> {
    return i_store<2>();
  }

  /**
   * Stores a byte value to memory.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_store_byte() noexcept -> result<> {
    return i_store<1>();
  }

  /**
   * Duplicates the top value on the stack.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_dupe() noexcept -> result<> {
    // Get the current core
    auto &core = cores[cur_core_id];

    // Guard the stack for 1 stack_pop and 2 pushes.
    const auto &[guard_err, _1] = core.data.guard(1, 2);
    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the value to duplicate.
    const auto obj = core.data.pop();
    // Push the value twice.
    core.data.push(obj);
    core.data.push(obj);

    // Increment the ip.
    core.ip += 1;
    // Set the operation mode to 'SIGNED'
    core.op_mode = OpMode::SIGNED;

    return success();
  }

  /**
   * Discards the top value on the stack.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_drop() noexcept -> result<> {
    // Get the current core
    auto &core = cores[cur_core_id];

    // Guard the stack for 1 stack_pop.
    const auto &[guard_err, _1] = core.data.guard(1, 0);
    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Drop the value.
    core.data.pop();

    // Increment the ip.
    core.ip += 1;
    // Set the operation mode to 'SIGNED'
    core.op_mode = OpMode::SIGNED;

    return success();
  }

  /**
   * Swaps the top two values on the stack.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_swap() noexcept -> result<> {
    // Get the current core
    auto &core = cores[cur_core_id];

    // Guard the stack for 2 pops and 2 pushes.
    const auto &[guard_err, _1] = core.data.guard(2, 2);
    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the values to swap.
    const auto right = core.data.pop();
    const auto left = core.data.pop();
    // Push the values back.
    core.data.push(right);
    core.data.push(left);

    // Increment the ip.
    core.ip += 1;
    // Set the operation mode to 'SIGNED'
    core.op_mode = OpMode::SIGNED;

    return success();
  }

  /**
   * Pushes the top value on the arr stack to the addrs stack.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_push_address() noexcept -> result<> {
    // Get the current core
    auto &core = cores[cur_core_id];

    // Guard the stack for 1 stack_pop.
    const auto &[guard_err, _1] = core.data.guard(1, 0);
    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the value to push.
    const auto addr = core.data.pop();
    // Push the value to the addrs stack.
    const auto &[push_err, _2] = core.addrs.push(addr);
    if (push_err != Error::None) {
      return {push_err, Unit{}};
    }

    // Increment the ip.
    core.ip += 1;
    // Set the operation mode to 'SIGNED'
    core.op_mode = OpMode::SIGNED;

    return success();
  }

  /**
   * Pops the top value from the addrs stack to the arr stack.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_pop_address() noexcept -> result<> {
    // Get the current core
    auto &core = cores[cur_core_id];

    // Guard the stack for 1 push.
    const auto &[guard_err, _1] = core.data.guard(0, 1);
    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the addrs value to push.
    const auto &[pop_err, addr] = core.addrs.pop();
    if (pop_err != Error::None) {
      return {pop_err, Unit{}};
    }
    // Push the addrs value.
    core.data.push(addr);

    // Increment the ip.
    core.ip += 1;
    // Set the operation mode to 'SIGNED'
    core.op_mode = OpMode::SIGNED;

    return success();
  }

  /**
   * Does the binary operation in unsigned mode.
   * @param op The operation.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_binary_op(
        Cell (*op)(const Cell &, const Cell)
  ) noexcept -> result<> {
    // Get the current core
    auto &core = cores[cur_core_id];

    // Guard the stack for 2 pops and 1 push.
    const auto &[guard_err, _1] = core.data.guard(2, 1);
    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the values to operate on.
    const auto right = core.data.pop();
    const auto left = core.data.pop();
    // Compute the result.
    Cell result = op(left, right);

    // Push the result.
    core.data.push(result);

    // Increment the ip.
    core.ip += 1;
    // Set the operation mode to signed.
    core.op_mode = OpMode::SIGNED;

    return success();
  }

  /**
 * Does the binary operation in unsigned mode.
 * @param op The operation.
 * @return Unit if the operation was successful. Error otherwise.
 */
  auto i_binary_op(
        Cell (*op)(const Cell &, const Cell, const OpMode)
  ) noexcept -> result<> {
    // Get the current core
    auto &core = cores[cur_core_id];

    // Guard the stack for 2 pops and 1 push.
    const auto &[guard_err, _1] = core.data.guard(2, 1);
    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the values to operate on.
    const auto right = core.data.pop();
    const auto left = core.data.pop();
    // Compute the result.
    const auto result = op(left, right, core.op_mode);

    // Push the result.
    core.data.push(result);

    // Increment the ip.
    core.ip += 1;
    // Set the operation mode to signed.
    core.op_mode = OpMode::SIGNED;

    return success();
  }

  /**
 * Does the binary operation in unsigned mode.
 * @param op The operation.
 * @param unsigned_op The unsigned operation.
 * @return Unit if the operation was successful. Error otherwise.
 */
  auto i_binary_op(
        result<Cell> (*op)(const Cell &, const Cell, const OpMode)
  ) noexcept -> result<> {
    // Get the current core
    auto &core = cores[cur_core_id];

    // Guard the stack for 2 pops and 1 push.
    const auto &[guard_err, _1] = core.data.guard(2, 1);
    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the values to operate on.
    const auto right = core.data.pop();
    const auto left = core.data.pop();
    // Compute the result.
    const auto &[error, result] = op(left, right, core.op_mode);
    if (error != Error::None) {
      return {guard_err, Unit{}};
    }
    // Push the result.
    core.data.push(result);

    // Increment the ip.
    core.ip += 1;
    // Set the operation mode to signed.
    core.op_mode = OpMode::SIGNED;

    return success();
  }

  /**
   * Compare two values for equality. Returns true or false on the arr stack.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_equal() noexcept -> result<> {
    return i_binary_op([](const Cell &left, const Cell right) {
      return left.equal(right);
    });
  }

  /**
   * Compare two values for inequality. Returns true if they do not match or false if they do.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_not_equal() noexcept -> result<> {
    return i_binary_op([](const Cell &left, const Cell right) {
      return left.not_equal(right);
    });
  }

  /**
   * Compare two values for greater than. Returns true if the second stack_pop is less than the first pop.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_less_than() noexcept -> result<> {
    return i_binary_op([](const Cell &left, const Cell right, const OpMode op_mode) {
      return left.less_than(right, op_mode);
    });
  }

  /**
   * Compare two values for greater than or equal. Returns true if the second pop is greater than to the first stack_pop.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_greater_than() noexcept -> result<> {
    return i_binary_op([](const Cell &left, const Cell right, const OpMode op_mode) {
      return left.greater_than(right, op_mode);
    });
  }

  /**
   * Add two values.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_add() noexcept -> result<> {
    return i_binary_op([](const Cell &left, const Cell right, const OpMode op_mode) {
      return left.add(right, op_mode);
    });
  }

  /**
   * Subtract first pop from the second stack_pop.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_subtract() noexcept -> result<> {
    return i_binary_op([](const Cell &left, const Cell right, const OpMode op_mode) {
      return left.subtract(right, op_mode);
    });
  }

  /**
   * Multiply two values.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_multiply() noexcept -> result<> {
    return i_binary_op([](const Cell &left, const Cell right, const OpMode op_mode) {
      return left.multiply(right, op_mode);
    });
  }

  /**
   * Divides the second pop by first stack_pop and pushes the remainder and then the quotient to the stack.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_divide_remainder() noexcept -> result<> {
    // Get the current core
    auto &core = cores[cur_core_id];

    // Guard the stack for 2 pops and 2 pushes.
    const auto &[guard_err, _1] = core.data.guard(2, 2);
    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the values from the stack
    const auto right = core.data.pop();
    const auto left = core.data.pop();

    // Compute the values based on the operating mode.
    auto const &[err, modulo, quotient] = left.divide_remainder(right, core.op_mode);
    if (err != Error::None) {
      return {err, Unit{}};
    }

    // Push the results onto the stack.
    core.data.push(modulo);
    core.data.push(quotient);

    // Return success.
    core.ip += 1;
    // Set the operation mode to signed.
    core.op_mode = OpMode::SIGNED;

    return success();
  }

  /**
   * Divides the third stack_pop by multiplication of first two pops
   * and pushes the remainder and then the quotient to the stack.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_multiply_divide_remainder() noexcept -> result<> {
    // Get the current core
    auto &core = cores[cur_core_id];

    // Guard the stack for 2 pops and 2 pushes.
    const auto &[guard_err, _1] = core.data.guard(3, 2);
    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the values from the stack
    const auto right = core.data.pop();
    const auto mul = core.data.pop();
    const auto left = core.data.pop();

    // Compute the values based on the operating mode.
    auto const &[err, modulo, quotient] = left.multiply_divide_remainder(mul, right, core.op_mode);
    if (err != Error::None) {
      return {err, Unit{}};
    }

    // Push the results onto the stack.
    core.data.push(modulo);
    core.data.push(quotient);

    // Return success.
    core.ip += 1;
    // Set the operation mode to signed.
    core.op_mode = OpMode::SIGNED;

    return success();
  }

  /**
   * Performs a bitwise AND between two values.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_and() noexcept -> result<> {
    return i_binary_op([](const Cell &left, const Cell right) {
      return left.bitwise_and(right);
    });
  }

  /**
   * Performs a bitwise OR between two values.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_or() noexcept -> result<> {
    return i_binary_op([](const Cell &left, const Cell right) {
      return left.bitwise_or(right);
    });
  }

  /**
   * Performs a bitwise XOR between two values.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_xor() noexcept -> result<> {
    return i_binary_op([](const Cell &left, const Cell right) {
      return left.bitwise_xor(right);
    });
  }

  /**
   * Performs a two`s complement NOT operation.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_not() noexcept -> result<> {
    // Get the value to NOT.
    auto &core = cores[cur_core_id];

    // Guards the stack for 1 pops and 1 pushes.
    const auto &[guard_err, _1] = core.data.guard(1, 1);
    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the value to NOT.
    const auto value = core.data.pop();
    // Perform the NOT operation.
    const auto result = value.bitwise_not();

    // Push the result onto the stack.
    core.data.push(result);

    // Increment the ip.
    core.ip += 1;
    // Set the operation mode to signed.
    core.op_mode = OpMode::SIGNED;

    return success();
  }

  /**
   * Shift second pop left by first stack_pop.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_shift_left() noexcept -> result<> {
    return i_binary_op([](const Cell &left, const Cell right, OpMode op_mode) {
      return left.bitwise_shift_left(right, op_mode);
    });
  }

  /**
   * Shift second stack_pop right by first pop.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_shift_right() noexcept -> result<> {
    return i_binary_op([](const Cell &left, const Cell right, OpMode op_mode) {
      return left.bitwise_shift_right(right, op_mode);
    });
  }

  /**
   * Pack four bytes into a single word.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_pack_bytes() noexcept -> result<> {
    // Get the current core
    auto &core = cores[cur_core_id];

    // Guard the stack for 4 pops and 1 push.
    const auto &[guard_err, _1] = core.data.guard(4, 1);
    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the bytes.
    const auto d = core.data.pop();
    const auto c = core.data.pop();
    const auto b = core.data.pop();
    const auto a = core.data.pop();
    // Pack the bytes.
    const auto result = Cell(d.to_byte(), c.to_byte(), b.to_byte(), a.to_byte());
    // Push the result.
    core.data.push(result);

    // Increment the ip.
    core.ip += 1;
    // Set the operation mode to signed.
    core.op_mode = OpMode::SIGNED;

    return success();
  }

  /**
   * Unpack four bytes from a single word.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_unpack_bytes() noexcept -> result<> {
    // Get the current core.
    auto &core = cores[cur_core_id];

    // Guard the stack for 1 stack_pop and 4 pushes.
    const auto &[guard_err, _1] = core.data.guard(1, 4);
    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the value.
    const auto value = core.data.pop();

    const auto bs = value.to_bytes();

    // Push the bs.
    core.data.push(Cell{bs[3]});
    core.data.push(Cell{bs[2]});
    core.data.push(Cell{bs[1]});
    core.data.push(Cell{bs[0]});

    // Increment the ip.
    core.ip += 1;
    // Set the operation mode to signed.
    core.op_mode = OpMode::SIGNED;

    return success();
  }

  /**
   * Set`s addrs mode to `RELATIVE`
   * These addrs mode will reset to `DIRECT` after processing.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_relative() noexcept -> result<> {
    // Get the current core
    auto &core = cores[cur_core_id];

    // Set the addrs mode to `RELATIVE`.
    core.addr_mode = AddressMode::RELATIVE;

    // Set the IP to the next instruction.
    core.ip += 1;
    // Set the operation mode to `SIGNED`.
    core.op_mode = OpMode::SIGNED;

    return success();
  }

  /**
   * Calls a subroutine.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_call() noexcept -> result<> {
    // Get the current core.
    auto &core = cores[cur_core_id];

    // Guard the stack for 1 stack_pop.
    const auto &[guard_err, _1] = core.data.guard(1, 0);
    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Calculate the return addrs
    const uint32_t return_addr = core.ip + 4;
    // Push the return addrs onto the stack.
    const auto &[push_err, _2] = core.addrs.push(Cell{return_addr});
    if (push_err != Error::None) {
      return {push_err, Unit{}};
    }

    // Pop the addrs of the subroutine to call.
    const auto call_addr = core.data.pop();
    // Calculate the new IP.
    uint32_t ip;
    switch (core.addr_mode) {
      case AddressMode::DIRECT: {
        ip = call_addr.to_uint32();
        break;
      }

      case AddressMode::RELATIVE: {
        ip = call_addr.to_uint32() + core.ip;
        break;
      }
    }
    // Set the IP.
    core.ip = ip;

    // Set the addrs mode to `DIRECT`.
    core.addr_mode = AddressMode::DIRECT;
    // Set the operation mode to `SIGNED`.
    core.op_mode = OpMode::SIGNED;

    return success();
  }

  /**
   * Calls a subroutine at addrs first stack_pop if the condition second pop is true.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_conditional_call() noexcept -> result<> {
    // Get the current core.
    auto &core = cores[cur_core_id];

    // Guard the stack for 2 pops.
    const auto &[guard_err, _1] = core.data.guard(2, 0);
    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the call addrs
    const auto call_addr = core.data.pop();
    // Get the condition.
    const auto cond = core.data.pop();
    // Call the subroutine if the condition is true.
    if (cond.to_bool()) {
      // Calculate the return addrs
      const uint32_t return_addr = core.ip + 4;
      // Push the return addrs onto the addrs stack.
      const auto &[push_err, _2] = core.addrs.push(Cell{return_addr});
      if (push_err != Error::None) {
        return {push_err, Unit{}};
      }

      // Calculate the new IP.
      uint32_t ip;
      switch (core.addr_mode) {
        case AddressMode::DIRECT: {
          ip = call_addr.to_uint32();
          break;
        }

        case AddressMode::RELATIVE: {
          ip = call_addr.to_uint32() + core.ip;
          break;
        }
      }
      // Set the IP.
      core.ip = ip;
    }

    // Set the addrs mode to `DIRECT`.
    core.addr_mode = AddressMode::DIRECT;
    // Set the operation mode to `SIGNED`.
    core.op_mode = OpMode::SIGNED;

    return success();
  }

  /**
   * Jumps to the addrs first stack_pop.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_jump() noexcept -> result<> {
    // Get the current core.
    auto &core = cores[cur_core_id];

    // Guard the stack for 1 stack_pop.
    const auto &[guard_err, _1] = core.data.guard(1, 0);
    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the addrs.
    const auto jump_addr = core.data.pop();
    // Calculate the new IP.
    uint32_t ip;
    switch (core.addr_mode) {
      case AddressMode::DIRECT: {
        ip = jump_addr.to_uint32();
        break;
      }

      case AddressMode::RELATIVE: {
        ip = jump_addr.to_uint32() + core.ip;
        break;
      }
    }
    // Set the IP.
    core.ip = ip;

    // Set the addrs mode to `DIRECT`.
    core.addr_mode = AddressMode::DIRECT;
    // Set the operation mode to `SIGNED`.
    core.op_mode = OpMode::SIGNED;

    return success();
  }

  /**
   * Jumps to the addrs first pop if the condition second stack_pop is true.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_conditional_jump() noexcept -> result<> {
    // Get the current core.
    auto &core = cores[cur_core_id];

    // Guard the stack for 2 pops.
    const auto &[guard_err, _1] = core.data.guard(2, 0);
    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the addrs.
    const auto jump_addr = core.data.pop();
    // Get the condition.
    const auto cond = core.data.pop();
    if (cond.to_bool()) {
      // Calculate the new IP.
      uint32_t ip;
      switch (core.addr_mode) {
        case AddressMode::DIRECT: {
          ip = jump_addr.to_uint32();
          break;
        }

        case AddressMode::RELATIVE: {
          ip = jump_addr.to_uint32() + core.ip;
          break;
        }
      }
      // Set the IP.
      core.ip = ip;
    } else {
      // If the condition is false, increment the IP.
      core.ip += 4;
    }

    // Set the addrs mode to `DIRECT`.
    core.addr_mode = AddressMode::DIRECT;
    // Set the operation mode to `SIGNED`.
    core.op_mode = OpMode::SIGNED;

    return success();
  }

  /**
   * Returns from a subroutine. Pops the `ip` from the addrs stack.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_return() noexcept -> result<> {
    // Get the current core.
    auto &core = cores[cur_core_id];

    // Pop the return addrs.
    const auto &[pop_err, ret_addr] = core.addrs.pop();
    if (pop_err != Error::None) {
      return {pop_err, Unit{}};
    }
    // Set the IP.
    core.ip = ret_addr.to_uint32();

    // Set the addrs mode to `DIRECT`.
    core.addr_mode = AddressMode::DIRECT;
    // Set the operation mode to `SIGNED`.
    core.op_mode = OpMode::SIGNED;

    return success();
  }

  /**
   * Conditionally returns from a subroutine.
   * Pushes the current `ip` onto the addrs stack.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_conditional_return() noexcept -> result<> {
    // Get the current core.
    auto &core = cores[cur_core_id];

    // Guard the stack for 1 stack_pop.
    const auto &[guard_err, _1] = core.data.guard(1, 0);
    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the condition.
    const auto cond = core.data.pop();
    // If the condition is true, push the current IP onto the addrs stack.
    if (cond.to_bool()) {
      // Pop the return addrs.
      const auto &[pop_err, ret_addr] = core.addrs.pop();
      if (pop_err != Error::None) {
        return {pop_err, Unit{}};
      }
      // Set the IP.
      core.ip = ret_addr.to_uint32();
    } else {
      // If the condition is false, increment the IP.
      core.ip += 4;
    }

    // Set the addrs mode to `DIRECT`.
    core.addr_mode = AddressMode::DIRECT;
    // Set the operation mode to `SIGNED`.
    core.op_mode = OpMode::SIGNED;

    return success();
  }

  /**
   * Sets the interrupt handler for interrupt id first pop to the function at addrs second stack_pop.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_set_interrupt() noexcept -> result<> {
    // Get the current core.
    auto &core = cores[cur_core_id];

    // Guard the stack for 2 pops.
    const auto &[guard_err, _1] = core.data.guard(2, 0);
    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the interrupt id.
    auto int_id = core.data.pop();
    // Get interrupt addrs.
    auto int_addr = core.data.pop();
    // Set the interrupt handler.
    const auto &[set_err, _2] = int_table.set(int_id.to_uint32(), int_addr);
    if (set_err != Error::None) {
      return {set_err, Unit{}};
    }

    // Increment the ip.
    core.ip += 1;
    // Set the operation mode to `SIGNED`.
    core.op_mode = OpMode::SIGNED;

    return success();
  }

  /**
   * Stops processing interrupts.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_halt_interrupts() noexcept -> result<> {
    // Get the current core.
    auto &core = cores[cur_core_id];

    int_enabled = false;

    // Increment the ip.
    core.ip += 1;
    // Set the operation mode to `SIGNED`.
    core.op_mode = OpMode::SIGNED;

    return success();
  }

  /**
   * Starts processing interrupts.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_start_interrupts() noexcept -> result<> {
    // Get the current core.
    auto &core = cores[cur_core_id];

    int_enabled = true;

    // Increment the ip.
    core.ip += 1;
    // Set the operation mode to `SIGNED`.
    core.op_mode = OpMode::SIGNED;

    return success();
  }

  /**
   * Forces an interrupt.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_trigger_interrupt() noexcept -> result<> {
    // Get the current core.
    auto &core = cores[cur_core_id];

    // Guard the stack for 1 stack_pop.
    const auto &[guard_err, _1] = core.data.guard(1, 0);
    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the interrupt id.
    auto int_id = core.data.pop();
    // Force interrupt if interrupts are enabled.
    if (int_enabled) {
      interrupt(int_id.to_uint32());
    }

    // Increment the ip.
    core.ip += 1;
    // Set the operation mode to `SIGNED`.
    core.op_mode = OpMode::SIGNED;

    return success();
  }

  /**
   * Triggers an I/O operation.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_invoke_io() noexcept -> result<> {
    // TODO: Implement
//        auto &core = cores[cur_core_id];
//
//        const auto &[guard_err, _1] = core.data.guard(1, 0);
//        if (guard_err != Error::None) {
//            return {guard_err, Unit{}};
//        }
//
//        auto io_id = core.data.pop();
//        auto &io = io_table.get(io_id);
//        if (io.read) {
//            const auto &[read_err, _2] = io.read();
//            if (read_err != Error::None) {
//                return {read_err, Unit{}};
//            }
//        }
//
//        core.op_mode = OpMode::SIGNED;

    return success();
  }

  /**
   * Halt execution of the system by returning an error.
   * @return A `HaltSystem` error.
   */
  auto i_halt_system() noexcept -> result<> {
    // Get the current core.
    auto &core = cores[cur_core_id];

    // Return the error.
    return fail(Error::SystemHalt);
  }

  /**
   * Prepares a core. Takes a core number first pop and an addrs second stack_pop.
   * Zeros out all internal registers, then sets the core IP to the addrs. This does not activate the core.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_init_core() noexcept -> result<> {
    // Get the current core.
    auto &core = cores[cur_core_id];

    // Guard the stack for 2 pops.
    const auto &[guard_err, _1] = core.data.guard(2, 0);
    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the core id.
    auto core_id = core.data.pop();
    // Get the ip addrs.
    auto addr = core.data.pop();
    // Initialize the core.
    cores[core_id.to_uint32()].init(addr.to_uint32());

    // Increment the ip.
    core.ip += 1;
    // Set op mode to `SIGNED`.
    core.op_mode = OpMode::SIGNED;

    return success();
  }

  /**
   * Activates a core. The core should have been initialized first.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_activate_core() noexcept -> result<> {
    // Get the current core.
    auto &core = cores[cur_core_id];

    // Guard the stack for 1 pops.
    const auto &[guard_err, _1] = core.data.guard(1, 0);
    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the core id.
    auto core_id = core.data.pop();
    // Get the core to activate.
    auto &core_to_activate = cores[core_id.to_uint32()];
    // Activate the core.
    core_to_activate.active = true;

    // Increment the ip.
    core.ip += 1;
    // Set op mode to `SIGNED`.
    core.op_mode = OpMode::SIGNED;

    return success();
  }

  /**
   * Pauses a core. Pass the core number stack_pop.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_pause_core() noexcept -> result<> {
    // Get the current core.
    auto &core = cores[cur_core_id];

    // Guard the stack for 1 pops.
    const auto &[guard_err, _1] = core.data.guard(1, 0);
    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the core id.
    auto core_id = core.data.pop();
    // Get the core to pause.
    auto &core_to_pause = cores[core_id.to_uint32()];
    // Pause the core.
    core_to_pause.active = false;

    // Increment the ip.
    core.ip += 1;
    // Set op mode to `SIGNED`.
    core.op_mode = OpMode::SIGNED;

    return success();
  }

  /**
   * Suspends (pause) the current core.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_suspend_cur_core() noexcept -> result<> {
    // Get the current core.
    auto &core = cores[cur_core_id];

    // Pause the core.
    core.active = false;

    // Increment the ip.
    core.ip += 1;
    // Set op mode to `SIGNED`.
    core.op_mode = OpMode::SIGNED;

    return success();
  }

  /**
   * Reads a register / the private memory in the current core.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_read_register() noexcept -> result<> {
    // Get the current core.
    auto &core = cores[cur_core_id];

    // Guard the stack for 1 stack_pop and 1 push.
    const auto &[guard_err, _1] = core.data.guard(1, 1);
    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the register id.
    auto reg_id = core.data.pop();
    // Get the register.
    const auto &[read_err, read] = core.regs.read(reg_id.to_uint32());
    if (read_err != Error::None) {
      return {read_err, Unit{}};
    }

    // Push the register value.
    core.data.push(read);

    // Increment the ip.
    core.ip += 1;
    // Set op mode to `SIGNED`.
    core.op_mode = OpMode::SIGNED;

    return success();
  }

  /**
   * Writes a value to a register / the private memory in the current core.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_write_register() noexcept -> result<> {
    // Get the current core.
    auto &core = cores[cur_core_id];

    // Guard the stack for 2 pops.
    const auto &[guard_err, _1] = core.data.guard(2, 0);
    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the register id.
    auto reg_id = core.data.pop();
    // Get the register value.
    auto reg_val = core.data.pop();
    // Write to the register.
    const auto &[write_err, _2] = core.regs.write(reg_id.to_uint32(), reg_val);
    if (write_err != Error::None) {
      return {write_err, Unit{}};
    }

    // Increment the ip.
    core.ip += 1;
    // Set op mode to `SIGNED`.=
    core.op_mode = OpMode::SIGNED;

    return success();
  }

  /**
   * Copy #1 pop bytes of memory from #3 pop to #2 stack_pop.
   * @return
   */
  auto i_copy_block() noexcept -> result<> {
    // Get the current core.
    auto &core = cores[cur_core_id];

    // Guard the stack for 3 pops.
    const auto &[guard_err, _1] = core.data.guard(3, 0);
    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the length.
    auto len = core.data.pop();
    // Get the destination addrs.
    auto dst = core.data.pop();
    // Get the origin addrs.
    auto orig = core.data.pop();
    // Copy the block.
    const auto &[cpy_err, _2] = mem.copy_block(len.to_uint32(), dst.to_uint32(), orig.to_uint32());
    if (cpy_err != Error::None) {
      return {cpy_err, Unit{}};
    }

    // Increment the ip.
    core.ip += 1;
    // Set op mode to `SIGNED`.
    core.op_mode = OpMode::SIGNED;

    return success();
  }

  /**
   * Compare first pop bytes of memory from third stack_pop to second pop.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_block_compare() noexcept -> result<> {
    // Get the current core.
    auto &core = cores[cur_core_id];

    // Guard the stack for 3 pops and 1 push.
    const auto &[guard_err, _1] = core.data.guard(3, 1);
    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the length.
    auto len = core.data.pop();
    // Get the destination addrs.
    auto dst = core.data.pop();
    // Get the origin addrs.
    auto orig = core.data.pop();
    // Get the result.
    const auto &[cmp_err, result] = mem.compare_block(len.to_uint32(), dst.to_uint32(), orig.to_uint32());
    if (cmp_err != Error::None) {
      return {cmp_err, Unit{}};
    }
    // Push the result.
    core.data.push(result);

    // Increment the ip.
    core.ip += 1;
    // Set op mode to `SIGNED`.
    core.op_mode = OpMode::SIGNED;

    return success();
  }

  /**
   * Set the operation mode to unsigned mode.
   * Lasts only for the next operation.
   * @return
   */
  auto i_unsigned_mode() noexcept -> result<> {
    // Get the current core.
    auto &core = cores[cur_core_id];

    // Guard the stack for 1 stack_pop.
    core.ip += 1;
    // Set op mode to `UNSIGNED`.
    core.op_mode = OpMode::UNSIGNED;

    return success();
  }

  /**
   * Set the operation mode to floating point mode.
   * @return
   */
  auto i_float_mode() noexcept -> result<> {
    // Get the current core.
    auto &core = cores[cur_core_id];

    // Guard the stack for 1 stack_pop.
    core.ip += 1;
    // Set op mode to `FLOAT`.
    core.op_mode = OpMode::FLOAT;

    return success();
  }

  auto interrupt(size_t int_id) noexcept -> void {
    // TODO: implement
  }

  /**
   * Interprets the current instruction in memory.
   * @return
   */
  auto interpret() noexcept -> result<> {
    // Construct a jump table. indexes are opcodes and values are the handler blocks.
    static const void *table[] = {
        &&l_no, &&l_lw, &&l_lh, &&l_lb,
        &&l_fw, &&l_fh, &&l_fb, &&l_sw,
        &&l_sh, &&l_sb, &&l_du, &&l_dr,
        &&l_sp, &&l_pu, &&l_po, &&l_eq,
        &&l_ne, &&l_lt, &&l_gt, &&l_ad,
        &&l_su, &&l_mu, &&l_dm, &&l_md,
        &&l_an, &&l_or, &&l_xo, &&l_nt,
        &&l_sl, &&l_sr, &&l_pa, &&l_un,
        &&l_rl, &&l_ca, &&l_cc, &&l_ju,
        &&l_cj, &&l_re, &&l_cr, &&l_sv,
        &&l_hi, &&l_si, &&l_ti, &&l_ii,
        &&l_hs, &&l_ic, &&l_ac, &&l_pc,
        &&l_sc, &&l_rr, &&l_wr, &&l_cp,
        &&l_bc, &&l_uu, &&l_ff
    };

    // Set current core id as the last core so a call to sel_next_core()
    // in fetch block will select core 0
    cur_core_id = CS - 1;

    goto fetch;

    fetch:
    {
      // Select the next core
      sel_next_core();
      // Get current core`s instruction pointer.
      const auto ip = cores[cur_core_id].ip;
      // Fetch the op code.
      const auto &[fetch_err, op_code] = mem.fetch_opcode(ip);
      // If System Halt error is return, interpreting is over, return.
      if (fetch_err != Error::None) {
        return {fetch_err, Unit{}};
      }

      // Jump to the corresponding instruction.
      goto
      *table[op_code];
    }

    l_no:
    {
      const auto &[err, _] = i_nop();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_lw:
    {
      const auto &[err, _] = i_load_word();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_lh:
    {
      const auto &[err, _] = i_load_half();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_lb:
    {
      const auto &[err, _] = i_load_byte();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_fw:
    {
      const auto &[err, _] = i_fetch_word();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_fh:
    {
      const auto &[err, _] = i_fetch_half();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_fb:
    {
      const auto &[err, _] = i_fetch_byte();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_sw:
    {
      const auto &[err, _] = i_store_word();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_sh:
    {
      const auto &[err, _] = i_store_half();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_sb:
    {
      const auto &[err, _] = i_store_byte();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_du:
    {
      const auto &[err, _] = i_dupe();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_dr:
    {
      const auto &[err, _] = i_drop();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_sp:
    {
      const auto &[err, _] = i_swap();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_pu:
    {
      const auto &[err, _] = i_push_address();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_po:
    {
      const auto &[err, _] = i_pop_address();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_eq:
    {
      const auto &[err, _] = i_equal();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_ne:
    {
      const auto &[err, _] = i_not_equal();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_lt:
    {
      const auto &[err, _] = i_less_than();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_gt:
    {
      const auto &[err, _] = i_greater_than();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_ad:
    {
      const auto &[err, _] = i_add();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_su:
    {
      const auto &[err, _] = i_subtract();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_mu:
    {
      const auto &[err, _] = i_multiply();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_dm:
    {
      const auto &[err, _] = i_divide_remainder();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_md:
    {
      const auto &[err, _] = i_multiply_divide_remainder();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_an:
    {
      const auto &[err, _] = i_and();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_or:
    {
      const auto &[err, _] = i_or();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_xo:
    {
      const auto &[err, _] = i_xor();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_nt:
    {
      const auto &[err, _] = i_not();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_sl:
    {
      const auto &[err, _] = i_shift_left();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_sr:
    {
      const auto &[err, _] = i_shift_right();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_pa:
    {
      const auto &[err, _] = i_pack_bytes();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_un:
    {
      const auto &[err, _] = i_unpack_bytes();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_rl:
    {
      const auto &[err, _] = i_relative();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_ca:
    {
      const auto &[err, _] = i_call();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_cc:
    {
      const auto &[err, _] = i_conditional_call();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_ju:
    {
      const auto &[err, _] = i_jump();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_cj:
    {
      const auto &[err, _] = i_conditional_jump();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_re:
    {
      const auto &[err, _] = i_return();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_cr:
    {
      const auto &[err, _] = i_conditional_return();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_sv:
    {
      const auto &[err, _] = i_set_interrupt();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_hi:
    {
      const auto &[err, _] = i_halt_interrupts();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_si:
    {
      const auto &[err, _] = i_start_interrupts();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_ti:
    {
      const auto &[err, _] = i_trigger_interrupt();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_ii:
    {
      const auto &[err, _] = i_invoke_io();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_hs:
    {
      const auto &[err, _] = i_halt_system();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_ic:
    {
      const auto &[err, _] = i_init_core();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_ac:
    {
      const auto &[err, _] = i_activate_core();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_pc:
    {
      const auto &[err, _] = i_pause_core();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_sc:
    {
      const auto &[err, _] = i_suspend_cur_core();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_rr:
    {
      const auto &[err, _] = i_read_register();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_wr:
    {
      const auto &[err, _] = i_write_register();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_cp:
    {
      const auto &[err, _] = i_copy_block();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_bc:
    {
      const auto &[err, _] = i_block_compare();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_uu:
    {
      const auto &[err, _] = i_unsigned_mode();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_ff:
    {
      const auto &[err, _] = i_float_mode();
      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }

  }

 public:
  // Constructs a vm.
  VM()
  noexcept {
    for (auto &core : cores) {
      core = Core<DS, AS, RS>{};
    }
    cores[0].active = true;
  }

  auto load_program(std::array<uint8_t, MS> prg, size_t prg_size) noexcept -> void {
    mem.load_program(prg, prg_size);
  }

  auto run() noexcept -> void {
    interpret();
  }

  /**
   * Gets a snapshot of the vm
   * @return A snapshot of the vm
   */
  auto snapshot() noexcept -> VMSnapshot<MS, DS, AS, RS, CS, IS> {
    auto core_snapshots = std::array<CoreSnapshot<DS, AS, RS>, CS>{};
    for (int i = 0; i < CS; ++i) {
      auto const snapshot = cores[i].snapshot();
      core_snapshots[i] = snapshot;
    }
    return VMSnapshot(mem.snapshot(), int_table.snapshot(), core_snapshots, cur_core_id, int_enabled);
  }
};

}

