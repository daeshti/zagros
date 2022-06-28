//
// Created by Arian Dashti on 6/28/22.
//

#ifndef ZAGROS_IO
#define ZAGROS_IO

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>
#include <utility>
#include "result.hpp"
#include "cell.hpp"
#include "callback.hpp"
#include "zagros_configuration.h"
#include "instruction_mode.hpp"
#include "snapshot.hpp"
#include "stack.hpp"
#include "register.hpp"
#include "memory.hpp"

/**
 * A table of io ids to callbacks.
 * @tparam S The size of the memory.
 */
class IoTable {
 private:
  /// The table`s arr
  std::array<Callback*, IO_TABLE_SIZE> callbacks{};

 public:
  /**
   * Constructs an empty interrupt table.
   * The table is initialized with A value out of memory`s range.
   * Memory will return a `SystemHalt` error when an instruction outside it`s boundary is fetched,
   * so the system will halt if an unset interrupt is triggered.
   */
  IoTable() noexcept {
    std::fill(callbacks.begin(), callbacks.begin() + IO_TABLE_SIZE, new Callback{});
  }

  /**
   * Constructs an full interrupt table.
   * The table is initialized with A value out of memory`s range.
   * Memory will return a `SystemHalt` error when an instruction outside it`s boundary is fetched,
   * so the system will halt if an unset interrupt is triggered.
   */
  explicit IoTable(std::array<Callback*, IO_TABLE_SIZE> callbacks) : callbacks{callbacks} {

  }

  /**
   * Runs the callback for a given I/O id. Doesn't call anything if I/O id is invalid or if the callback is a `nullptr`
   * @param id The I/O id.
   */
  void call(size_t id) const noexcept {
    if (id >= INTERRUPT_TABLE_SIZE) {
      return;
    }

    const auto callback = callbacks[id];
    if (callback == nullptr) {
      return;
    }

    callback->run();
  }

  IoTableSnapshot snapshot() {
    std::vector<std::string> descriptions;
    for (const auto &callback : callbacks) {
      if (callback != nullptr) {
        descriptions.push_back(callback->description());
      } else {
        descriptions.emplace_back("nullptr");
      }
    }
    return IoTableSnapshot{descriptions};
  }
};



#endif //ZAGROS_IO
