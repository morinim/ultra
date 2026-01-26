/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2024 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#if !defined(ULTRA_FUNCTION_H)
#define      ULTRA_FUNCTION_H

#include "kernel/symbol.h"
#include "kernel/value.h"

#include <vector>

namespace ultra
{
///
/// A callable GP symbol with one or more input parameters.
///
/// In ULTRA, a `function` represents a non-terminal symbol in a genetic
/// programming "tree". Functions label internal nodes and are evaluated by
/// recursively evaluating their arguments and combining the results.
///
/// Each function has:
/// - a return category (inherited from `symbol`);
/// - a fixed arity;
/// - a category for each input parameter.
///
/// Categories are used to enforce strong typing constraints during program
/// construction, mutation, and crossover.
///
/// \note
/// Functions must satisfy the *closure property*: they must be able to
/// handle any input values that conform to their parameter categories
/// without causing undefined behaviour or runtime errors.
///
class function : public symbol
{
public:
  /// Type used to describe the categories of input parameters.
  ///
  /// The size of this container defines the arity of the function.
  using param_data_types = std::vector<category_t>;

  /// Type used to describe the return category of the function.
  using return_type = category_t;

  class params;

  function(const std::string &, return_type, param_data_types);
  function(const std::string &, std::size_t);

  [[nodiscard]] category_t param_category(std::size_t) const noexcept;
  [[nodiscard]] const param_data_types &categories() const noexcept;

  [[nodiscard]] std::size_t arity() const noexcept;

  /// Evaluates the function for the given parameters.
  ///
  /// \return result of the function evaluation
  ///
  /// \remark
  /// Parameters are accessed via the `params` interface, which supports
  /// lazy evaluation and optional referential transparency. Implementations
  /// should avoid fetching arguments multiple times unless necessary.
  ///
  /// \warning
  /// Implementations must not assume any particular evaluation order of
  /// parameters.
  virtual value_t eval(const params &) const = 0;

  /// Returns a string representation of the function.
  ///
  /// \remark
  /// The base implementation produces a generic functional notation
  /// (e.g. `ADD({0},{1})`). Derived classes may override this method to
  /// support alternative syntaxes or formatting conventions.
  [[nodiscard]] virtual std::string to_string(format = c_format) const;

  [[nodiscard]] bool is_valid() const override;

private:
  const param_data_types params_;
};

///
/// Interface for accessing function arguments during evaluation.
///
/// The `params` interface abstracts argument retrieval and allows function
/// implementations to evaluate inputs lazily. This enables optimisations
/// such as short-circuiting, memoisation or selective evaluation.
///
/// Implementations may distinguish between referentially transparent
/// evaluation and opaque evaluation with side effects.
///
class function::params
{
public:
  /// Retrieves the value of an input parameter assuming referential
  /// transparency.
  ///
  /// \remark
  /// Referential transparency allows caching and other optimisations.
  /// Repeated calls with the same index must return the same value.
  [[nodiscard]] virtual value_t fetch_arg(std::size_t) const = 0;

  /// Retrieves the value of an input parameter without assuming referential
  /// transparency.
  ///
  /// \remark
  /// This method must be used when argument evaluation may produce side
  /// effects or when caching is not safe.
  virtual value_t fetch_opaque_arg(std::size_t) const = 0;

  /// Accesses a parameter using referentially transparent evaluation.
  ///
  /// \remark
  /// This operator is equivalent to calling `fetch_arg(i)`.
  [[nodiscard]] value_t operator[](std::size_t i) const { return fetch_arg(i); }
};

}  // namespace ultra

#endif  // include guard
