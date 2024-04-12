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

#if !defined(ULTRA_SRC_INTERPRETER_H)
#define      ULTRA_SRC_INTERPRETER_H

#include "kernel/gp/interpreter.h"

namespace ultra::src
{
///
/// This class extends ultra::interpreter to manage input variables.
///
/// For further details see `ultra::src::variable` class.
///
class interpreter : public ultra::interpreter
{
public:
  explicit interpreter(const gp::individual &);

  value_t run(const std::vector<value_t> &);

  [[nodiscard]] value_t fetch_var(std::size_t) const;

private:
  // Tells the compiler we want both the run function from `ultra::interpreter`
  // and `ultra::src::interpreter`.
  // Without this statement there will be no `run()` function in the scope
  // of `src::interpreter`, because it would be hidden by another method with
  // the same name (compiler won't search for function in base classes if
  // derived class has at least one method with specified name, even if it has
  // different arguments).
  using ultra::interpreter::run;

  const std::vector<value_t> *example_ {nullptr};
};

[[nodiscard]] value_t run(const gp::individual &,
                          const std::vector<value_t> &);

}  // namespace ultra

#endif  // include guard
