//
// Created by Arian Dashti on 6/27/22.
//

#ifndef ZAGROS_INSTRUCTION_MODE
#define ZAGROS_INSTRUCTION_MODE

namespace zagros {
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
}
#endif //ZAGROS_INSTRUCTION_MODE