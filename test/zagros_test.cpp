#include "gtest/gtest.h"
#include "../src/zagros.hpp"

using Zagros::Error;
using Zagros::Cell;
using Zagros::DataStack;
using Zagros::AddressStack;
using Zagros::RegisterBank;
using Zagros::Memory;
using Zagros::InterruptTable;
using Zagros::Core;
using Zagros::VMSnapshot;
using Zagros::VM;

TEST(DataStack, PushPop) {
  auto stack = DataStack<32>{};
  stack.push(Cell{1});
  stack.push(Cell{0});
  EXPECT_EQ(stack.pop(), Cell{0});
  EXPECT_EQ(stack.pop(), Cell{1});
}

TEST(DataStack, PushPopEmptyUnderflows) {
  auto stack = DataStack<32>{};
  auto const &[underflow, _] = stack.guard(1, 0);
  ASSERT_EQ(underflow, Error::DataStackUnderflow);
}

TEST(DataStack, PushPopFull) {
  auto stack = DataStack<32>{};
  for (uint32_t i = 0; i < 32; ++i) {
    stack.push(Cell{i});
  }

  for (uint32_t i = 0; i < 32; ++i) {
    EXPECT_EQ(stack.pop(), Cell{32 - i - 1});
  }
}

TEST(DataStack, PushPopFullOverflows) {
  auto stack = DataStack<32>{};

  for (uint32_t i = 0; i < 32; ++i) {
    stack.push(Cell{i});
  }

  auto const &[overflow, _] = stack.guard(0, 1);
  EXPECT_EQ(overflow, Error::DataStackOverflow);
}

TEST(DataStack, PushPopFullUnderflows) {
  auto stack = DataStack<32>{};
  for (uint32_t i = 0; i < 32; ++i) {
    stack.push(Cell{i});
  }

  for (uint32_t i = 0; i < 32; ++i) {
    EXPECT_EQ(stack.pop(), Cell{32 - i - 1});
  }

  auto const &[underflow, _] = stack.guard(1, 0);
  ASSERT_EQ(underflow, Error::DataStackUnderflow);
}

TEST(DataStack, ClearWorks) {
  auto stack = DataStack<32>{};
  for (uint32_t i = 0; i < 32; ++i) {
    stack.push(Cell{i});
  }

  stack.clear();
  auto const &[underflow, _] = stack.guard(1, 0);
  ASSERT_EQ(underflow, Error::DataStackUnderflow);
}

TEST(DataStack, SnapshotWorks) {
  auto stack = DataStack<32>{};
  for (uint32_t i = 0; i < 32; ++i) {
    stack.push(Cell{i});
  }

  auto const snapshot = stack.snapshot();
  ASSERT_EQ(snapshot.get_top(), 32);

  auto const &snapshot_arr = snapshot.get_arr();
  for (uint32_t i = 0; i < 32; ++i) {
    EXPECT_EQ(snapshot_arr[i], Cell{i});
  }
}

TEST(AddressStack, PushPop) {
  auto stack = AddressStack<128>{};
  stack.push(Cell{1});
  stack.push(Cell{0});
  auto const &[none_1, zero] = stack.pop();
  auto const &[none_2, one] = stack.pop();

  ASSERT_EQ(none_1, Error::None);
  ASSERT_EQ(zero, Cell{0});
  ASSERT_EQ(none_2, Error::None);
  ASSERT_EQ(one, Cell{1});
}

TEST(AddressStack, PushPopEmptyUnderflows) {
  auto stack = AddressStack<128>{};
  auto const &[underflow, _] = stack.pop();
  ASSERT_EQ(underflow, Error::AddressStackUnderflow);
}

TEST(AddressStack, PushPopFull) {
  auto stack = AddressStack<128>{};
  for (uint32_t i = 0; i < 128; ++i) {
    auto const &[none, _] = stack.push(Cell{i});
    EXPECT_EQ(none, Error::None);
  }

  for (uint32_t i = 0; i < 128; ++i) {
    auto const &[none, popped] = stack.pop();
    EXPECT_EQ(none, Error::None);
    EXPECT_EQ(popped, Cell{128 - i - 1});
  }
}

