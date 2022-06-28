//
// Created by Arian Dashti on 6/28/22.
//

#ifndef ZAGROS_INTERRUPT
#define ZAGROS_INTERRUPT

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
#include "memory.hpp"

/**
 * A table of interrupt ids to interrupt handler addresses.
 * @tparam S The size of the memory.
 */
class InterruptTable {
 private:
  /// The table`s arr
  std::array<Cell, INTERRUPT_TABLE_SIZE> data{};

 public:
  /**
   * Constructs an empty interrupt table.
   * The table is initialized with A value out of memory`s range.
   * Memory will return a `SystemHalt` error when an instruction outside it`s boundary is fetched,
   * so the system will halt if an unset interrupt is triggered.
   */
  InterruptTable() noexcept {
    std::fill(data.begin(), data.begin() + INTERRUPT_TABLE_SIZE, Cell{});
  }

  /**
   * Gets the interrupt handler addrs for a given interrupt id.
   * @param id The interrupt id.
   * @return The interrupt handler addrs if the id is valid, ZError otherwise.
   */
  auto get(size_t id) const noexcept -> std::pair<ZError, Cell> {
    if (id >= INTERRUPT_TABLE_SIZE) {
      return {ZError::IllegalInterruptId, Cell{}};
    }
    const auto addr = data[id];
    return {ZError::None, addr};
  }

  /**
   * Sets the interrupt handler addrs for a given interrupt id.
   * @param id The interrupt id.
   * @param addr The interrupt handler addrs.
   * @return Unit if the id is valid, ZError otherwise.
   */
  auto set(size_t id, Cell addr) noexcept -> std::pair<ZError, Cell> {
    if (id >= INTERRUPT_TABLE_SIZE) {
      return {ZError::IllegalInterruptId, Cell{}};
    }
    data[id] = addr;
    return {ZError::None, addr};
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
  auto snapshot() const noexcept -> InterruptTableSnapshot {
    return InterruptTableSnapshot(data);
  }
};


#endif //ZAGROS_INTERRUPT
