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

#if !defined(ULTRA_INTERPRETER_H)
#define      ULTRA_INTERPRETER_H

#include "kernel/gp/individual.h"

namespace ultra
{
///
/// Executes a GP individual by interpreting its genome.
///
/// The interpreter provides the execution context required by GP functions
/// (`function::eval`) and implements lazy, memoised argument evaluation
/// through the `function::params` interface.
///
/// Evaluation proceeds by recursively visiting genes starting from a given
/// locus (instruction pointer). When referential transparency is assumed,
/// intermediate results are cached to avoid redundant evaluations.
///
/// ### Execution model
/// - Each gene represents a function invocation.
/// - Arguments may be immediate values, addresses of other genes
///   (sub-expressions), nullary symbols, variables.
/// - Address-based arguments are evaluated by temporarily moving the
///   instruction pointer to the referenced locus.
///
/// ### Caching
/// When arguments are fetched via `fetch_arg`, the interpreter memoises the
/// result associated with the referenced locus. This optimisation assumes
/// referential transparency of the evaluated expressions.
///
/// Side effects must therefore be accessed through `fetch_opaque_arg`, which
/// bypasses the cache.
///
/// ### Lifetime
/// The interpreter does not own the associated `gp::individual`.
/// The lifetime of the individual must exceed that of the interpreter.
///
/// ### Thread safety
/// An interpreter instance is not thread-safe. It maintains mutable execution
/// state (instruction pointer and evaluation cache) and must not be accessed
/// concurrently from multiple threads.
/// Parallel execution must be achieved by using one interpreter instance per
/// thread.
///
/// \see function::params
///
class interpreter : public function::params
{
public:
  explicit interpreter(const gp::individual &);

  value_t run(const locus &);
  value_t run();

  [[nodiscard]] value_t fetch_arg(std::size_t) const final;
  [[nodiscard]] value_t fetch_opaque_arg(std::size_t) const final;

  [[nodiscard]] bool is_valid() const;

  [[nodiscard]] const gp::individual &program() const noexcept;

private:
  [[nodiscard]] const gene &current_gene() const;

  const gp::individual *prg_;

  struct elem_ {value_t value; bool valid;};
  mutable matrix<elem_> cache_;

  mutable locus ip_ {};  // instruction pointer
};

extern value_t run(const gp::individual &);

}  // namespace ultra

#endif  // include guard
