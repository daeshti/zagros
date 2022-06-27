%module(directors="1") Zagros

%{
 /* Includes the header in the wrapper code */
#include "../src/callback.hpp"
#include "../src/result.hpp"
#include "../src/instruction_mode.hpp"
#include "../src/cell.hpp"
#include "../src/io.h"
#include "../srs/snapshot.hpp"
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
%include "std_tuple.i"
%template(Triple) std::tuple<zagros::Error, zagros::Cell, zagros::Cell>;

%include "../src/result.hpp"
%template() zagros::result<zagros::Cell>;
%template() zagros::result<zagros::Unit>;
%template() zagros::result<uint8_t>;
%template(CellResult) std::pair<zagros::Error, zagros::Cell>;
%template(UnitResult) std::pair<zagros::Error, zagros::Unit>;
%template(ByteResult) std::pair<zagros::Error, uint8_t>;


%include "../src/instruction_mode.hpp"

%template(CellArray) std::array<uint8_t, 4>;

%include "../src/cell.hpp"

%template(StringVector) std::vector<std::string>;
%template(MemoryArray) std::array<uint8_t, MEMORY_SIZE >;
%template(AddressArray) std::array<zagros::Cell,ADDRESS_STACK_SIZE >;
%template(DataArray) std::array<zagros::Cell,DATA_STACK_SIZE >;
%template(InterruptArray) std::array<zagros::Cell,INTERRUPT_TABLE_SIZE >;
%template(RegisterArray) std::array<zagros::Cell,REGISTER_BANK_SIZE >;

%include "../src/snapshot.hpp"
%template(CoreSnapshotArray) std::array<zagros::CoreSnapshot,CORE_COUNT >;

%template(CallbackArray) std::array<zagros::Callback*, zagros::IO_TABLE_SIZE>;
%include "../src/io.h"

/* Parse the header file to generate wrappers */
%include "../src/vm.hpp"


