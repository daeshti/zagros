//
// Created by Arian Dashti on 6/27/22.
//

#ifndef ZAGROS_RESULT
#define ZAGROS_RESULT

#include <utility>

/**
 * A struct to represent an empty / void value.
 */
enum class Unit {
};

/**
 * An enum to represent the different reasons to stop the current interpreting flow.
 */
enum class Error {
  /// The operation was successful.
  None,

  /// The operation failed because it`s attempting to divide_remainder by zero.
  DivisionByZero,

  /// The operation failed because it`s attempting to do illegal instruction on float mode.
  InvalidFloatOperation,

  /// The operation failed because not enough memory was available.
  OutOfMemory,

  /// The operation failed because not enough space is available on the arr stack.
  DataStackOverflow,

  /// The operation failed because not enough elements are available on the arr stack.
  DataStackUnderflow,

  /// The operation failed because not enough space is available on the addrs stack.
  AddressStackOverflow,

  /// The operation failed because not enough elements are available on the addrs stack.
  AddressStackUnderflow,

  /// The operation failed because of illegal register id.
  IllegalRegisterId,

  /// The operation failed because of illegal memory addrs.
  IllegalMemoryAddress,

  /// The operation failed because of illegal interrupt id.
  IllegalInterruptId,

  /// System should successfully halted.
  SystemHalt
};

/**
 * A type alias for return values.
 * Since we`re not using exceptions we use a product type to represent the (outcome * error) of an operation.
 * Unit as type parameter will be optimized by the compiler because it`s an empty struct.
 */
template<class T=Unit>
using outcome = std::pair<Error, T>;

template<class T, class U, class V>
struct Triple {
  typedef T first_type;
  typedef U second_type;
  typedef V third_type;

  Triple() = default;;
  Triple(T first, U second, V third) : first(first), second(second), third(third) {}
  Triple(const Triple &other) : first(other.first), second(other.second), third(other.third) {}

  template<class U1, class U2, class U3>
  Triple(const Triple<U1, U2, U3> &other) : first(other.first), second(other.second), third(other.third) {}

  T first;
  U second;
  V third;
};

#endif //ZAGROS_RESULT
