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

#if !defined(ULTRA_SYMBOL_H)
#define      ULTRA_SYMBOL_H

#include <limits>
#include <string>

#include "utility/assert.h"

namespace ultra
{

///
/// Base class for all symbols.
///
/// In ULTRA, *symbols* are the atomic building blocks from which programs are
/// constructed. Together, functions (internal nodes) and terminals (leaf
/// nodes) form the symbol set used by the evolutionary algorithm to assemble
/// executable program structures.
///
/// A symbol is uniquely identified across runs by its name. Symbols may also
/// belong to a category, which is used to enforce strong typing in GP and to
/// manage value domains in GA / DE contexts.
///
/// \note
/// Symbols are immutable with respect to their identity (name and opcode).
/// The category may be assigned after construction only if initially marked
/// as undefined.
///
class symbol
{
public:
  // ---- Member types ----

  /// Type used to represent symbol categories.
  ///
  /// Categories are used to:
  /// - enforce strong typing in genetic programming;
  /// - define admissible value ranges in genetic algorithms and differential
  ///   evolution.
  ///
  /// A category represents a sub-domain of values. The same numerical value
  /// may belong to different categories (e.g. "kg" vs "km/h").
  using category_t = unsigned;

  /// Default category assigned when typing is not used.
  static constexpr category_t default_category = 0;

  /// Sentinel value indicating that the category has not yet been assigned.
  static constexpr category_t undefined_category =
    std::numeric_limits<category_t>::max();

  /// Type used as a fast, session-local identifier for symbols.
  ///
  /// Opcodes are unique within a single execution and are primarily used for
  /// hashing and fast comparisons. They are *not* stable across executions and
  /// must not be serialised.
  using opcode_t = unsigned;

  /// Supported rendering formats for symbol stringification.
  enum format {c_format, cpp_format, python_format, sup_format};

  // ---- Constructors ----
  explicit symbol(const std::string &, category_t = default_category);

  /// Virtual destructor to allow safe polymorphic deletion.
  virtual ~symbol() = default;

  // ---- Observers ----
  [[nodiscard]] category_t category() const noexcept;
  [[nodiscard]] opcode_t opcode() const noexcept;
  [[nodiscard]] const std::string &name() const noexcept;

  // ---- Modifiers ----
  void category(category_t) noexcept;

  // ---- Misc ----
  [[nodiscard]] virtual bool is_valid() const;

private:
  std::string name_;
  category_t category_;
  opcode_t opcode_;
};

template<class S> concept Symbol = std::derived_from<S, symbol>;

///
/// Attempts to retrieve a pointer to a specific derived symbol type.
///
/// \tparam S expected dynamic type, which must derive from `symbol`
///
/// \param[in] s pointer to the symbol to inspect
/// \return      pointer to `const S` if `s` refers to an object of type `S`,
///              otherwise a null pointer
///
/// \remark
/// This function performs a runtime-checked downcast. It is intended for
/// situations where heterogeneous symbol containers must be inspected
/// safely without relying on external type tags.
///
template<Symbol S>
[[nodiscard]] auto get_if(const symbol *s)
{
  return dynamic_cast<const S *>(s);
}

///
/// Attempts to retrieve a pointer to a specific derived symbol type.
///
/// \tparam S expected dynamic type, which must derive from `symbol`
///
/// \param[in] s reference to the symbol to inspect
/// \return      pointer to `const S` if `s` refers to an object of type `S`,
///              otherwise a null pointer
///
/// \remark
/// This overload is equivalent to calling `get_if<S>(&s)`.
///
template<Symbol S>
[[nodiscard]] auto get_if(const symbol &s)
{
  return get_if<S>(&s);
}

///
/// Checks whether a symbol is of a given derived type.
///
/// \tparam S expected dynamic type, which must derive from `symbol`
///
/// \param[in] s pointer to the symbol to inspect
/// \return      `true` if `s` refers to an object of type `S`, `false`
///              otherwise
///
/// \remark
/// This is a convenience wrapper around `get_if<S>()` intended for
/// readability when only a boolean test is required.
///
template<Symbol S>
[[nodiscard]] bool is(const symbol *s)
{
  return get_if<S>(s);
}

///
/// Checks whether a symbol is of a given derived type.
///
/// \tparam S expected dynamic type, which must derive from `symbol`
///
/// \param[in] s reference to the symbol to inspect
/// \return      `true` if `s` refers to an object of type `S`, `false
///              otherwise.
///
/// \remark
/// This overload is equivalent to calling `is<S>(&s)`.
///
template<Symbol S>
[[nodiscard]] bool is(const symbol &s)
{
  return is<S>(&s);
}

}  // namespace ultra

#endif  // include guard