TEST(AddressStack, PushPopFullOverflows) {
  auto stack = AddressStack<128>{};

  for (uint32_t i = 0; i < 128; ++i) {
    auto const &[none, _] = stack.push(Cell{i});
    EXPECT_EQ(none, Error::None);
  }

  auto const &[overflow, _] = stack.push(Cell{128});
  ASSERT_EQ(overflow, Error::AddressStackOverflow);
}

TEST(AddressStack, PushPopFullUnderflows) {
  auto stack = AddressStack<128>{};
  for (uint32_t i = 0; i < 128; ++i) {
    auto const &[none, _] = stack.push(Cell{i});
    EXPECT_EQ(none, Error::None);
  }

  for (uint32_t i = 0; i < 128; ++i) {
    auto const &[none, popped] = stack.pop();
    EXPECT_EQ(none, Error::None);
    EXPECT_EQ(popped, Cell{128 - i - 1});
  }

  auto const &[underflow, _] = stack.pop();
  ASSERT_EQ(underflow, Error::AddressStackUnderflow);
}

TEST(AddressStack, ClearWorks) {
  auto stack = AddressStack<128>{};
  for (uint32_t i = 0; i < 128; ++i) {
    auto const &[none, _] = stack.push(Cell{i});
    EXPECT_EQ(none, Error::None);
  }

  stack.clear();
  auto const &[underflow, _] = stack.pop();
  ASSERT_EQ(underflow, Error::AddressStackUnderflow);
}

TEST(AddressStack, SnapshotWorks) {
  auto stack = AddressStack<128>{};
  for (uint32_t i = 0; i < 128; ++i) {
    stack.push(Cell{i});
  }

  auto const snapshot = stack.snapshot();
  ASSERT_EQ(snapshot.get_top(), 128);

  auto const &snapshot_arr = snapshot.get_arr();
  for (uint32_t i = 0; i < 128; ++i) {
    EXPECT_EQ(snapshot_arr[i], Cell{i});
  }
}

TEST(RegisterBank, ReadWriteWorks) {
  auto bank = RegisterBank<24>{};
  for (uint32_t i = 0; i < 24; ++i) {
    auto const &[none, _] = bank.write(i, Cell{i});
    ASSERT_EQ(none, Error::None);
  }

  for (uint32_t i = 0; i < 24; ++i) {
    auto const &[none, read] = bank.read(i);
    ASSERT_EQ(none, Error::None);
    ASSERT_EQ(read, Cell{i});
  }
}

TEST(RegisterBank, ReturnsErrorOnIllegalRegisterId) {
  auto bank = RegisterBank<24>{};
  auto const &[read_err, _1] = bank.read(24);
  ASSERT_EQ(read_err, Error::IllegalRegisterId);
  auto const &[write_err, _2] = bank.write(24, Cell{0});
  ASSERT_EQ(write_err, Error::IllegalRegisterId);
}

TEST(RegisterBank, ClearWorks) {
  auto bank = RegisterBank<24>{};
  for (uint32_t i = 0; i < 24; ++i) {
    auto const &[none, _] = bank.write(i, Cell{i});
    EXPECT_EQ(none, Error::None);
  }
  bank.clear();
  for (uint32_t i = 0; i < 24; ++i) {
    auto const &[none, read] = bank.read(i);
    EXPECT_EQ(none, Error::None);
    ASSERT_EQ(read, Cell{0});
  }
}

TEST(RegisterBank, SnapshotWorks) {
  auto bank = RegisterBank<24>{};
  for (uint32_t i = 0; i < 24; ++i) {
    auto const &[none, _] = bank.write(i, Cell{i});
    EXPECT_EQ(none, Error::None);
  }

  auto const snapshot = bank.snapshot();
  auto const &snapshot_arr = snapshot.get_arr();
  for (uint32_t i = 0; i < 24; ++i) {
    EXPECT_EQ(snapshot_arr[i], Cell{i});
  }
}

TEST(Memory, ReadWriteWorks) {
  auto memory = Memory<65535>{};
  for (uint32_t i = 0; i < 65535; ++i) {
    Cell value = Cell{i % 256};
    auto const &[none, _] = memory.write_bytes<1>(i, value);
    EXPECT_EQ(none, Error::None);
  }

  for (uint32_t i = 0; i < 65535; ++i) {
    uint32_t value = i % 256;
    auto const &[none, read] = memory.read_bytes<1>(i);
    EXPECT_EQ(none, Error::None);
    EXPECT_EQ(read, Cell{value});
  }
}

