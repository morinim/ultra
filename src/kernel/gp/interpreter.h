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
/// Executes a GP individual (a program).
///
/// The program can produce an output or perform some actions.
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

  [[nodiscard]] const gp::individual &program() const;

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
