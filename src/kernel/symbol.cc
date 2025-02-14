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

namespace ultra
{
///
/// \param[in] name name of the symbol
/// \param[in] c    category of the symbol
///
/// \warning
/// Since the name of the symbol is used for serialization, it must be
/// unique. Even the opcode is unique, but it can change between executions.
///
symbol::symbol(const std::string &name, category_t c)
  : name_(name), category_(c)
{
  static opcode_t opc_count_(0);

  opcode_ = opc_count_++;

  Ensures(is_valid());
}

///
/// \return the name of the symbol
///
std::string symbol::name() const noexcept
{
  return name_;
}

///
/// Changes the category of a symbol.
///
/// \param[in] c the new category
///
/// \remark
/// Should be called only for symbols with undefined category.
///
void symbol::category(category_t c) noexcept
{
  Expects(category_ == undefined_category);
  Expects(c != category_);

  category_ = c;
}

///
/// \return the type (a.k.a. category) of the symbol
///
/// In strongly typed GP every terminal and every function argument / return
/// value has a type (a.k.a. category).
/// For GAs / DE category is used to define a valid interval for numeric
/// arguments.
///
symbol::category_t symbol::category() const noexcept
{
  return category_;
}

///
/// An opcode is a unique, numerical session ID for a symbol.
///
/// \return the opcode
///
/// The opcode is a fast way to uniquely identify a symbol and is primarily
/// used for hashing.
///
/// \remark
/// A symbol can be identified also by its name (a `std::string`). The name
/// is often a better option since it doesn't change among executions.
///
symbol::opcode_t symbol::opcode() const noexcept
{
  return opcode_;
}

///
/// \return `true` if the object passes the internal consistency check
///
bool symbol::is_valid() const
{
  return !name().empty();
}

}  // namespace ultra
