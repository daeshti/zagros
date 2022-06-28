
#ifndef ZAGROS
#define ZAGROS

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
#include "io.h"
#include "core.hpp"


/**
 * The Zagros VM.
 */
class VM {
 private:
  /// The memory.
  Memory mem;

  /// The interrupt table.
  InterruptTable int_table;

  /// The cores.
  std::array<Core, CORE_COUNT>
      cores;

  IoTable io_table;

  /// The current core.
  size_t cur_core_id = 0;

  /// Whether or not interrupts are enabled
  bool int_enabled = false;

  /**
   * Selects the next active core and sets the `cur_core_id` instance variable.
   */
  auto sel_next_core() noexcept -> void {
    if (CORE_COUNT == 1) {
      return;
    }

    // Look from current core to the end of the array
    for (size_t next = cur_core_id + 1; next < CORE_COUNT; next++) {
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

    return {Error::None, Unit{}};
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
    const auto guard_result = core.data.guard(0, 1);
    const auto guard_err = std::get<0>(guard_result);

    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the addrs to look for the value.
    const auto cell_addr = core.ip + addr_offset;
    // Read the value from the memory.
    const auto read_result = mem.template read_bytes<S>(cell_addr);
    const auto read_err = std::get<0>(read_result);
    const auto cell = std::get<1>(read_result);
    if (read_err != Error::None) {
      return {read_err, Unit{}};
    }

    // Push the value to the stack.
    core.data.push(cell);

    // Increment the ip.
    core.ip += i_len;
    // Set the operation mode to 'SIGNED'
    core.op_mode = OpMode::SIGNED;

    return {Error::None, Unit{}};
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
    const auto guard_result = core.data.guard(1, 1);
    const auto guard_err = std::get<0>(guard_result);

    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the addrs to look for the value.
    const auto cell_addr = core.data.pop();
    // Read the value from the memory.
    const auto read_result = mem.template read_bytes<S>(cell_addr.to_size());
    const auto read_err = std::get<0>(read_result);
    const auto cell = std::get<1>(read_result);
    if (read_err != Error::None) {
      return {read_err, Unit{}};
    }

    // Push the value to the stack.
    core.data.push(cell);

    // Increment the ip.
    core.ip += 1;
    // Set the operation mode to signed.
    core.op_mode = OpMode::SIGNED;

    return {Error::None, Unit{}};
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
    const auto guard_result = core.data.guard(2, 0);
    const auto guard_err = std::get<0>(guard_result);

    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the addrs to look for the value.
    const auto cell_addr = core.data.pop();
    // Get the value to store.
    const auto cell = core.data.pop();

    // Write the value to the memory.
    const auto write_result = mem.template write_bytes<S>(cell_addr.to_size(), cell);
    const auto write_err = std::get<0>(write_result);

    if (write_err != Error::None) {
      return {write_err, Unit{}};
    }

    // Increment the ip.
    core.ip += 1;
    // Set the operation mode to 'SIGNED'
    core.op_mode = OpMode::SIGNED;

    return {Error::None, Unit{}};
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
    const auto guard_result = core.data.guard(1, 2);
    const auto guard_err = std::get<0>(guard_result);

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

    return {Error::None, Unit{}};
  }

  /**
   * Discards the top value on the stack.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_drop() noexcept -> result<> {
    // Get the current core
    auto &core = cores[cur_core_id];

    // Guard the stack for 1 stack_pop.
    const auto guard_result = core.data.guard(1, 0);
    const auto guard_err = std::get<0>(guard_result);

    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Drop the value.
    core.data.pop();

    // Increment the ip.
    core.ip += 1;
    // Set the operation mode to 'SIGNED'
    core.op_mode = OpMode::SIGNED;

    return {Error::None, Unit{}};
  }

  /**
   * Swaps the top two values on the stack.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_swap() noexcept -> result<> {
    // Get the current core
    auto &core = cores[cur_core_id];

    // Guard the stack for 2 pops and 2 pushes.
    const auto guard_result = core.data.guard(2, 2);
    const auto guard_err = std::get<0>(guard_result);

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

    return {Error::None, Unit{}};
  }

  /**
   * Pushes the top value on the arr stack to the addrs stack.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_push_address() noexcept -> result<> {
    // Get the current core
    auto &core = cores[cur_core_id];

    // Guard the stack for 1 stack_pop.
    const auto guard_result = core.data.guard(1, 0);
    const auto guard_err = std::get<0>(guard_result);

    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the value to push.
    const auto addr = core.data.pop();
    // Push the value to the addrs stack.
    const auto push_result = core.addrs.push(addr);
    const auto push_err = std::get<0>(push_result);

    if (push_err != Error::None) {
      return {push_err, Unit{}};
    }

    // Increment the ip.
    core.ip += 1;
    // Set the operation mode to 'SIGNED'
    core.op_mode = OpMode::SIGNED;

    return {Error::None, Unit{}};
  }

  /**
   * Pops the top value from the addrs stack to the arr stack.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_pop_address() noexcept -> result<> {
    // Get the current core
    auto &core = cores[cur_core_id];

    // Guard the stack for 1 push.
    const auto guard_result = core.data.guard(0, 1);
    const auto guard_err = std::get<0>(guard_result);

    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the addrs value to push.
    const auto pop_result = core.addrs.pop();
    const auto pop_err = std::get<0>(pop_result);
    const auto addr = std::get<1>(pop_result);
    if (pop_err != Error::None) {
      return {pop_err, Unit{}};
    }
    // Push the addrs value.
    core.data.push(addr);

    // Increment the ip.
    core.ip += 1;
    // Set the operation mode to 'SIGNED'
    core.op_mode = OpMode::SIGNED;

    return {Error::None, Unit{}};
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
    const auto guard_result = core.data.guard(2, 1);
    const auto guard_err = std::get<0>(guard_result);

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

    return {Error::None, Unit{}};
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
    const auto guard_result = core.data.guard(2, 1);
    const auto guard_err = std::get<0>(guard_result);

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

    return {Error::None, Unit{}};
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
    const auto guard_result = core.data.guard(2, 1);
    const auto guard_err = std::get<0>(guard_result);

    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the values to operate on.
    const auto right = core.data.pop();
    const auto left = core.data.pop();
    // Compute the result.
    const auto error_result = op(left, right, core.op_mode);
    const auto error = std::get<0>(error_result);
    const auto result = std::get<1>(error_result);
    if (error != Error::None) {
      return {guard_err, Unit{}};
    }
    // Push the result.
    core.data.push(result);

    // Increment the ip.
    core.ip += 1;
    // Set the operation mode to signed.
    core.op_mode = OpMode::SIGNED;

    return {Error::None, Unit{}};
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
    const auto guard_result = core.data.guard(2, 2);
    const auto guard_err = std::get<0>(guard_result);

    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the values from the stack
    const auto right = core.data.pop();
    const auto left = core.data.pop();

    // Compute the values based on the operating mode.
    auto const op_result = left.divide_remainder(right, core.op_mode);
    auto const err = std::get<0>(op_result);
    auto const modulo = std::get<1>(op_result);
    auto const quotient = std::get<2>(op_result);
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

    return {Error::None, Unit{}};
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
    const auto guard_result = core.data.guard(3, 2);
    const auto guard_err = std::get<0>(guard_result);

    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the values from the stack
    const auto right = core.data.pop();
    const auto mul = core.data.pop();
    const auto left = core.data.pop();

    // Compute the values based on the operating mode.
    auto const op_result = left.multiply_divide_remainder(mul, right, core.op_mode);
    auto const err = std::get<0>(op_result);
    auto const modulo = std::get<1>(op_result);
    auto const quotient = std::get<2>(op_result);
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

    return {Error::None, Unit{}};
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
    const auto guard_result = core.data.guard(1, 1);
    const auto guard_err = std::get<0>(guard_result);

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

    return {Error::None, Unit{}};
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
    const auto guard_result = core.data.guard(4, 1);
    const auto guard_err = std::get<0>(guard_result);

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

    return {Error::None, Unit{}};
  }

  /**
   * Unpack four bytes from a single word.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_unpack_bytes() noexcept -> result<> {
    // Get the current core.
    auto &core = cores[cur_core_id];

    // Guard the stack for 1 stack_pop and 4 pushes.
    const auto guard_result = core.data.guard(1, 4);
    const auto guard_err = std::get<0>(guard_result);

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

    return {Error::None, Unit{}};
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

    return {Error::None, Unit{}};
  }

  /**
   * Calls a subroutine.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_call() noexcept -> result<> {
    // Get the current core.
    auto &core = cores[cur_core_id];

    // Guard the stack for 1 stack_pop.
    const auto guard_result = core.data.guard(1, 0);
    const auto guard_err = std::get<0>(guard_result);

    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Calculate the return addrs
    const uint32_t return_addr = core.ip + 4;
    // Push the return addrs onto the stack.
    const auto push_result = core.addrs.push(Cell{return_addr});
    const auto push_err = std::get<0>(push_result);

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

    return {Error::None, Unit{}};
  }

  /**
   * Calls a subroutine at addrs first stack_pop if the condition second pop is true.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_conditional_call() noexcept -> result<> {
    // Get the current core.
    auto &core = cores[cur_core_id];

    // Guard the stack for 2 pops.
    const auto guard_result = core.data.guard(2, 0);
    const auto guard_err = std::get<0>(guard_result);

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
      const auto push_result = core.addrs.push(Cell{return_addr});
      const auto push_err = std::get<0>(push_result);

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

    return {Error::None, Unit{}};
  }

  /**
   * Jumps to the addrs first stack_pop.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_jump() noexcept -> result<> {
    // Get the current core.
    auto &core = cores[cur_core_id];

    // Guard the stack for 1 stack_pop.
    const auto guard_result = core.data.guard(1, 0);
    const auto guard_err = std::get<0>(guard_result);

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

    return {Error::None, Unit{}};
  }

  /**
   * Jumps to the addrs first pop if the condition second stack_pop is true.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_conditional_jump() noexcept -> result<> {
    // Get the current core.
    auto &core = cores[cur_core_id];

    // Guard the stack for 2 pops.
    const auto guard_result = core.data.guard(2, 0);
    const auto guard_err = std::get<0>(guard_result);

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

    return {Error::None, Unit{}};
  }

  /**
   * Returns from a subroutine. Pops the `ip` from the addrs stack.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_return() noexcept -> result<> {
    // Get the current core.
    auto &core = cores[cur_core_id];

    // Pop the return addrs.
    const auto pop_result = core.addrs.pop();
    const auto pop_err = std::get<0>(pop_result);
    const auto ret_addr = std::get<1>(pop_result);
    if (pop_err != Error::None) {
      return {pop_err, Unit{}};
    }
    // Set the IP.
    core.ip = ret_addr.to_uint32();

    // Set the addrs mode to `DIRECT`.
    core.addr_mode = AddressMode::DIRECT;
    // Set the operation mode to `SIGNED`.
    core.op_mode = OpMode::SIGNED;

    return {Error::None, Unit{}};
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
    const auto guard_result = core.data.guard(1, 0);
    const auto guard_err = std::get<0>(guard_result);

    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the condition.
    const auto cond = core.data.pop();
    // If the condition is true, push the current IP onto the addrs stack.
    if (cond.to_bool()) {
      // Pop the return addrs.
      const auto pop_result = core.addrs.pop();
      const auto pop_err = std::get<0>(pop_result);
      const auto ret_addr = std::get<1>(pop_result);
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

    return {Error::None, Unit{}};
  }

  /**
   * Sets the interrupt handler for interrupt id first pop to the function at addrs second stack_pop.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_set_interrupt() noexcept -> result<> {
    // Get the current core.
    auto &core = cores[cur_core_id];

    // Guard the stack for 2 pops.
    const auto guard_result = core.data.guard(2, 0);
    const auto guard_err = std::get<0>(guard_result);

    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the interrupt id.
    auto int_id = core.data.pop();
    // Get interrupt addrs.
    auto int_addr = core.data.pop();
    // Set the interrupt handler.
    const auto set_result = int_table.set(int_id.to_uint32(), int_addr);
    const auto set_err = std::get<0>(set_result);

    if (set_err != Error::None) {
      return {set_err, Unit{}};
    }

    // Increment the ip.
    core.ip += 1;
    // Set the operation mode to `SIGNED`.
    core.op_mode = OpMode::SIGNED;

    return {Error::None, Unit{}};
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

    return {Error::None, Unit{}};
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

    return {Error::None, Unit{}};
  }

  /**
   * Forces an interrupt.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_trigger_interrupt() noexcept -> result<> {
    // Get the current core.
    auto &core = cores[cur_core_id];

    // Guard the stack for 1 stack_pop.
    const auto guard_result = core.data.guard(1, 0);
    const auto guard_err = std::get<0>(guard_result);

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

    return {Error::None, Unit{}};
  }

  /**
   * Triggers an I/O operation.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_invoke_io() noexcept -> result<> {

    // Get the current core.
    auto &core = cores[cur_core_id];

    // Guard the stack for 1 stack_pop.
    const auto guard_result = core.data.guard(1, 0);
    const auto guard_err = std::get<0>(guard_result);

    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the current I/O id
    auto io_id = core.data.pop().to_size();
    // Call the I/O
    io_table.call(io_id);

    // Increment the ip.
    core.ip += 1;

    // Set the operation mode to `SIGNED`.
    core.op_mode = OpMode::SIGNED;

    return {Error::None, Unit{}};
  }

  /**
   * Halt execution of the system by returning an error.
   * @return A `HaltSystem` error.
   */
  auto i_halt_system() noexcept -> result<> {
    // Get the current core.
    auto &core = cores[cur_core_id];

    // Return the error.
    return {Error::SystemHalt, Unit{}};
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
    const auto guard_result = core.data.guard(2, 0);
    const auto guard_err = std::get<0>(guard_result);

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

    return {Error::None, Unit{}};
  }

  /**
   * Activates a core. The core should have been initialized first.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_activate_core() noexcept -> result<> {
    // Get the current core.
    auto &core = cores[cur_core_id];

    // Guard the stack for 1 pops.
    const auto guard_result = core.data.guard(1, 0);
    const auto guard_err = std::get<0>(guard_result);

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

    return {Error::None, Unit{}};
  }

  /**
   * Pauses a core. Pass the core number stack_pop.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_pause_core() noexcept -> result<> {
    // Get the current core.
    auto &core = cores[cur_core_id];

    // Guard the stack for 1 pops.
    const auto guard_result = core.data.guard(1, 0);
    const auto guard_err = std::get<0>(guard_result);

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

    return {Error::None, Unit{}};
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

    return {Error::None, Unit{}};
  }

  /**
   * Reads a register / the private memory in the current core.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_read_register() noexcept -> result<> {
    // Get the current core.
    auto &core = cores[cur_core_id];

    // Guard the stack for 1 stack_pop and 1 push.
    const auto guard_result = core.data.guard(1, 1);
    const auto guard_err = std::get<0>(guard_result);

    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the register id.
    auto reg_id = core.data.pop();
    // Get the register.
    const auto read_result = core.regs.read(reg_id.to_uint32());
    const auto read_err = std::get<0>(read_result);
    const auto read = std::get<1>(read_result);
    if (read_err != Error::None) {
      return {read_err, Unit{}};
    }

    // Push the register value.
    core.data.push(read);

    // Increment the ip.
    core.ip += 1;
    // Set op mode to `SIGNED`.
    core.op_mode = OpMode::SIGNED;

    return {Error::None, Unit{}};
  }

  /**
   * Writes a value to a register / the private memory in the current core.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_write_register() noexcept -> result<> {
    // Get the current core.
    auto &core = cores[cur_core_id];

    // Guard the stack for 2 pops.
    const auto guard_result = core.data.guard(2, 0);
    const auto guard_err = std::get<0>(guard_result);

    if (guard_err != Error::None) {
      return {guard_err, Unit{}};
    }

    // Get the register id.
    auto reg_id = core.data.pop();
    // Get the register value.
    auto reg_val = core.data.pop();
    // Write to the register.
    const auto write_result = core.regs.write(reg_id.to_uint32(), reg_val);
    const auto write_err = std::get<0>(write_result);

    if (write_err != Error::None) {
      return {write_err, Unit{}};
    }

    // Increment the ip.
    core.ip += 1;
    // Set op mode to `SIGNED`.=
    core.op_mode = OpMode::SIGNED;

    return {Error::None, Unit{}};
  }

  /**
   * Copy #1 pop bytes of memory from #3 pop to #2 stack_pop.
   * @return
   */
  auto i_copy_block() noexcept -> result<> {
    // Get the current core.
    auto &core = cores[cur_core_id];

    // Guard the stack for 3 pops.
    const auto guard_result = core.data.guard(3, 0);
    const auto guard_err = std::get<0>(guard_result);

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
    const auto cpy_result = mem.copy_block(len.to_uint32(), dst.to_uint32(), orig.to_uint32());
    const auto cpy_err = std::get<0>(cpy_result);

    if (cpy_err != Error::None) {
      return {cpy_err, Unit{}};
    }

    // Increment the ip.
    core.ip += 1;
    // Set op mode to `SIGNED`.
    core.op_mode = OpMode::SIGNED;

    return {Error::None, Unit{}};
  }

  /**
   * Compare first pop bytes of memory from third stack_pop to second pop.
   * @return Unit if the operation was successful. Error otherwise.
   */
  auto i_block_compare() noexcept -> result<> {
    // Get the current core.
    auto &core = cores[cur_core_id];

    // Guard the stack for 3 pops and 1 push.
    const auto guard_result = core.data.guard(3, 1);
    const auto guard_err = std::get<0>(guard_result);

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
    const auto cmp_result = mem.compare_block(len.to_uint32(), dst.to_uint32(), orig.to_uint32());
    const auto cmp_err = std::get<0>(cmp_result);
    const auto result = std::get<1>(cmp_result);
    if (cmp_err != Error::None) {
      return {cmp_err, Unit{}};
    }
    // Push the result.
    core.data.push(result);

    // Increment the ip.
    core.ip += 1;
    // Set op mode to `SIGNED`.
    core.op_mode = OpMode::SIGNED;

    return {Error::None, Unit{}};
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

    return {Error::None, Unit{}};
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

    return {Error::None, Unit{}};
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
    cur_core_id = CORE_COUNT - 1;

    goto fetch;

    fetch:
    {
      // Select the next core
      sel_next_core();
      // Get current core`s instruction pointer.
      const auto ip = cores[cur_core_id].ip;
      // Fetch the op code.
      const auto fetch_result = mem.fetch_opcode(ip);
      const auto fetch_err = std::get<0>(fetch_result);
      const auto op_code = std::get<1>(fetch_result);
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
      const auto err_result = i_nop();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_lw:
    {
      const auto err_result = i_load_word();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_lh:
    {
      const auto err_result = i_load_half();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_lb:
    {
      const auto err_result = i_load_byte();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_fw:
    {
      const auto err_result = i_fetch_word();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_fh:
    {
      const auto err_result = i_fetch_half();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_fb:
    {
      const auto err_result = i_fetch_byte();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_sw:
    {
      const auto err_result = i_store_word();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_sh:
    {
      const auto err_result = i_store_half();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_sb:
    {
      const auto err_result = i_store_byte();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_du:
    {
      const auto err_result = i_dupe();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_dr:
    {
      const auto err_result = i_drop();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_sp:
    {
      const auto err_result = i_swap();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_pu:
    {
      const auto err_result = i_push_address();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_po:
    {
      const auto err_result = i_pop_address();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_eq:
    {
      const auto err_result = i_equal();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_ne:
    {
      const auto err_result = i_not_equal();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_lt:
    {
      const auto err_result = i_less_than();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_gt:
    {
      const auto err_result = i_greater_than();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_ad:
    {
      const auto err_result = i_add();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_su:
    {
      const auto err_result = i_subtract();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_mu:
    {
      const auto err_result = i_multiply();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_dm:
    {
      const auto err_result = i_divide_remainder();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_md:
    {
      const auto err_result = i_multiply_divide_remainder();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_an:
    {
      const auto err_result = i_and();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_or:
    {
      const auto err_result = i_or();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_xo:
    {
      const auto err_result = i_xor();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_nt:
    {
      const auto err_result = i_not();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_sl:
    {
      const auto err_result = i_shift_left();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_sr:
    {
      const auto err_result = i_shift_right();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_pa:
    {
      const auto err_result = i_pack_bytes();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_un:
    {
      const auto err_result = i_unpack_bytes();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_rl:
    {
      const auto err_result = i_relative();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_ca:
    {
      const auto err_result = i_call();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_cc:
    {
      const auto err_result = i_conditional_call();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_ju:
    {
      const auto err_result = i_jump();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_cj:
    {
      const auto err_result = i_conditional_jump();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_re:
    {
      const auto err_result = i_return();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_cr:
    {
      const auto err_result = i_conditional_return();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_sv:
    {
      const auto err_result = i_set_interrupt();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_hi:
    {
      const auto err_result = i_halt_interrupts();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_si:
    {
      const auto err_result = i_start_interrupts();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_ti:
    {
      const auto err_result = i_trigger_interrupt();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_ii:
    {
      const auto err_result = i_invoke_io();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_hs:
    {
      const auto err_result = i_halt_system();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_ic:
    {
      const auto err_result = i_init_core();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_ac:
    {
      const auto err_result = i_activate_core();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_pc:
    {
      const auto err_result = i_pause_core();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_sc:
    {
      const auto err_result = i_suspend_cur_core();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_rr:
    {
      const auto err_result = i_read_register();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_wr:
    {
      const auto err_result = i_write_register();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_cp:
    {
      const auto err_result = i_copy_block();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_bc:
    {
      const auto err_result = i_block_compare();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_uu:
    {
      const auto err_result = i_unsigned_mode();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }
    l_ff:
    {
      const auto err_result = i_float_mode();
      const auto err = std::get<0>(err_result);

      if (err != Error::None) {
        return {err, Unit{}};
      }

      goto fetch;
    }

  }

 public:

  /**
   * Constructs the vm with empty io table
   */
  VM()
  noexcept {
    for (auto &core : cores) {
      core = Core{};
    }
    cores[0].active = true;
  }

  /**
   * Constructs the vm with a valid io table
   * @param io_table The IO table
   */
  explicit VM(IoTable io_table) : io_table{io_table} {
    for (auto &core : cores) {
      core = Core{};
    }
    cores[0].active = true;
  }

  /**
   * Loads the memory from a memory array.
   * @param is The input stream.
   */
  auto load_program(std::array<uint8_t, MEMORY_SIZE> prg, size_t prg_size) noexcept -> void {
    mem.load_program(prg, prg_size);
  }

  /**
   * Writes the byte into given memory address
   * @param addr The address of memory byte.
   * @param byte The byte of memory.
   * @return Result of the operation
   */
  result<> io_write(size_t addr, uint8_t byte) noexcept {
    return mem.write_io_byte(addr, byte);
  }

  result<uint8_t> io_read(size_t addr) noexcept {
    return mem.read_io_byte(addr);
  }

  auto run() noexcept -> void {
    interpret();
  }

  /**
   * Gets a snapshot of the vm
   * @return A snapshot of the vm
   */
  auto snapshot() noexcept -> VMSnapshot {
    auto core_snapshots = std::array<CoreSnapshot, CORE_COUNT>{};
    for (int i = 0; i < CORE_COUNT; ++i) {
      auto const snapshot = cores[i].snapshot();
      core_snapshots[i] = snapshot;
    }
    return {mem.snapshot(), int_table.snapshot(), io_table.snapshot(), core_snapshots, cur_core_id, int_enabled};
  }
};


#endif //ZAGROS