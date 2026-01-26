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

#include "kernel/symbol_set.h"
#include "kernel/random.h"
#include "utility/log.h"

namespace ultra
{

namespace internal
{

// ****************************** w_symbol ******************************
w_symbol::w_symbol(const symbol *s, weight_t w) : sym(s), weight(w)
{
  Expects(s);
}

bool is_terminal(const w_symbol &ws)
{
  return is<terminal>(ws.sym);
}

bool is_function(const w_symbol &ws)
{
  return is<function>(ws.sym);
}

// ****************************** sum_container  ******************************
sum_container::sum_container(std::string n) : name_(std::move(n))
{
  Expects(!name_.empty());
}

w_symbol::weight_t sum_container::sum() const noexcept
{
  return sum_;
}

std::size_t sum_container::size() const noexcept
{
  return elems_.size();
}

// Inserts a weighted symbol in the container.
//
// \param[in] ws a weighted symbol
//
// We manage to sort the symbols in descending order (with respect to the
// weight) so the selection algorithm would run faster.
void sum_container::insert(const w_symbol &ws)
{
  elems_.push_back(ws);
  sum_ += ws.weight;

  std::ranges::sort(*this, std::ranges::greater{}, &w_symbol::weight);
}

template<class F>
void sum_container::scale_weights(double ratio, F f)
{
  for (auto &s : elems_)
    if (f(s))
    {
      sum_ -= s.weight;
      s.weight = static_cast<w_symbol::weight_t>(s.weight * ratio);
      sum_ += s.weight;
    }
}

// Extracts a random symbol from the collection.
//
// \return a random symbol
//
// \see
// `test/speed_symbol_set.cc` compares various weighted random selection
// algorithms.
const symbol *sum_container::roulette() const
{
  Expects(sum());

  const auto slot(random::sup(sum()));

  std::size_t i(0);
  for (auto wedge(elems_[i].weight);
       wedge <= slot;
       wedge += elems_[++i].weight)
  {}

  assert(i < size());
  return elems_[i].sym;
}

/// \return `true` if the object passes the internal consistency check
bool sum_container::is_valid() const
{
  const auto check_sum(std::accumulate(elems_.begin(), elems_.end(),
                                       w_symbol::weight_t(0),
                                       [](auto sum, const auto &e)
                                       {
                                         return sum + e.weight;
                                       }));

  if (check_sum != sum())
  {
    ultraERROR << name_ << ": incorrect cached sum of weights (stored: "
               << sum() << ", correct: " << check_sum << ')';
    return false;
  }

  return true;
}

// ****************************** collection  ******************************
/// New empty collection.
//
/// \param[in] n name of the collection
collection::collection(std::string n) : name_(std::move(n))
{
}

void collection::insert(const w_symbol &ws)
{
  all.insert(ws);

  if (is_terminal(ws))
    terminals.insert(ws);
  else  // function
    functions.insert(ws);
}

///
/// \return `true` if the object passes the internal consistency check
///
bool collection::is_valid() const
{
  if (!all.is_valid() || !functions.is_valid() || !terminals.is_valid())
  {
    ultraERROR << "(inside " << name_ << ")";
    return false;
  }

  if (std::ranges::any_of(functions, internal::is_function))
    return false;

  if (std::ranges::any_of(terminals, internal::is_terminal))
    return false;

  for (const auto &s : all)
  {
    if (is_terminal(s))
    {
      if (std::ranges::find(terminals, s) == terminals.end())
      {
        ultraERROR << name_ << ": terminal " << s.sym->name()
                   << " badly stored";
        return false;
      }
    }
    else  // function
    {
      if (std::ranges::find(functions, s) == functions.end())
      {
        ultraERROR << name_ << ": function " << s.sym->name()
                   << " badly stored";
        return false;
      }
    }
  }

  const auto ssize(all.size());

  if (ssize < functions.size())
  {
    ultraERROR << name_ << ": wrong function set size (more than symbol set)";
    return false;
  }

  if (ssize < terminals.size())
  {
    ultraERROR << name_ << ": wrong terminal set size (more than symbol set)";
    return false;
  }

  // The condition:
  //
  //     if (ssize && !terminals.size())
  //     {
  //       ultraERROR << name_ << ": no terminal in the symbol set";
  //       return false;
  //     }
  //
  // must be satisfied when symbol_set is completely populated.
  // Since we don't want to enforce a particular insertion order (i.e.
  // terminals before functions), we cannot perform the check here.

  return ssize == functions.size() + terminals.size();
}

}  // namespace internal

///
/// Clears the current symbol set.
///
void symbol_set::clear()
{
  *this = {};
}

///
/// Adds a new symbol to the set.
///
/// \param[in] new_sym symbol to be added
/// \param[in] w       the weight of `new_sym` (`default_weight` means
///                    standard frequency, `2 * default_weight` doubles
///                    the selection probability)
/// \return            a raw pointer to the symbol just added (or `nullptr` in
///                    case of error)
///
/// A symbol with undefined category will be changed to the first free
/// category.
///
symbol *symbol_set::insert(std::unique_ptr<symbol> new_sym, weight_t w)
{
  Expects(new_sym);

  symbols_.push_back(std::move(new_sym));

  auto *s(symbols_.back().get());

  auto category(s->category());
  if (category == symbol::undefined_category)
  {
    category = views_.size();
    s->category(category);
  }

  // Add possibly missing collections.
  for (symbol::category_t i(views_.size()); i <= category; ++i)
    views_.emplace_back("Collection " + std::to_string(i));
  assert(category < views_.size());

  const internal::w_symbol ws(s, w);
  views_[category].insert(ws);

  return s;
}

///
/// \return number of categories in the symbol set (`>= 1`)
///
symbol::category_t symbol_set::categories() const noexcept
{
  return static_cast<symbol::category_t>(views_.size());
}

///
/// \param[in] c a category
/// \return      number of functions in category `c`
///
std::size_t symbol_set::functions(symbol::category_t c) const noexcept
{
  return c < categories() ? views_[c].functions.size() : 0;
}

///
/// \param[in] c a category
/// \return      number of terminals in category `c`
///
std::size_t symbol_set::terminals(symbol::category_t c) const noexcept
{
  return c < categories() ? views_[c].terminals.size() : 0;
}

///
/// Calculates the set of categories that need a terminal before the correct
/// representation of a SLP `gp::individual` would be possible.
///
/// \return the set of categories
///
/// Consider that:
/// - random generation of individuals may put whatever available function at
///   index `0`;
/// - input values of a function at index `0` can only be terminals.
///
/// So we want, at least, one terminal for every category used by a function.
///
std::set<symbol::category_t> symbol_set::categories_missing_terminal() const
{
  if (views_.empty())
    return {};

  std::set<symbol::category_t> need;

  for (const auto &s : symbols_)
    if (const auto *f = get_if<function>(s.get()))
      need.insert(f->categories().begin(), f->categories().end());

  std::set<symbol::category_t> missing;

  for (const auto &i : need)
    if (i >= categories() || !terminals(i))
      missing.insert(i);

  return missing;
}

///
/// \return `true` if there are enough terminals for secure individual
///         generation
///
bool symbol_set::enough_terminals() const noexcept
{
  return categories_missing_terminal().empty();
}

///
/// Extracts the first terminal of a given category.
///
/// \param[in] c a category
/// \return      first terminal of category `c`
///
const terminal *symbol_set::front_terminal(symbol::category_t c) const
{
  Expects(c < categories());

  return static_cast<const terminal *>(views_[c].terminals.begin()->sym);
}

///
/// Extracts a random symbol from the symbol set without bias between terminals
/// and functions.
///
/// \param[in] c a category
/// \return      a random symbol of category `c`
///
/// \attention
/// - \f$P(terminal) = P(function) = 1/2\f$
/// - \f$P(terminal_i|terminal) = \frac{w_i}{\sum_{t \in terminals} w_t}\f$
/// - \f$P(function_i|function) = \frac{w_i}{\sum_{f \in functions} w_f}\f$
///
/// \note
/// If all symbols have the same probability to appear into a chromosome, there
/// could be some problems.
/// For instance, if our problem has many variables (let's say 100) and the
/// function set has only 4 symbols we cannot get too complex trees because the
/// functions have a reduced chance to appear in the chromosome.
///
const symbol *symbol_set::roulette(symbol::category_t c) const
{
  Expects(c < categories());
  Expects(terminals(c) > 0);

  if (functions(c) > 0 && random::boolean())
    return views_[c].functions.roulette();

  return views_[c].terminals.roulette();
}

///
/// \param[in] c a category
/// \return      a random function of category `c`
///
const function *symbol_set::roulette_function(symbol::category_t c) const
{
  Expects(c < categories());
  Expects(functions(c) > 0);

  return static_cast<const function *>(views_[c].functions.roulette());
}

///
/// Extracts a **literal** terminal value.
///
/// \param[in] c a category
/// \return      a random value of category `c`
///
value_t symbol_set::roulette_terminal(symbol::category_t c) const
{
  Expects(c < categories());
  Expects(terminals(c) > 0);

  return static_cast<const terminal *>(views_[c].terminals.roulette())
         ->instance();
}

///
/// Extends roulette_terminal allowing `param_address` values.
///
/// \param[in] sup  superior bound for the parameter address value
/// \param[in] c    a category
/// \param[in] pa_w weight used for `param_addr` type
/// \return         a random value among those allowed for category `c`
///
value_t symbol_set::roulette_terminal(std::size_t sup,
                                      symbol::category_t c,
                                      weight_t pa_w) const
{
  Expects(pa_w > 0);
  Expects(c < categories());
  Expects(terminals(c) > 0);

  if (sup && functions(c))
  {
    const auto sum(views_[c].terminals.sum() + pa_w);

    if (const auto r(random::sup(sum)); r < pa_w)
      return param_address(r % sup);
  }

  return roulette_terminal(c);
}

///
/// Extracts a random symbol from the symbol set.
///
/// \param[in] c a category
/// \return      a random symbol of category `c`
///
/// \attention
/// Given \f$S_t = \sum_{i \in terminals} {w_i}\f$ and
/// \f$S_f = \sum_{i \in functions} {w_i}\f$ we have:
/// - \f$P(terminal_i|terminal) = \frac {w_i} {S_t}\f$
/// - \f$P(function_i|function) = \frac {w_i} {S_f}\f$
/// - \f$P(terminal) = \frac {S_t} {S_t + S_f}\f$
/// - \f$P(function) = \frac {S_f} {S_t + S_f}\f$
///
const symbol *symbol_set::roulette_free(symbol::category_t c) const
{
  Expects(c < categories());
  return views_[c].all.roulette();
}

///
/// \param[in] s a symbol
/// \return      the weight of `s`
///
symbol_set::weight_t symbol_set::weight(const symbol &s) const
{
  for (const auto &ws : views_[s.category()].all)
    if (ws.sym == &s)
      return ws.weight;

  return 0;
}

///
/// \param[in] opcode numerical code used as primary key for a symbol
/// \return           a pointer to the ultra::symbol identified by `opcode`
///                   (`nullptr` if not found).
///
const symbol *symbol_set::decode(symbol::opcode_t opcode) const
{
  for (const auto &s : symbols_)
    if (s->opcode() == opcode)
      return s.get();

  return nullptr;
}

///
/// \param[in] dex the name of a symbol
/// \return        a pointer to the symbol identified by `dex` (0 if not found)
///
/// \attention
/// Please note that opcodes are automatically generated and fully identify
/// a symbol (they're primary keys). Conversely the name of a symbol is chosen
/// by the user, so, if you don't pay attention, different symbols may have the
/// same name.
///
const symbol *symbol_set::decode(const std::string &dex) const
{
  Expects(!dex.empty());

  for (const auto &s : symbols_)
    if (s->name() == dex)
      return s.get();

  return nullptr;
}

///
/// \return `true` if the object passes the internal consistency check
///
bool symbol_set::is_valid() const
{
  if (!enough_terminals())
  {
    ultraERROR << "Symbol set doesn't contain enough symbols";
    return false;
  }

  return true;
}

///
/// Prints the symbol set to an output stream.
///
/// \param[out] o  output stream
/// \param[in]  ss symbol set to be printed
/// \return        output stream including `ss`
///
/// \note
/// Useful for debugging purpose.
///
/// \related symbol_set
///
std::ostream &operator<<(std::ostream &o, const symbol_set &ss)
{
  for (const auto &s : ss.symbols_)
  {
    o << s->name();

    if (const auto *f = get_if<function>(s.get()))
    {
      o << '(' << f->param_category(0);
      for (std::size_t j(1); j < f->arity(); ++j)
        o << ", " << f->param_category(j);
      o << ')';
    }

    o << " -> " << s->category() << " (opcode " << s->opcode()
      << ", weight "
      << ss.weight(*s) << ")\n";
  }

  return o;
}

}  // namespace ultra
