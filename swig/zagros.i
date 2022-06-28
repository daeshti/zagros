%module(directors="1") Zagros

%{
 /* Includes the header in the wrapper code */
#include "../src/vm.hpp"
%}

%feature("director") Callback;

%rename(assign) operator=;
%rename(equals) operator==;

%inline %{
typedef unsigned int uint32_t;
%}

%inline %{
typedef int int32_t;
%}

%inline %{
typedef unsigned char uint8_t;
%}

%include <std_string.i>

%include "../src/callback.hpp"
%include <std_array.i>
%include <std_pair.i>
%include <std_vector.i>

%include "../src/result.hpp"
%template() outcome<Cell>;
%template() outcome<Unit>;
%template() outcome<uint8_t>;
%template(CellResult) std::pair<Error, Cell>;
%template(UnitResult) std::pair<Error, Unit>;
%template(ByteResult) std::pair<Error, uint8_t>;
%template(TripleResult) Triple<Error, Cell, Cell>;


%include "../src/instruction_mode.hpp"

%template(CellArray) std::array<uint8_t, 4>;

%include "../src/cell.hpp"

%template(StringVector) std::vector<std::string>;
%template(MemoryArray) std::array<uint8_t, MEMORY_SIZE >;
%template(AddressArray) std::array<Cell,ADDRESS_STACK_SIZE >;
%template(DataArray) std::array<Cell,DATA_STACK_SIZE >;
%template(InterruptArray) std::array<Cell,INTERRUPT_TABLE_SIZE >;
%template(RegisterArray) std::array<Cell,REGISTER_BANK_SIZE >;

%include "../src/snapshot.hpp"
%template(CoreSnapshotArray) std::array<CoreSnapshot,CORE_COUNT >;

%template(CallbackArray) std::array<Callback*, IO_TABLE_SIZE>;
%include "../src/io.h"

/* Parse the header file to generate wrappers */
%include "../src/vm.hpp"


