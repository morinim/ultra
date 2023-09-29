/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2023 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#if !defined(ULTRA_CONTRACTS_H)
#define      ULTRA_CONTRACTS_H

#include <cassert>

namespace ultra
{

#if defined(__clang__) || defined(__GNUC__)
#  define ULTRA_LIKELY(x) __builtin_expect(!!(x), 1)
#  define ULTRA_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#  define ULTRA_LIKELY(x) (!!(x))
#  define ULTRA_UNLIKELY(x) (!!(x))
#endif

#if defined(NDEBUG)

#define Expects(expression)
#define Ensures(expression)

#else

/// Preconditions can be stated in many ways, including comments, `if`
/// statements and `assert()`. This can make them hard to distinguish from
/// ordinary code, hard to update, hard to manipulate by tools and may have
/// the wrong semantics (do you always want to abort in debug mode and check
/// nothing in productions runs?).
/// \see C++ Core Guidelines I.6 <https://github.com/isocpp/CppCoreGuidelines/>
#define Expects(expression)  assert(ULTRA_LIKELY(expression))

/// Postconditions are often informally stated in a comment that states the
/// purpose of a function; `Ensures()` can be used to make this more
/// systematic, visible and checkable.
/// Postconditions are especially important when they relate to something that
/// isn't directly reflected in a returned result, such as a state of a data
/// structure used.
/// \note
/// Postconditions of the form "this resource must be released" are best
/// expressed by RAII.
/// \see C++ Core Guidelines I.8 <https://github.com/isocpp/CppCoreGuidelines/>
#define Ensures(expression)  assert(ULTRA_LIKELY(expression))

#endif

}  // namespace ultra

#endif  // include guard
