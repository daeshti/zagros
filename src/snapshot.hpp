//
// Created by Arian Dashti on 6/27/22.
//

#ifndef ZAGROS_SNAPSHOT
#define ZAGROS_SNAPSHOT

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>
#include <utility>
#include <ostream>
#include "result.hpp"
#include "cell.hpp"
#include "zagros_configuration.h"
#include "instruction_mode.hpp"

/**
 * A snapshot of a data stack.
 */
class DataStackSnapshot {
 protected:
  /// The stack`s data.
  std::array<Cell, DATA_STACK_SIZE> arr;

  /// The stack`s top index.
  size_t top;

 public:

  /**
   * Default constructor
   */
  DataStackSnapshot() : arr{}, top{} {
  }

  /**
   * Constructor.
   * @param arr The stack`s arr.
   * @param top The stack`s top index.
   */
  DataStackSnapshot(std::array<Cell, DATA_STACK_SIZE> arr, size_t top) noexcept: arr(arr), top(top) {}

  /**
   * Copy Constructor
   */
  DataStackSnapshot(const DataStackSnapshot &rhs) noexcept: arr{rhs.arr}, top{rhs.top} {};

  /**
   * Get the stack`s data.
   * @return The stack`s data.
   */
  std::array<Cell, DATA_STACK_SIZE> get_arr() const noexcept {
    return arr;
  }

  /**
   * Get the stack`s top index.
   * @return The stack`s top index.
   */
  size_t get_top() const noexcept {
    return top;
  }

  virtual std::string toString() const {
    std::stringstream os;
    os << "{ arr: [";
    size_t i;
    for (i = 0; i < arr.size() - 1; ++i) {
      os << arr[i].toString() << ", ";
    }
    os << arr[i].toString() << "]";
    os << " top: " << top << " }";
    return os.str();
  }
};

/**
 * A snapshot of an address stack.
 */
class AddressStackSnapshot {
 protected:
  /// The stack`s address.
  std::array<Cell, ADDRESS_STACK_SIZE> arr;

  /// The stack`s top index.
  size_t top;

 public:

  /**
   * Default constructor
   */
  AddressStackSnapshot() : arr{}, top{} {
  }

  /**
   * Constructor.
   * @param arr The stack`s arr.
   * @param top The stack`s top index.
   */
  AddressStackSnapshot(std::array<Cell, ADDRESS_STACK_SIZE> arr, size_t top) noexcept: arr(arr), top(top) {}

  /**
   * Copy Constructor
   */
  AddressStackSnapshot(const AddressStackSnapshot &rhs) noexcept: arr{rhs.arr}, top{rhs.top} {};

  /**
   * Get the stack`s address.
   * @return The stack`s address.
   */
  std::array<Cell, ADDRESS_STACK_SIZE> get_arr() const noexcept {
    return arr;
  }

  /**
   * Get the stack`s top index.
   * @return The stack`s top index.
   */
  size_t get_top() const noexcept {
    return top;
  }

  virtual std::string toString() const {
    std::stringstream os;
    os << "{ arr: [";
    size_t i;
    for (i = 0; i < arr.size() - 1; ++i) {
      os << arr[i].toString() << ", ";
    }
    os << arr[i].toString() << "]";
    os << " top: " << top << " }";
    return os.str();
  }
};

/**
 * A snapshot of bank of registers.
 * @tparam S Size of the bank.
 */
class RegisterBankSnapshot {
 private:
  /// The bank`s data.
  std::array<Cell, REGISTER_BANK_SIZE> arr;

 public:

  /**
   * Default constructor
   */
  RegisterBankSnapshot() : arr{} {

  }
  /**
   * Constructor
   * @param arr The bank`s data.
   * @param top The bank`s top index.
   */
  explicit RegisterBankSnapshot(std::array<Cell, REGISTER_BANK_SIZE> arr) noexcept: arr(arr) {}

  /**
 * Copy Constructor
 */
  RegisterBankSnapshot(const RegisterBankSnapshot &rhs) noexcept: arr{rhs.arr} {};

