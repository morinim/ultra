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

#if !defined(ULTRA_TERMINAL_H)
#define      ULTRA_TERMINAL_H

#include "kernel/symbol.h"
#include "kernel/value.h"

namespace ultra
{

///
/// Leaf symbol in a genetic programming expression.
///
/// A terminal represents an atomic value in a GP expression tree or linear
/// program. Unlike functions, terminals do not take arguments and therefore
/// have arity zero.
///
/// Typical examples of terminals include:
/// - input variables (e.g. program inputs or features);
/// - constant literals;
/// - nullary functions (functions with no parameters).
///
/// Terminals are evaluated by directly producing a value, rather than
/// combining the results of sub-expressions. As such, they always appear as
/// leaves in the program structure.
///
/// \see symbol
///
class terminal : public symbol
{
public:
  using symbol::symbol;

  /// Produce the runtime value of the terminal.
  ///
  /// This function returns the value represented by the terminal when the
  /// program is evaluated. The returned value may depend on the evaluation
  /// context (e.g. the current input vector), but does not depend on any
  /// child expressions.
  ///
  /// \return the value associated with this terminal
  ///
  /// \note
  /// Implementations must not throw exceptions and should be side-effect
  /// free to preserve referential transparency during program evaluation.
  [[nodiscard]] virtual value_t instance() const = 0;
};

/// Constraint for types modelling a GP terminal.
///
/// This concept is satisfied by any type that derives from `ultra::terminal`.
/// It is intended for use in templates that operate specifically on
/// terminal symbols, enabling clearer intent and improved diagnostics.
///
template<class T> concept Terminal = std::derived_from<T, terminal>;

}  // namespace ultra

#endif  // include guard
