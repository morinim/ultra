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

#include "kernel/symbol.h"

#include <atomic>

namespace ultra
{
///
/// Constructs a symbol with the given name and category.
///
/// \param[in] name name of the symbol (unique textual identifier)
/// \param[in] c    category of the symbol
///
/// \warning
/// The symbol name is used for serialisation and must be globally unique
/// within a symbol set. While opcodes are also unique, they are assigned
/// dynamically and may differ between executions.
///
symbol::symbol(const std::string &name, category_t c)
  : name_(name), category_(c)
{
  // Atomic is necessary because multiple `ultra::problem` instances
  // may be created concurrently, each adding their own `symbol`s in parallel.
  static std::atomic<opcode_t> opc_count_(0);

  opcode_ = opc_count_++;

  Ensures(is_valid());
}

///
/// Returns the name of the symbol.
///
/// \return constant reference to the symbol name
///
/// \note
/// The name uniquely identifies the symbol across executions and is used
/// during serialisation.
///
const std::string &symbol::name() const noexcept
{
  return name_;
}

///
/// Assigns a category to the symbol.
///
/// \param[in] c new category value
///
/// \pre The current category must be `undefined_category`
///
/// \remark
/// This function exists to support deferred category assignment. It should not
/// be used to change an already defined category.
///
void symbol::category(category_t c) noexcept
{
  Expects(category_ == undefined_category);

  category_ = c;
}

///
/// Returns the category of the symbol.
////
/// \return category associated with the symbol
///
symbol::category_t symbol::category() const noexcept
{
  return category_;
}

///
/// Returns the opcode of the symbol.
///
/// \return a session-local, unique numerical identifier
///
/// \note
/// The opcode is intended for fast internal use (e.g. hashing). It is not
/// stable across executions and must not be used for persistence.
///
symbol::opcode_t symbol::opcode() const noexcept
{
  return opcode_;
}

///
/// Performs an internal consistency check.
///
/// \return `true` if the symbol satisfies basic validity constraints
///
/// \remark
/// Derived classes are expected to extend this check to enforce additional
/// invariants.
///
bool symbol::is_valid() const
{
  return !name().empty();
}

}  // namespace ultra