  /**
   * Get the banks`s data.
   * @return The banks`s data.
   */
  std::array<Cell, REGISTER_BANK_SIZE> get_arr() const noexcept {
    return arr;
  }

  virtual std::string toString() const {
    std::stringstream os;
    os << "{ arr: [";
    size_t i;
    for (i = 0; i < arr.size() - 1; ++i) {
      os << arr[i].toString() << ", ";
    }
    os << arr[i].toString() << "] }";
    return os.str();
  }
};

/**
 * A snapshot of the memory.
 * @tparam S The size of the memory.
 */
class MemorySnapshot {
 private:
  /// The memory`s data.
  const std::array<uint8_t, MEMORY_SIZE> arr;

 public:

  /**
   * Constructor
   * @param arr The memory`s data.
   * @param top The memory`s top index.
   */
  explicit MemorySnapshot(std::array<uint8_t, MEMORY_SIZE> arr) noexcept: arr(arr) {}

  /**
   * Get the memory`s data.
   * @return The memory`s data.
   */
  std::array<uint8_t, MEMORY_SIZE> get_arr() const noexcept {
    return arr;
  }

  virtual std::string toString() const {
    std::stringstream os;
    os << "{ arr: [";
    size_t i;
    for (i = 0; i < arr.size() - 1; ++i) {
      os << arr[i] << ", ";
    }
    os << arr[i] << "] }";
    return os.str();
  }
};
/**
 * A snapshot of the interrupt table.
 * @tparam S The size of the table.
 */
class InterruptTableSnapshot {
 private:
  /// The table`s data
  const std::array<Cell, INTERRUPT_TABLE_SIZE> arr{};

 public:

  /**
   * Constructs a snapshot of the interrupt table.
   * @param data The table`s arr.
   */
  explicit InterruptTableSnapshot(std::array<Cell, INTERRUPT_TABLE_SIZE> arr) noexcept: arr(arr) {}

  /**
   * Returns the table`s arr.
   * @return The table`s arr.
   */
  auto get_arr() const noexcept -> std::array<Cell, INTERRUPT_TABLE_SIZE> {
    return arr;
  }

  virtual std::string toString() const {
    std::stringstream os;
    os << "{ arr: [";
    size_t i;
    for (i = 0; i < arr.size() - 1; ++i) {
      os << arr[i].toString() << ", ";
    }
    os << arr[i].toString() << "] }";
    return os.str();
  }
};
/**
 * Snapshot of the core
 * @tparam DS Size of the arr stack.
 */
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
  DataStackSnapshot data;

  /// The addrs stack.
  AddressStackSnapshot addrs;

  /// The register bank.
  RegisterBankSnapshot regs;

 public:

  /**
   * Default constructor
   */
  CoreSnapshot() : ip{}, active{}, op_mode{}, addr_mode{}, data{}, addrs{}, regs{} {

  }

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
  CoreSnapshot(uint32_t ip, bool active, OpMode op_mode, AddressMode addr_mode,
               DataStackSnapshot data, AddressStackSnapshot addrs, RegisterBankSnapshot regs) noexcept
      : ip{ip}, active{active}, op_mode{op_mode}, addr_mode{addr_mode},
        data{data}, addrs{addrs}, regs{regs} {
  }

  /**
   * Copy constructor
   * @param rhs Right hand side
   */
  CoreSnapshot(const CoreSnapshot &rhs) : ip{rhs.ip},
                                          active{rhs.active},
                                          op_mode{rhs.op_mode},
                                          addr_mode{rhs.addr_mode},
                                          data{rhs.data},
                                          addrs{rhs.addrs},
                                          regs{rhs.regs} {

  }

  /**
   * Gets the instruction pointer.
   * @return The instruction pointer.
   */
  auto get_ip() const noexcept -> uint32_t {
    return ip;
  }

  /**
   * Gets whether the core is active or not.
   * @return Whether the core is active or not.
   */
  auto is_active() const noexcept -> bool {
    return active;
  }

  /**
   * Gets the current operation mode.
   * @return The current operation mode.
   */
  auto get_op_mode() const noexcept -> OpMode {
    return op_mode;
  }

  /**
   * Gets the current addrs mode.
   * @return The current addrs mode.
   */
  auto get_addr_mode() const noexcept -> AddressMode {
    return addr_mode;
  }

  /**
   * Gets the arr stack.
   * @return The arr stack.
   */
  auto get_data() const noexcept -> DataStackSnapshot {
    return data;
  }

  /**
   * Gets the addrs stack.
   * @return The addrs stack.
   */
  auto get_addrs() const noexcept -> AddressStackSnapshot {
    return addrs;
  }

  /**
   * Gets the register bank.
   * @return The register bank.
   */
  auto get_regs() const noexcept -> RegisterBankSnapshot {
    return regs;
  }

  virtual std::string toString() const {
    std::stringstream os;
    os << "{ ip: " << ip << " active: " << active << " op_mode: " << (int) op_mode << " addr_mode: "
       << (int) addr_mode << " data: " << data.toString() << " addrs: " << addrs.toString() << " regs: "
       << regs.toString() << " }";
    return os.str();
  }

};