TEST(Memory, CompareBlockWorks) {
  auto memory = Memory<65535>{};
  for (uint32_t i = 0; i < 65535; ++i) {
    Cell value = Cell{i % 256};
    auto const &[none, _] = memory.write_bytes<1>(i, value);
    EXPECT_EQ(none, Error::None);
  }

  auto const &[none, res] = memory.compare_block(0, 0, 65535);
  EXPECT_EQ(none, Error::None);
  EXPECT_EQ(res, Cell{true});
}

TEST(Memory, CopyBlockWorks) {
  auto memory = Memory<65535>{};
  for (uint32_t i = 0; i < 32767; ++i) {
    Cell value = Cell{i % 256};
    auto const &[none, _] = memory.write_bytes<1>(i, value);
    EXPECT_EQ(none, Error::None);
  }

  {
    auto const &[none, _] = memory.copy_block(32767, 32767, 0);
    EXPECT_EQ(none, Error::None);
  }

  for (uint32_t i = 32767; i < 65534; ++i) {
    Cell value = Cell{(i - 32767) % 256};
    auto const &[none, read] = memory.read_bytes<1>(i);
    EXPECT_EQ(none, Error::None);
    EXPECT_EQ(read, value);
  }
  {
    auto const &[none, read] = memory.read_bytes<1>(65534);
    EXPECT_EQ(none, Error::None);
    EXPECT_EQ(read, Cell{0});
  }
}

TEST(Memory, LoadProgramWorks) {
  std::array<uint8_t, 65535UL> prg{};
  for (uint32_t i = 0; i < 65535; ++i) {
    prg[i] = i % 256;
  }

  auto memory = Memory<65535>{};

  {
    auto const &[none, _] = memory.load_program(prg, prg.size());
    EXPECT_EQ(none, Error::None);
  }

  for (uint32_t i = 0; i < 65535; ++i) {
    Cell value = Cell{i % 256};
    auto const &[none, read] = memory.read_bytes<1>(i);
    EXPECT_EQ(none, Error::None);
    EXPECT_EQ(read, value);
  }
}

TEST(Memory, ClearWorks) {
  auto memory = Memory<65535>{};
  for (uint32_t i = 0; i < 65535; ++i) {
    Cell value = Cell{i % 256};
    auto const &[none, _] = memory.write_bytes<1>(i, value);
    EXPECT_EQ(none, Error::None);
  }
  memory.clear();
  for (uint32_t i = 0; i < 65535; ++i) {
    auto const &[none, read] = memory.read_bytes<1>(i);
    EXPECT_EQ(none, Error::None);
    EXPECT_EQ(read, Cell{0});
  }
}

TEST(Memory, SnapshotWorks) {
  auto memory = Memory<65535>{};
  for (uint32_t i = 0; i < 65535; ++i) {
    Cell value = Cell{i % 256};
    auto const &[none, _] = memory.write_bytes<1>(i, value);
    EXPECT_EQ(none, Error::None);
  }

  auto const snapshot = memory.snapshot();
  auto const &snapshot_arr = snapshot.get_arr();
  for (uint32_t i = 0; i < 65535; ++i) {
    EXPECT_EQ(snapshot_arr[i], i % 256);
  }
}

TEST(InterruptTable, ReadWriteWorks) {
  auto interrupt_table = InterruptTable<128>{};
  for (uint32_t i = 0; i < 128; ++i) {
    uint8_t value = i % 256;
    auto const &[none, _] = interrupt_table.set(i, Cell{value});
    EXPECT_EQ(none, Error::None);
  }

  for (uint32_t i = 0; i < 128; ++i) {
    auto value = Cell{i % 256};
    auto const &[none, read] = interrupt_table.get(i);
    EXPECT_EQ(none, Error::None);
    EXPECT_EQ(read, value);
  }
}

