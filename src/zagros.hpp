#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
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

  /// The operation failed because it`s attempting to divide by zero.
  DivisionByZero,

  /// The operation failed because it`s attempting to do illegal instruction on float mode.
  InvalidFloatOperation,

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
 * A snapshot of a stack.
 * @tparam S Size of the stack.
 */
template<size_t S>
class StackSnapshot {
 private:
  /// The stack`s data.
  const std::array<uint32_t, S> arr;

  /// The stack`s top index.
  const size_t top;

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
  constexpr StackSnapshot(std::array<uint32_t, S> arr, size_t top) noexcept: arr(arr), top(top) {}

  /**
   * Get the stack`s data.
   * @return The stack`s data.
   */
  constexpr std::array<uint32_t, S> get_arr() const noexcept {
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
  std::array<uint32_t, S> arr;

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
  auto push(uint32_t value) noexcept -> void {
    arr[top++] = value;
  }

  /**
   * Pops a value off the stack.
   * @return The value popped off the stack.
   */
  auto pop() noexcept -> uint32_t {
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
  std::array<uint32_t, S> arr;

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
  auto push(uint32_t value) noexcept -> result<> {
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
  auto pop() noexcept -> result<uint32_t> {
    if (top == 0) {
      return fail<uint32_t>(Error::AddressStackUnderflow);
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
  const std::array<uint32_t, S> arr;

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
  constexpr explicit RegisterBankSnapshot(std::array<uint32_t, S> arr) noexcept: arr(arr) {}

  /**
   * Get the banks`s data.
   * @return The banks`s data.
   */
  constexpr std::array<uint32_t, S> get_arr() const noexcept {
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
  std::array<uint32_t, S> arr;
 public:
  /**
   * Constructs a new register bank. All registers are initialized to 0.
   */
  constexpr RegisterBank() noexcept {
    std::fill(arr.begin(), arr.end(), 0);
  }

  /**
   * Returns the value of a register.
   * @param id The register`s id.
   * @return The value if the operation is successful, Error otherwise.
   */
  [[nodiscard]] auto read(size_t id) const noexcept -> result<uint32_t> {
    if (id >= S) {
      return fail<uint32_t>(Error::IllegalRegisterId);
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
  auto write(size_t id, uint32_t value) noexcept -> result<> {
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
    std::fill(arr.begin(), arr.end(), 0);
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
  constexpr Memory() noexcept {
    std::fill(arr.begin(), arr.end(), 0);
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
   * Sets the value of a memory location.
   * @tparam T The type of the value.
   * @param addr The addr.
   * @return The bytes if `addr` is in range, Error otherwise.
   */
  template<typename T>
  [[nodiscard]] auto read_bytes(size_t addr) const noexcept -> result<T> {
    T dst{};
    if (addr + sizeof(T) > S) {
      return {Error::IllegalMemoryAddress, dst};
    }
    std::copy_n(arr.begin() + addr, sizeof(T), reinterpret_cast<uint8_t *>(&dst));
    return success(dst);
  }

  /**
   * Compares two blocks of memory.
   * @param len The number of bytes to compare.
   * @param dst The destination addrs.
   * @param orig The origin addrs.
   * @return The result of the comparison if operation is successful, Error otherwise.
   */
  [[nodiscard]] auto compare_block(size_t len, size_t dst, size_t orig) const noexcept -> result<bool> {
    if (dst + len > S) {
      return {Error::IllegalMemoryAddress, false};
    }
    if (orig + len > S) {
      return {Error::IllegalMemoryAddress, false};
    }
    if (std::equal(arr.begin() + dst, arr.begin() + dst + len, arr.begin() + orig)) {
      return success(true);
    }
    return success(true);
  }

  /**
   * Sets the value of a memory location.
   * @tparam T The type of the value.
   * @param address The addrs.
   * @param value The value.
   * @return Unit if the addrs is in range and it can contain the value, Error otherwise.
   */
  template<typename T>
  auto write_bytes(size_t address, T value) noexcept -> result<> {
    if (address + sizeof(T) > S) {
      return fail(Error::IllegalMemoryAddress);
    }
    std::copy_n(reinterpret_cast<uint8_t *>(&value), sizeof(T), arr.begin() + address);
    return {};
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
//    std::copy_n(arr.begin() + dst, len, arr.begin() + orig);
    std::copy_n(arr.begin(), len, arr.begin() + dst);
    return {};
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
  std::array<uint32_t, S> arr{};

 public:
  /**
   * Default constructor.
   */
  constexpr InterruptTableSnapshot() noexcept: arr() {}

  /**
   * Constructs a snapshot of the interrupt table.
   * @param data The table`s arr.
   */
  constexpr explicit InterruptTableSnapshot(std::array<uint32_t, S> arr) noexcept: arr(arr) {}

  /**
   * Returns the table`s arr.
   * @return The table`s arr.
   */
  [[nodiscard]] auto get_arr() const noexcept -> std::array<uint32_t, S> {
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
  std::array<uint32_t, S> data{};

 public:
  /**
   * Constructs an empty interrupt table.
   * The table is initialized with A value out of memory`s range.
   * Memory will return a `SystemHalt` error when an instruction outside it`s boundary is fetched,
   * so the system will halt if an unset interrupt is triggered.
   */
  InterruptTable() noexcept {
    std::fill(data.begin(), data.begin() + S, 0);
  }

  /**
   * Gets the interrupt handler addrs for a given interrupt id.
   * @param id The interrupt id.
   * @return The interrupt handler addrs if the id is valid, Error otherwise.
   */
  [[nodiscard]] auto get(size_t id) const noexcept -> result<uint32_t> {
    if (id >= S) {
      return fail<uint32_t>(Error::IllegalInterruptId);
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
  auto set(size_t id, uint32_t addr) noexcept -> result<uint32_t> {
    if (id >= S) {
      return fail<uint32_t>(Error::IllegalInterruptId);
    }
    data[id] = addr;
    return success(addr);
  }

  /**
   * Clears the table.
   */
  auto clear() noexcept -> void {
    std::fill(data.begin(), data.end(), 0);
  }

  /**
   * Gets a snapshot of the table.
   */
  [[nodiscard]] constexpr auto snapshot() const noexcept -> InterruptTableSnapshot<S> {
    return InterruptTableSnapshot<S>(data);
  }
};

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
  constexpr CoreSnapshot() noexcept
      : ip(0), active(false), op_mode(OpMode::SIGNED), addr_mode(AddressMode::DIRECT), data(), addrs(), regs() {}

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

//    DataStack<DS> arr;
//    AddressStack<AS> addrs;
//    RegisterBank<RS> regs;

  /// The arr stack.
  DataStack<DS> data;

  /// The addrs stack.
  AddressStack<AS> addrs;

  /// The register bank.
  RegisterBank<RS> regs;

 public:

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
   * This pushes the T value in the following memory location to the stack.
   * It will increment the `ip` and push the arr value to the stack.
   * @tparam T The type of the value to push.
   * @param addr_offset The addrs offset from `ip` to look for the value.
   * @param i_len Length of the instruction.
   * @param mapper A mapper from `T` to uint32_t.
   * @return Unit if the operation was successful. Error otherwise.
   */
  template<typename T>
  auto i_load(uint32_t addr_offset, size_t i_len, uint32_t mapper(const T)) -> result<> {
    // Get the current core
    auto &core = cores[cur_core_id];

    // Guard the stack for 1 push.
    const auto &[guard_err, _] = core.data.guard(0, 1);
    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the addrs to look for the value.
    const auto obj_addr = core.ip + addr_offset;
    // Read the value from the memory.
    const auto &[read_err, obj] = mem.template read_bytes<T>(obj_addr);
    if (read_err != Error::None) {
      return {read_err, Unit{}};
    }

    // Convert the value to uint32_t.
    const auto data = mapper(obj);
    // Push the value to the stack.
    core.data.push(data);

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
    return i_load<uint32_t>(4, 8, [](uint32_t x) {
      return x;
    });
  }

  /**
   * Pushes the little endian first half of value to the stack.
   * The value is taken from the following two slots in the memory.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_load_half() noexcept -> result<> {
    return i_load<uint16_t>(1, 3, [](uint16_t x) {
      return static_cast<uint32_t>(x);
    });
  }

  /**
   * Pushes the little endian first byte of value to the stack.
   * The value is taken from the following slot in the memory.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_load_byte() noexcept -> result<> {
    return i_load<uint8_t>(1, 2, [](uint8_t x) {
      return static_cast<uint32_t>(x);
    });
  }

  /**
   * Fetches a T value from memory.
   * @tparam T The type of the value to fetch.
   * @param mapper The mapper from `T` to uint32_t.
   * @return Unit if the operation was successful. Error otherwise.
   */
  template<typename T>
  auto i_fetch(uint32_t mapper(const T)) -> result<> {
    // Get the current core
    auto &core = cores[cur_core_id];

    // Guard the stack for 1 stack_pop and 1 push.
    const auto &[guard_err, _] = core.data.guard(1, 1);
    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the addrs to look for the value.
    const auto obj_addr = core.data.pop();
    // Read the value from the memory.
    const auto &[read_err, obj] = mem.template read_bytes<T>(obj_addr);
    if (read_err != Error::None) {
      return {read_err, Unit{}};
    }

    // Convert the value to uint32_t.
    const auto word = mapper(obj);
    // Push the value to the stack.
    core.data.push(word);

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
    return i_fetch<uint32_t>([](uint32_t x) {
      return x;
    });
  }

  /**
   * Fetches a half-word value from memory.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_fetch_half() noexcept -> result<> {
    return i_fetch<uint16_t>([](uint16_t x) {
      return static_cast<uint32_t>(x);
    });
  }

  /**
   * Fetches a byte value from memory.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_fetch_byte() noexcept -> result<> {
    return i_fetch<uint8_t>([](uint8_t x) {
      return static_cast<uint32_t>(x);
    });
  }

  /**
   * Stores a T value to memory.
   * @tparam T The type of the value to store.
   * @param mapper The mapper from `T` to uint32_t.
   * @return Unit if the operation was successful. Error otherwise.
   */
  template<typename T>
  auto i_store(uint32_t mapper(const T)) -> result<> {
    // Get the current core
    auto &core = cores[cur_core_id];

    // Guard the stack for 2 pops.
    const auto &[guard_err, _1] = core.data.guard(2, 0);
    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the addrs to look for the value.
    const auto obj_addr = core.data.pop();
    // Get the value to store.
    const auto obj = core.data.pop();
    // Convert the value to uint32_t.
    const auto word = mapper(obj); // TODO: Find a solution
    // Write the value to the memory.
    const auto &[write_err, _2] = mem.template write_bytes<T>(obj_addr, word);
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
    return i_store<uint32_t>([](uint32_t x) {
      return x;
    });
  }

  /**
   * Stores a half-word value to memory.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_store_half() noexcept -> result<> {
    return i_store<uint16_t>([](uint16_t x) {
      return static_cast<uint32_t>(x);
    });
  }

  /**
   * Stores a byte value to memory.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_store_byte() noexcept -> result<> {
    return i_store<uint8_t>([](uint8_t x) {
      return static_cast<uint32_t>(x);
    });
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
   * Does the binary operations based on the operation mode.
   * @param signed_op The signed operation.
   * @param unsigned_op The unsigned operation.
   * @param float_op The float operation.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_binary_op(
      uint32_t signed_op(uint32_t, uint32_t),
      uint32_t unsigned_op(uint32_t, uint32_t),
      uint32_t float_op(uint32_t, uint32_t)
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

    // Compute the result based on the operation mode.
    uint32_t result;
    switch (core.op_mode) {
      case OpMode::SIGNED: {
        result = signed_op(left, right);
        break;
      }
      case OpMode::UNSIGNED: {
        result = unsigned_op(left, right);
        break;
      }
      case OpMode::FLOAT: {
        result = float_op(left, right);
        break;
      }
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
   * Does the binary operations based on the operation mode.
   * Returns error if operation mode is floating point mode.
   * @param signed_op The signed operation.
   * @param unsigned_op The unsigned operation.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_binary_op(
      uint32_t signed_op(uint32_t, uint32_t),
      uint32_t unsigned_op(uint32_t, uint32_t)
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

    // Compute the result based on the operation mode.
    uint32_t result;
    switch (core.op_mode) {
      case OpMode::SIGNED: {
        result = signed_op(left, right);
        break;
      }
      case OpMode::UNSIGNED: {
        result = unsigned_op(left, right);
        break;
      }
      case OpMode::FLOAT: {
        return fail(Error::InvalidFloatOperation);
      }
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
   * Does the binary operation in unsigned mode.
   * @param signed_op The signed operation.
   * @param unsigned_op The unsigned operation.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_binary_op(
      uint32_t unsigned_op(uint32_t, uint32_t)
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
    uint32_t result = unsigned_op(left, right);
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
    return i_binary_op([](uint32_t a, uint32_t b) {
      return static_cast<uint32_t>(a == b);
    });
  }

  /**
   * Compare two values for inequality. Returns true if they do not match or false if they do.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_not_equal() noexcept -> result<> {
    return i_binary_op([](uint32_t a, uint32_t b) {
      return static_cast<uint32_t> (a != b);
    });
  }

  /**
   * Compare two values for greater than. Returns true if the second stack_pop is less than the first pop.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_less_than() noexcept -> result<> {
    return i_binary_op(
        [](uint32_t a, uint32_t b) {
          // convert a to a signed int
          int32_t left;
          memcpy(&left, &a, sizeof(int32_t));
          // convert b to a signed int
          int32_t right;
          memcpy(&right, &b, sizeof(int32_t));
          auto result = left < right;
          return static_cast<uint32_t> (result);
        },
        [](uint32_t a, uint32_t b) {
          return static_cast<uint32_t> (a < b);
        },
        [](uint32_t a, uint32_t b) {
          // convert a to a float
          float left;
          memcpy(&left, &a, sizeof(int32_t));
          // convert b to a float
          float right;
          memcpy(&right, &b, sizeof(int32_t));
          auto result = left < right;
          return static_cast<uint32_t> (result);
        });
  }

  /**
   * Compare two values for greater than or equal. Returns true if the second pop is greater than to the first stack_pop.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_greater_than() noexcept -> result<> {
    return i_binary_op(
        [](uint32_t a, uint32_t b) {
          // convert a to a signed int
          int32_t left;
          memcpy(&left, &a, sizeof(int32_t));
          // convert b to a signed int
          int32_t right;
          memcpy(&right, &b, sizeof(int32_t));
          auto result = left > right;
          return static_cast<uint32_t> (result);
        },
        [](uint32_t a, uint32_t b) {
          return static_cast<uint32_t> (a > b);
        },
        [](uint32_t a, uint32_t b) {
          // convert a to a float
          float left;
          memcpy(&left, &a, sizeof(int32_t));
          // convert b to a float
          float right;
          memcpy(&right, &b, sizeof(int32_t));
          auto result = left > right;
          return static_cast<uint32_t> (result);
        });
  }

  /**
   * Add two values.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_add() noexcept -> result<> {
    return i_binary_op(
        [](uint32_t a, uint32_t b) {
          // convert a to a signed int
          int32_t left;
          memcpy(&left, &a, sizeof(int32_t));
          // convert b to a signed int
          int32_t right;
          memcpy(&right, &b, sizeof(int32_t));
          int32_t result = left + right;
          // convert the result to a uint32_t
          uint32_t u_result;
          memcpy(&u_result, &result, sizeof(int32_t));
          return u_result;
        },
        [](uint32_t a, uint32_t b) {
          return static_cast<uint32_t> (a + b);
        },
        [](uint32_t a, uint32_t b) {
          // convert a to a float
          float left;
          memcpy(&left, &a, sizeof(int32_t));
          // convert b to a float
          float right;
          memcpy(&right, &b, sizeof(int32_t));
          float result = left + right;
          // convert the result to a uint32_t
          uint32_t u_result;
          memcpy(&u_result, &result, sizeof(int32_t));
          return u_result;
        });
  }

  /**
   * Subtract first pop from the second stack_pop.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_subtract() noexcept -> result<> {
    return i_binary_op(
        [](uint32_t a, uint32_t b) {
          // convert a to a signed int
          int32_t left;
          memcpy(&left, &a, sizeof(int32_t));
          // convert b to a signed int
          int32_t right;
          memcpy(&right, &b, sizeof(int32_t));
          int32_t result = left - right;
          // convert the result to a uint32_t
          uint32_t u_result;
          memcpy(&u_result, &result, sizeof(int32_t));
          return u_result;
        },
        [](uint32_t a, uint32_t b) {
          return static_cast<uint32_t> (a - b);
        },
        [](uint32_t a, uint32_t b) {
          // convert a to a float
          float left;
          memcpy(&left, &a, sizeof(int32_t));
          // convert b to a float
          float right;
          memcpy(&right, &b, sizeof(int32_t));
          float result = left - right;
          // convert the result to a uint32_t
          uint32_t u_result;
          memcpy(&u_result, &result, sizeof(int32_t));
          return u_result;
        });
  }

  /**
   * Multiply two values.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_multiply() noexcept -> result<> {
    return i_binary_op(
        [](uint32_t a, uint32_t b) {
          // convert a to a signed int
          int32_t left;
          memcpy(&left, &a, sizeof(int32_t));
          // convert b to a signed int
          int32_t right;
          memcpy(&right, &b, sizeof(int32_t));
          int32_t result = left * right;
          // convert the result to a uint32_t
          uint32_t u_result;
          memcpy(&u_result, &result, sizeof(int32_t));
          return u_result;
        },
        [](uint32_t a, uint32_t b) {
          return static_cast<uint32_t> (a * b);
        },
        [](uint32_t a, uint32_t b) {
          // convert a to a float
          float left;
          memcpy(&left, &a, sizeof(int32_t));
          // convert b to a float
          float right;
          memcpy(&right, &b, sizeof(int32_t));
          float result = left * right;
          // convert the result to a uint32_t
          uint32_t u_result;
          memcpy(&u_result, &result, sizeof(int32_t));
          return u_result;
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
    uint32_t modulo;
    uint32_t quotient;
    switch (core.op_mode) {
      case OpMode::SIGNED: {
        // Convert the values to signed ints.
        int32_t s_left;
        memcpy(&s_left, &left, sizeof(int32_t));
        int32_t s_right;
        memcpy(&s_right, &right, sizeof(int32_t));
        // Check for division by zero.
        if (s_right == 0) {
          return fail(Error::DivisionByZero);
        }
        int32_t s_modulo = s_left % s_right;
        // Convert the modulo to a uint32_t.
        memcpy(&modulo, &s_modulo, sizeof(int32_t));
        int32_t s_quotient = s_left / s_right;
        // Convert the quotient to a uint32_t.
        memcpy(&quotient, &s_quotient, sizeof(int32_t));
        break;
      }
      case OpMode::UNSIGNED: {
        modulo = left % right;
        quotient = left / right;
        break;
      }
      case OpMode::FLOAT: {
        // Convert the values to floats.
        float f_left;
        memcpy(&f_left, &left, sizeof(int32_t));
        float f_right;
        memcpy(&f_right, &right, sizeof(int32_t));
        float f_modulo = fmod(f_left, f_right);
        // Convert the modulo to a uint32_t.
        memcpy(&modulo, &f_modulo, sizeof(int32_t));
        // Convert the quotient to a uint32_t.
        float f_quotient = f_left / f_right;
        memcpy(&quotient, &f_quotient, sizeof(int32_t));
        break;
      }
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

    // Guard the stack for 3 pops and 2 pushes.
    const auto &[guard_err, _1] = core.data.guard(3, 2);
    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the values from the stack
    const auto right = core.data.pop();
    const auto middle = core.data.pop();
    const auto left = core.data.pop();

    // Compute the values based on the operating mode.
    uint32_t modulo;
    uint32_t quotient;
    switch (core.op_mode) {
      case OpMode::SIGNED: {
        // Convert the values to signed ints.
        int32_t s_left;
        memcpy(&s_left, &left, sizeof(int32_t));
        int32_t s_middle;
        memcpy(&s_middle, &middle, sizeof(int32_t));
        int32_t s_right;
        memcpy(&s_right, &right, sizeof(int32_t));
        // Check for division by zero.
        if (s_right == 0) {
          return fail(Error::DivisionByZero);
        }
        int32_t s_modulo = (s_left * s_middle) % s_right;
        // Convert the modulo to a uint32_t.
        memcpy(&modulo, &s_modulo, sizeof(int32_t));
        int32_t s_quotient = (s_left * s_middle) / s_right;
        // Convert the quotient to a uint32_t.
        memcpy(&quotient, &s_quotient, sizeof(int32_t));
        break;
      }
      case OpMode::UNSIGNED: {
        modulo = left % right;
        quotient = left / right;
        break;
      }
      case OpMode::FLOAT: {
        // Convert the values to floats.
        float f_left;
        memcpy(&f_left, &left, sizeof(int32_t));
        float f_middle;
        memcpy(&f_middle, &middle, sizeof(int32_t));
        float f_right;
        memcpy(&f_right, &right, sizeof(int32_t));
        float f_modulo = fmod(f_left * f_middle, f_right);
        // Convert the modulo to a uint32_t.
        memcpy(&modulo, &f_modulo, sizeof(int32_t));
        float f_quotient = (f_left * f_middle) / f_right;
        // Convert the quotient to a uint32_t.
        memcpy(&quotient, &f_quotient, sizeof(int32_t));
        break;
      }
    }

    // Push the results onto the stack.
    core.data.push(modulo);
    core.data.push(quotient);

    // Increment the ip.
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
    return i_binary_op([](uint32_t a, uint32_t b) {
      return (a & b);
    });
  }

  /**
   * Performs a bitwise OR between two values.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_or() noexcept -> result<> {
    return i_binary_op([](uint32_t a, uint32_t b) {
      return (a | b);
    });
  }

  /**
   * Performs a bitwise XOR between two values.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_xor() noexcept -> result<> {
    return i_binary_op([](uint32_t a, uint32_t b) {
      return (a ^ b);
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
    const auto result = ~value;

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
    return i_binary_op([](uint32_t a, uint32_t b) {
                         // Convert the value to a signed int.
                         int32_t left;
                         memcpy(&left, &a, sizeof(int32_t));
                         // Convert the shift amount to a signed int.
                         int32_t right;
                         memcpy(&right, &b, sizeof(int32_t));
                         int32_t result = left << right;
                         // Convert the result to a uint32_t.
                         uint32_t u_result;
                         memcpy(&u_result, &result, sizeof(int32_t));
                         return u_result;
                       },
                       [](uint32_t a, uint32_t b) {
                         return (a << b);
                       });
  }

  /**
   * Shift second stack_pop right by first pop.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_shift_right() noexcept -> result<> {
    return i_binary_op([](uint32_t a, uint32_t b) {
                         // Convert the value to a signed int.
                         int32_t left;
                         memcpy(&left, &a, sizeof(int32_t));
                         // Convert the shift amount to a signed int.
                         int32_t right;
                         memcpy(&right, &b, sizeof(int32_t));
                         int32_t result = left >> right;
                         // Convert the result to a uint32_t.
                         uint32_t u_result;
                         memcpy(&u_result, &result, sizeof(int32_t));
                         return u_result;
                       },
                       [](uint32_t a, uint32_t b) {
                         return (a >> b);
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
    const auto result = (d << 24) | (c << 16) | (b << 8) | a;
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
    // Unpack the bytes.
    const auto a = value & 0xFF;
    const auto b = (value >> 8) & 0xFF;
    const auto c = (value >> 16) & 0xFF;
    const auto d = (value >> 24) & 0xFF;

    // Push the bytes.
    core.data.push(a);
    core.data.push(b);
    core.data.push(c);
    core.data.push(d);

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
    const auto return_addr = core.ip + 4;
    // Push the return addrs onto the stack.
    const auto &[push_err, _2] = core.addrs.push(return_addr);
    if (push_err != Error::None) {
      return {push_err, Unit{}};
    }

    // Pop the addrs of the subroutine to call.
    const auto call_addr = core.data.pop();
    // Calculate the new IP.
    uint32_t ip;
    switch (core.addr_mode) {
      case AddressMode::DIRECT: {
        ip = call_addr;
        break;
      }

      case AddressMode::RELATIVE: {
        ip = call_addr + core.ip;
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
    if (cond) {
      // Calculate the return addrs
      const auto return_addr = core.ip + 4;
      // Push the return addrs onto the addrs stack.
      const auto &[push_err, _2] = core.addrs.push(return_addr);
      if (push_err != Error::None) {
        return {push_err, Unit{}};
      }

      // Calculate the new IP.
      uint32_t ip;
      switch (core.addr_mode) {
        case AddressMode::DIRECT: {
          ip = call_addr;
          break;
        }

        case AddressMode::RELATIVE: {
          ip = call_addr + core.ip;
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
        ip = jump_addr;
        break;
      }

      case AddressMode::RELATIVE: {
        ip = jump_addr + core.ip;
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
    if (cond) {
      // Calculate the new IP.
      uint32_t ip;
      switch (core.addr_mode) {
        case AddressMode::DIRECT: {
          ip = jump_addr;
          break;
        }

        case AddressMode::RELATIVE: {
          ip = jump_addr + core.ip;
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
    core.ip = ret_addr;

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
    if (cond) {
      // Pop the return addrs.
      const auto &[pop_err, ret_addr] = core.addrs.pop();
      if (pop_err != Error::None) {
        return {pop_err, Unit{}};
      }
      // Set the IP.
      core.ip = ret_addr;
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
    const auto &[set_err, _2] = int_table.set(int_id, int_addr);
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
      interrupt(int_id);
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

    // Set the all core`s IP to a maximum value.
//    for (auto &c : cores) {
//      c.ip = UINT32_MAX;
//    }

    // Set op mode to `SIGNED`.
    core.op_mode = OpMode::SIGNED;

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
    auto &core_to_init = cores[core_id];
    core_to_init = Core<DS, AS, RS>{
        .ip =  addr,
    };

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
    auto &core_to_activate = cores[core_id];
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
    auto &core_to_pause = cores[core_id];
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
    const auto &[read_err, read] = core.regs.read(reg_id);
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
    const auto &[write_err, _2] = core.regs.write(reg_id, reg_val);
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
    const auto &[cpy_err, _2] = mem.copy_block(len, dst, orig);
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
    const auto &[cmp_err, result] = mem.compare_block(len, dst, orig);
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

//  auto get_snapshot() noexcept -> VMSnapshot {
//    return {mem, int_table, cores, cur_core_id, int_enabled};
//  }


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
      std::memcpy(&core_snapshots[i], &snapshot, sizeof(CoreSnapshot<DS, AS, RS>));
    }
    return VMSnapshot(mem.snapshot(), int_table.snapshot(), core_snapshots, cur_core_id, int_enabled);
  }
};

}