class IoTableSnapshot {
  /// The table`s callback descriptions
  const std::vector<std::string> arr;

 public:
/**
 * Constructs a snapshot of the io table.
 * @param data The table`s arr.
 */
  explicit IoTableSnapshot(std::vector<std::string> arr) noexcept: arr(std::move(arr)) {}

  /**
   * Returns the table`s arr.
   * @return The table`s arr.
   */
  const std::vector<std::string> &get_arr() const noexcept {
    return arr;
  }

  virtual std::string toString() const {
    std::stringstream os;
    os << "arr: [";
    size_t i;
    for (i = 0; i < arr.size() - 1; ++i) {
      os << arr[i] << ", ";
    }
    os << arr[i] << "]";
    return os.str();
  }
};

/**
 * Snapshot of the vm.
 * @tparam DS Size of the arr stack.
 */

class VMSnapshot {
 private:
  /// The array view of memory.
  const MemorySnapshot mem;

  /// The array view of interrupt table.
  const InterruptTableSnapshot int_table;

  /// The cores.
  const std::array<CoreSnapshot, CORE_COUNT> cores;

  /// The I/O table descriptions
  const IoTableSnapshot io_table;

  /// The current core.
  const size_t cur_core_id;

  /// Whether or not interrupts are enabled
  const bool int_enabled;

 public:
  /// Constructs a readonly snapshot of the VM.
  VMSnapshot(const MemorySnapshot mem, const InterruptTableSnapshot int_table,
             const IoTableSnapshot io_table,
             const std::array<CoreSnapshot, CORE_COUNT> cores, size_t cur_core_id,
             bool int_enabled) noexcept:
      mem(mem),
      int_table(int_table),
      io_table(io_table),
      cores(cores),
      cur_core_id(cur_core_id),
      int_enabled(int_enabled) {

  }

  /**
   * Gets the memory.
   */
  MemorySnapshot get_mem() const noexcept {
    return mem;
  }

  /**
   * Gets the interrupt table.
   */
  InterruptTableSnapshot get_int_table() const noexcept {
    return int_table;
  }

  /**
   * Gets the cores.
   */
  std::array<CoreSnapshot, CORE_COUNT> get_cores() const noexcept {
    return cores;
  }

  /**
   * Gets the I/O table.
   */

  IoTableSnapshot get_io_table() const noexcept {
    return io_table;
  }

  /**
   * Gets the current core.
   */
  size_t get_cur_core_id() const noexcept {
    return cur_core_id;
  }

  /**
   * Gets whether or not interrupts are enabled.
   */
  bool get_int_enabled() const noexcept {
    return int_enabled;
  }

  virtual std::string toString() const {
    std::stringstream os;
    os << "{ mem: " << mem.toString() << " int_table: " << int_table.toString();

    os << " cores: [";
    size_t i;
    for (i = 0; i < cores.size() - 1; ++i) {
      os << cores[i].toString() << ", ";
    }
    os << cores[i].toString() << " ]";

    os << " io_table: " << io_table.toString() << " cur_core_id: " << cur_core_id << " int_enabled: "
       << int_enabled << "}";

    return os.str();
  }

};

#endif //ZAGROS_SNAPSHOT