TEST(InterruptTable, ClearWorks) {
  auto interrupt_table = InterruptTable<128>{};
  for (uint32_t i = 0; i < 128; ++i) {
    Cell value = Cell{i % 256};
    auto const &[none, _] = interrupt_table.set(i, value);
    EXPECT_EQ(none, Error::None);
  }
  interrupt_table.clear();
  for (uint32_t i = 0; i < 128; ++i) {
    auto const &[none, read] = interrupt_table.get(i);
    EXPECT_EQ(none, Error::None);
    EXPECT_EQ(read, Cell{0});
  }
}

TEST(InterruptTable, SnapshotWorks) {
  auto interrupt_table = InterruptTable<128>{};
  for (uint32_t i = 0; i < 128; ++i) {
    Cell value = Cell{i % 256};
    auto const &[none, _] = interrupt_table.set(i, value);
    EXPECT_EQ(none, Error::None);
  }

  auto const snapshot = interrupt_table.snapshot();
  auto const &snapshot_arr = snapshot.get_arr();
  for (uint32_t i = 0; i < 128; ++i) {
    EXPECT_EQ(snapshot_arr[i], Cell{i % 256});
  }
}

TEST(Core, SnapshotWorks) {
  // TODO: implement
}

// TODO: Add tests for VMSnapshot constructor
TEST(VM, SnapshotWorks) {

}

enum class OpCode {
  NO,
  LW,
  LH,
  LB,
  FW,
  FH,
  FB,
  SW,
  SH,
  SB,
  DU,
  DR,
  SP,
  PU,
  PO,
  EQ,
  NE,
  LT,
  GT,
  AD,
  SU,
  MU,
  DM,
  MD,
  AN,
  OR,
  XO,
  NT,
  SL,
  SR,
  PA,
  UN,
  RL,
  CA,
  CC,
  JU,
  CJ,
  RE,
  CR,
  SV,
  HI,
  SI,
  TI,
  II,
  HS,
  IC,
  AC,
  PC,
  SC,
  RR,
  WR,
  CP,
  BC,
  UU,
  FF,
};

using program = std::vector<std::variant<OpCode, uint8_t, uint16_t, uint32_t>>;

template<class... Ts>
struct overloaded : Ts ... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

auto loaded_vm(const program &prg) -> VM<> {
  std::vector<uint8_t> bytes;

  for (const auto &instr : prg) {
    std::visit(overloaded{
        [&bytes](OpCode op) {
          bytes.push_back(static_cast<uint8_t>(op));
        },
        [&bytes](uint8_t value) {
          bytes.push_back(value);
        },
        [&bytes](uint16_t value) {
          bytes.push_back(value & 0xFF);
          bytes.push_back(value >> 8);
        },
        [&bytes](uint32_t value) {
          bytes.push_back(value & 0xFF);
          bytes.push_back((value >> 8) & 0xFF);
          bytes.push_back((value >> 16) & 0xFF);
          bytes.push_back(value >> 24);
        },
    }, instr);
  }

  VM<> vm = {};
  std::array<uint8_t, 65535> byte_arr{};
  std::copy_n(bytes.begin(), 65535, byte_arr.begin());
  vm.load_program(byte_arr, bytes.size());
  return vm;
}


TEST(VM, LoadMemoryWorks) {
  program prg;
  for (int opcode_int = static_cast<int>(OpCode::NO); opcode_int != static_cast<int>(OpCode::FF); ++opcode_int) {
    auto opcode = static_cast<OpCode>(opcode_int);
    prg.push_back(opcode);
  }
  prg.push_back((uint16_t)UINT16_MAX);
  prg.push_back((uint32_t)UINT32_MAX);

  auto vm = loaded_vm(prg);
  auto const &mem = vm.snapshot().get_mem().get_arr();

  int opcode_int;
  for (opcode_int = static_cast<int>(OpCode::NO); opcode_int != static_cast<int>(OpCode::FF); ++opcode_int) {
    auto const read = mem[opcode_int];
    EXPECT_EQ(read, opcode_int);
  }
  auto const read_16 = mem[opcode_int] | (mem[opcode_int + 1] << 8);
  EXPECT_EQ(read_16, UINT16_MAX);
  auto const read_32 = mem[opcode_int] | (mem[opcode_int + 1] << 8) | (mem[opcode_int + 2] << 16) | (mem[opcode_int + 3] << 24);
  EXPECT_EQ(read_32, UINT32_MAX);
}



