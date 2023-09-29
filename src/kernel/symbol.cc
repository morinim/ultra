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
std::string symbol::name() const
{
  return name_;
}

///
/// Changes the category of a symbol.
///
/// \param[in] c the new category
///
/// \remark Should be called only for symbols with undefined category.
///
inline void symbol::category(category_t c)
{
  Expects(category_ == undefined_category);
  Expects(c != category_);

  category_ = c;
}

///
/// \return `true` if the object passes the internal consistency check
///
bool symbol::is_valid() const
{
  return !name().empty();
}

}  // namespace ultra
