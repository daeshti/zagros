//
// Created by Arian Dashti on 6/28/22.
//

#ifndef ZAGROS_CORE
#define ZAGROS_CORE

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
#include "interrupt.hpp"

namespace zagros {
/**
 * The state of a core of the VM.
 * @tparam DS Size of the arr stack.
 */
class Core {
 public:
  /// The instruction pointer.
  uint32_t ip = 0;

  /// Whether the core is active or not.
  bool active = false;

  /// The current operation mode.
  zagros::OpMode op_mode = zagros::OpMode::SIGNED;

  /// The current addrs mode.
  zagros::AddressMode addr_mode = zagros::DIRECT;

  /// The arr stack.
  zagros::DataStack data;

  /// The addrs stack.
  zagros::AddressStack addrs;

  /// The register bank.
  zagros::RegisterBank regs;

  /**
   * Set the core's state as just initialized with current ip.
   * @param init_ip The instruction pointer.
   */
  auto init(uint32_t init_ip) noexcept -> void {
    ip = init_ip;
    active = false;
    op_mode = zagros::OpMode::SIGNED;
    addr_mode = zagros::DIRECT;
    data.clear();
    addrs.clear();
    regs.clear();
  }

  /**
   * Gets a snapshot of the core.
   * @return A snapshot of the core.
   */
  auto snapshot() const noexcept -> zagros::CoreSnapshot {
    return {ip, active, op_mode, addr_mode, data.snapshot(), addrs.snapshot(), regs.snapshot()};
  }
};
}

#endif //ZAGROS_CORE