TEST(VM, COMPILES) {
  VM<> vm = {};
  vm.run();
}

TEST(VM, InstructionHaltSystemWorks) {
  program prg;
  prg.push_back(OpCode::HS); // 00
  auto vm = loaded_vm(prg);
  vm.run();
  auto const &ss = vm.snapshot();
  auto &core = ss.get_cores()[0];
  ASSERT_EQ(core.get_ip(), 0);
  ASSERT_EQ(core.get_op_mode(), Zagros::OpMode::SIGNED);
}

TEST(VM, InstructionNopWorks) {
  program prg;
  prg.push_back(OpCode::NO); // 00
  prg.push_back(OpCode::HS); // 01
  auto vm = loaded_vm(prg);
  vm.run();
  auto const &ss = vm.snapshot();
  auto &core = ss.get_cores()[0];
  ASSERT_EQ(core.get_ip(), 1);
  ASSERT_EQ(core.get_op_mode(), Zagros::OpMode::SIGNED);
}

template<size_t S>
auto stack_pop(Zagros::StackSnapshot<S> const &ss, size_t i) -> Cell {
  return ss.get_arr()[ss.get_top() - 1 - i];
}

TEST(VM, InstructionLoadWordWorks) {
  program prg;
  prg.push_back(OpCode::LW); // 00
  prg.push_back(OpCode::NO); // 01
  prg.push_back(OpCode::NO); // 02
  prg.push_back(OpCode::NO); // 03
  prg.push_back((uint32_t)1337); // 04
  prg.push_back(OpCode::HS); // 05
  auto vm = loaded_vm(prg);
  vm.run();
  auto const &ss = vm.snapshot();
  auto &core = ss.get_cores()[0];
  auto pop = stack_pop(core.get_data(), 0);
  ASSERT_EQ(pop, Cell{1337});
  ASSERT_EQ(core.get_ip(), 8);
  ASSERT_EQ(core.get_op_mode(), Zagros::OpMode::SIGNED);
}

TEST(VM, InstructionLoadHalfWorks) {
  program prg;
  prg.push_back(OpCode::LH); // 00
  prg.push_back((uint16_t)1337); // 01
  prg.push_back(OpCode::HS); // 03
  auto vm = loaded_vm(prg);
  vm.run();
  auto const &ss = vm.snapshot();
  auto &core = ss.get_cores()[0];
  auto pop = stack_pop(core.get_data(), 0);
  ASSERT_EQ(pop, Cell{1337});
  ASSERT_EQ(core.get_ip(), 3);
  ASSERT_EQ(core.get_op_mode(), Zagros::OpMode::SIGNED);
}

TEST(VM, InstructionLoadByteWorks) {
  program prg;
  prg.push_back(OpCode::LB); // 00
  prg.push_back((uint8_t)137); // 01
  prg.push_back(OpCode::HS); // 02
  auto vm = loaded_vm(prg);
  vm.run();
  auto const &ss = vm.snapshot();
  auto &core = ss.get_cores()[0];
  auto pop = stack_pop(core.get_data(), 0);
  ASSERT_EQ(pop, Cell{137});
  ASSERT_EQ(core.get_ip(), 2);
  ASSERT_EQ(core.get_op_mode(), Zagros::OpMode::SIGNED);
}

TEST(VM, InstructionJumpWorks) {
  program prg;
  prg.push_back(OpCode::LB); // 00
  prg.push_back((uint8_t)8); // 01
  prg.push_back(OpCode::JU); // 02
  prg.push_back(OpCode::NO); // 03
  prg.push_back(OpCode::NO); // 04
  prg.push_back(OpCode::NO); // 05
  prg.push_back(OpCode::NO); // 06
  prg.push_back(OpCode::NO); // 07
  prg.push_back(OpCode::HS); // 08
  auto vm = loaded_vm(prg);
  vm.run();
  auto const &ss = vm.snapshot();
  auto &core = ss.get_cores()[0];
  ASSERT_EQ(core.get_ip(), 8);
  ASSERT_EQ(core.get_op_mode(), Zagros::OpMode::SIGNED);
}

