//
// Created by Arian Dashti on 6/28/22.
//

#ifndef ZAGROS_MEMORY
#define ZAGROS_MEMORY

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
#include "register.hpp"


/**
 * A memory.
 * @tparam S The size of the memory.
 */
class Memory {
 private:
  /// The memory`s data.
  std::array<uint8_t, MEMORY_SIZE> arr;
 public:
  /**
   * Constructs a new memory bank. All memory is initialized to 0.
   */
  Memory() noexcept: arr{} {
  }

  /**
   * Returns the value of a memory location.
   * @param addr The addr.
   * @return The opcode if `addr` is in range, `SystemHalt` otherwise.
   */
  auto fetch_opcode(size_t addr) const noexcept -> std::pair<ZError, uint8_t> {
    if (addr >= MEMORY_SIZE) {
      return {ZError::SystemHalt, uint8_t{}};
    }
    const auto opcode = arr[addr];
    return {ZError::None, opcode};
  }

  /**
   * Reads bytes from memory location
   * @tparam BS The count of bytes to read
   * @param addr The address of the memory location
   * @return A success outcome with the bytes if `addr` is legal,
   * otherwise and error outcome with `ZError::IllegalMemoryAddress`.
   */
  template<size_t BS>
  auto read_bytes(size_t addr) const noexcept -> std::pair<ZError, Cell> {
    static_assert(BS <= 4, "Cell don't have more than 4 bytes.");
    if (addr + BS > MEMORY_SIZE) {
      return {ZError::IllegalMemoryAddress, Cell{}};
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
    return {ZError::None, Cell{dst}};
  }

  /**
   * Compares two blocks of memory.
   * @param len The number of bytes to compare.
   * @param dst The destination addrs.
   * @param orig The origin addrs.
   * @return The outcome of the comparison if operation is successful, ZError otherwise.
   */
  auto compare_block(size_t len, size_t dst, size_t orig) const noexcept -> std::pair<ZError, Cell> {
    if (dst + len > MEMORY_SIZE) {
      return {ZError::IllegalMemoryAddress, Cell{}};
    }
    if (orig + len > MEMORY_SIZE) {
      return {ZError::IllegalMemoryAddress, Cell{}};
    }
    if (std::equal(arr.begin() + dst, arr.begin() + dst + len, arr.begin() + orig)) {
      return {ZError::None, Cell{true}};
    }
    return {ZError::None, Cell{true}};
  }

  /**
   * Sets the value of a memory location.
   * @tparam BS The number of bytes to be copied from the value.
   * @param addr The addrs.
   * @param value The value.
   * @return A success outcome if `addr` is legal,
   * otherwise and error outcome with `ZError::IllegalMemoryAddress`.
   * */
  template<size_t BS>
  auto write_bytes(size_t addr, Cell value) noexcept -> std::pair<ZError, Unit> {
    static_assert(BS <= 4, "Cell don't have more than 4 bytes.");
    if (addr + BS > MEMORY_SIZE) {
      return {ZError::IllegalMemoryAddress, Unit{}};
    }
    auto src = value.to_bytes();
    for (int i = 0; i < BS; ++i) {
      arr[addr + i] = src[i];
    }
    return {ZError::None, Unit{}};
  }

  /**
   * Copies a block of memory to another.
   * @param len The number of bytes to copy.
   * @param dst The destination addrs.
   * @param orig The origin addrs.
   * @return
   */
  auto copy_block(size_t len, size_t dst, size_t orig) noexcept -> std::pair<ZError, Unit> {
    if (dst + len > MEMORY_SIZE || orig + len > MEMORY_SIZE) {
      return {ZError::IllegalMemoryAddress, Unit{}};
    }
    std::copy_n(arr.begin() + orig, len, arr.begin() + dst);
    return {ZError::None, Unit{}};
  }

  /**
   * Loads the memory from a memory array.
   * @param prg The memory array.
   */
  auto load_program(std::array<uint8_t, MEMORY_SIZE> prg, size_t prg_size) noexcept -> std::pair<ZError, Unit> {
    if (prg_size > MEMORY_SIZE) {
      return {ZError::IllegalMemoryAddress, Unit{}};
    }
    // Copy the program into the memory.
    std::copy_n(prg.begin(), prg_size, arr.begin());
    return {ZError::None, Unit{}};
  }

  /**
   * Writes the byte into given memory address, must be valid memory address for I/Os.
   * @param addr The address of memory byte.
   * @param byte The byte of memory.
   * @return ZError if address is illegal, Success otherwise.
   */
  auto write_io_byte(size_t addr, uint8_t byte) noexcept -> std::pair<ZError, Unit> {
    if (addr < IO_MEMORY_ADDRESS_BEGIN || addr >= IO_MEMORY_ADDRESS_END) {
      return {ZError::IllegalMemoryAddress, Unit{}};
    }
    arr[addr] = byte;
    return {ZError::None, Unit{}};
  }

  /**
   * Reads a byte from given memory address, must be valid memory address for I/Os.
   * @param addr The address of memory byte
   * @return ZError if address is illegal, Success otherwise.
   */
  auto read_io_byte(size_t addr) noexcept -> std::pair<ZError, uint8_t> {
    if (addr < IO_MEMORY_ADDRESS_BEGIN || addr >= IO_MEMORY_ADDRESS_END) {
      return {ZError::IllegalMemoryAddress, uint8_t {}};
    }
    return {ZError::None, arr[addr]};
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
  auto snapshot() const noexcept -> MemorySnapshot {
    return MemorySnapshot(arr);
  }
};


#endif //ZAGROS_MEMORY
