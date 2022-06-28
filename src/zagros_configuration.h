//
// Created by Arian Dashti on 6/27/22.
//

#ifndef ZAGROS_CONFIGURATION
#define ZAGROS_CONFIGURATION


#include <cstdlib>

// This file includes all the size configurations of the virtual machine

/// Size of the data stack
static const size_t DATA_STACK_SIZE = 32;

/// Size of the address stack
static const size_t ADDRESS_STACK_SIZE = 128;

/// Size of the register bank
static const size_t REGISTER_BANK_SIZE = 24;

/// Size of the memory
static const size_t MEMORY_SIZE = 65535;

/// Size of the interrupt table
static const size_t INTERRUPT_TABLE_SIZE = 128;

/// Size of the I/O Table
static const size_t IO_TABLE_SIZE = 16;

/// Beginning valid memory address for I/Os
static const size_t IO_MEMORY_ADDRESS_BEGIN = 0;

/// Ending memory address for I/Os (exclusive)
static const size_t IO_MEMORY_ADDRESS_END = 192;

/// Number of cores of the virtual machine
static const size_t CORE_COUNT = 2;



#endif //ZAGROS_CONFIGURATION