TEST(VM, InstructionFetchWordWorks) {
  program prg;
  prg.push_back(OpCode::LB); // 00
  prg.push_back((uint8_t)7); // 01
  prg.push_back(OpCode::JU); // 02
  prg.push_back((uint32_t)1337); // 03
  prg.push_back(OpCode::LB); // 07
  prg.push_back((uint8_t)3); // 08
  prg.push_back(OpCode::FW); // 09
  prg.push_back(OpCode::HS); // 10
  auto vm = loaded_vm(prg);
  vm.run();
  auto const &ss = vm.snapshot();
  auto &core = ss.get_cores()[0];
  auto pop = stack_pop(core.get_data(), 0);
  ASSERT_EQ(pop, Cell{1337});
  ASSERT_EQ(core.get_ip(), 10);
  ASSERT_EQ(core.get_op_mode(), Zagros::OpMode::SIGNED);
}

TEST(VM, InstructionFetchHalfWorks) {
  program prg;
  prg.push_back(OpCode::LB); // 00
  prg.push_back((uint8_t)5); // 01
  prg.push_back(OpCode::JU); // 02
  prg.push_back((uint16_t)1337); // 03
  prg.push_back(OpCode::LB); // 05
  prg.push_back((uint8_t)3); // 06
  prg.push_back(OpCode::FH); // 07
  prg.push_back(OpCode::HS); // 08
  auto vm = loaded_vm(prg);
  vm.run();
  auto const &ss = vm.snapshot();
  auto &core = ss.get_cores()[0];
  auto pop = stack_pop(core.get_data(), 0);
  ASSERT_EQ(pop, Cell{1337});
  ASSERT_EQ(core.get_ip(), 8);
  ASSERT_EQ(core.get_op_mode(), Zagros::OpMode::SIGNED);
}

TEST(VM, InstructionFetchByteWorks) {
  program prg;
  prg.push_back(OpCode::LB); // 00
  prg.push_back((uint8_t)4); // 01
  prg.push_back(OpCode::JU); // 02
  prg.push_back((uint8_t)137); // 03
  prg.push_back(OpCode::LB); // 04
  prg.push_back((uint8_t)3); // 05
  prg.push_back(OpCode::FB); // 06
  prg.push_back(OpCode::HS); // 07
  auto vm = loaded_vm(prg);
  vm.run();
  auto const &ss = vm.snapshot();
  auto &core = ss.get_cores()[0];
  auto pop = stack_pop(core.get_data(), 0);
  ASSERT_EQ(pop, Cell{137});
  ASSERT_EQ(core.get_ip(), 7);
  ASSERT_EQ(core.get_op_mode(), Zagros::OpMode::SIGNED);
}

TEST(VM, InstructionStoreWordWorks) {
  program prg;
  prg.push_back(OpCode::LB); // 00
  prg.push_back((uint8_t)7); // 01
  prg.push_back(OpCode::JU); // 02
  prg.push_back((uint32_t)0); // 03
  prg.push_back(OpCode::LW); // 07
  prg.push_back(OpCode::NO); // 08
  prg.push_back(OpCode::NO); // 09
  prg.push_back(OpCode::NO); // 10
  prg.push_back((uint32_t)1337); // 11
  prg.push_back(OpCode::LB); // 15
  prg.push_back((uint8_t)3); // 16
  prg.push_back(OpCode::SW); // 17
  prg.push_back(OpCode::HS); // 18
  auto vm = loaded_vm(prg);
  vm.run();
  auto const &ss = vm.snapshot();
  auto core = ss.get_cores()[0];
  auto mem = ss.get_mem().get_arr();
  auto word = mem[3] | (mem[4] << 8) | (mem[5] << 16) | (mem[6] << 24);
  ASSERT_EQ(word, 1337);
  ASSERT_EQ(core.get_ip(), 18);
  ASSERT_EQ(core.get_op_mode(), Zagros::OpMode::SIGNED);
}

