/* -----------------------------------------------------------------------------
* std_tuple.i
    *
    * SWIG typemaps for std::tuple<a,b,c>
* ----------------------------------------------------------------------------- */

%include <std_common.i>
%include <exception.i>

// ------------------------------------------------------------------------
// std::tuple
// ------------------------------------------------------------------------

%{
#include <utility>
%}

namespace std {

    template<class T, class U, class V> struct tuple {
      typedef T first_type;
      typedef U second_type;
      typedef V third_type;

      tuple();
      tuple(T first, U second, V third);
      tuple(const tuple& other);

      template <class U1, class U2, class U3> pair(const tuple<U1, U2, U3> &other);

      T first;
      U second;
      V third;
    };

    // add specializations here

}