TEST(VM, InstructionStoreHalfWorks) {
  program prg;
  prg.push_back(OpCode::LB); // 00
  prg.push_back((uint8_t)5); // 01
  prg.push_back(OpCode::JU); // 02
  prg.push_back((uint16_t)0); // 03
  prg.push_back(OpCode::LH); // 05
  prg.push_back((uint16_t)1337); // 06
  prg.push_back(OpCode::LB); // 08
  prg.push_back((uint8_t)3); // 09
  prg.push_back(OpCode::SH); // 10
  prg.push_back(OpCode::HS); // 10
  auto vm = loaded_vm(prg);
  vm.run();
  auto const &ss = vm.snapshot();
  auto core = ss.get_cores()[0];
  auto mem = ss.get_mem().get_arr();
  auto half = mem[3] | (mem[4] << 8) | (0 << 16) | (0 << 24);
  ASSERT_EQ(half, 1337);
  ASSERT_EQ(core.get_ip(), 11);
  ASSERT_EQ(core.get_op_mode(), Zagros::OpMode::SIGNED);
}

TEST(VM, InstructionStoreByteWorks) {
  program prg;
  prg.push_back(OpCode::LB); // 00
  prg.push_back((uint8_t)4); // 01
  prg.push_back(OpCode::JU); // 02
  prg.push_back((uint8_t)0); // 03
  prg.push_back(OpCode::LB); // 04
  prg.push_back((uint8_t)137); // 05
  prg.push_back(OpCode::LB); // 06
  prg.push_back((uint8_t)3); // 07
  prg.push_back(OpCode::SH); // 08
  prg.push_back(OpCode::HS); // 09
  auto vm = loaded_vm(prg);
  vm.run();
  auto const &ss = vm.snapshot();
  auto core = ss.get_cores()[0];
  auto mem = ss.get_mem().get_arr();
  auto byte = mem[3] | (0 << 8) | (0 << 16) | (0 << 24);
  ASSERT_EQ(byte, 137);
  ASSERT_EQ(core.get_ip(), 9);
  ASSERT_EQ(core.get_op_mode(), Zagros::OpMode::SIGNED);
}

TEST(VM, InstructDupeWorks) {
  program prg;
  prg.push_back(OpCode::LB); // 00
  prg.push_back((uint8_t)137); // 01
  prg.push_back(OpCode::DU); // 02
  prg.push_back(OpCode::HS); // 03
  auto vm = loaded_vm(prg);
  vm.run();
  auto const &ss = vm.snapshot();
  auto core = ss.get_cores()[0];
  auto pop0 = stack_pop(core.get_data(), 0);
  auto pop1 = stack_pop(core.get_data(), 1);
  ASSERT_EQ(pop0, Cell{137});
  ASSERT_EQ(pop1, Cell{137});
  ASSERT_EQ(core.get_ip(), 3);
  ASSERT_EQ(core.get_op_mode(), Zagros::OpMode::SIGNED);
}

TEST(VM, InstructionDropWorks) {
  program prg;
  prg.push_back(OpCode::LB); // 00
  prg.push_back((uint8_t)137); // 01
  prg.push_back(OpCode::DR); // 02
  prg.push_back(OpCode::HS); // 03
  auto vm = loaded_vm(prg);
  vm.run();
  auto const &ss = vm.snapshot();
  auto core = ss.get_cores()[0];
  auto top = core.get_data().get_top();
  ASSERT_EQ(top, 0);
  ASSERT_EQ(core.get_ip(), 3);
  ASSERT_EQ(core.get_op_mode(), Zagros::OpMode::SIGNED);
}

TEST(VM, InstructionSwapWorks) {
  program prg;
  prg.push_back(OpCode::LB); // 00
  prg.push_back((uint8_t)255); // 01
  prg.push_back(OpCode::LB); // 02
  prg.push_back((uint8_t)0); // 03
  prg.push_back(OpCode::SP); // 04
  prg.push_back(OpCode::HS); // 05
  auto vm = loaded_vm(prg);
  vm.run();
  auto const &ss = vm.snapshot();
  auto core = ss.get_cores()[0];
  auto pop0 = stack_pop(core.get_data(), 0);
  auto pop1 = stack_pop(core.get_data(), 1);
  ASSERT_EQ(pop0, Cell{255});
  ASSERT_EQ(pop1, Cell{0});
  ASSERT_EQ(core.get_ip(), 5);
  ASSERT_EQ(core.get_op_mode(), Zagros::OpMode::SIGNED);
}



