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

#include <algorithm>
#include <set>

#include "kernel/symbol_set.h"
#include "kernel/random.h"
#include "utility/log.h"

namespace ultra
{

namespace detail
{

// ****************************** w_symbol ******************************
w_symbol::w_symbol(const symbol *s, weight_t w) : sym(s), weight(w)
{
  Expects(s);
}

bool w_symbol::operator==(const w_symbol &rhs) const
{
  return sym == rhs.sym && weight == rhs.weight;
}

bool is_terminal(const w_symbol &ws)
{
  return is_terminal(ws.sym);
}

bool is_function(const w_symbol &ws)
{
  return is_function(ws.sym);
}

// ****************************** sum_container  ******************************
sum_container::sum_container(std::string n) : name_(std::move(n))
{
  Expects(!name_.empty());
}

auto sum_container::begin()
{
  return elems_.begin();
}

auto sum_container::begin() const
{
  return elems_.begin();
}

auto sum_container::end()
{
  return elems_.end();
}

auto sum_container::end() const
{
  return elems_.end();
}

w_symbol::weight_t sum_container::sum() const
{
  return sum_;
}

std::size_t sum_container::size() const
{
  return elems_.size();
}

const w_symbol &sum_container::operator[](std::size_t i) const
{
  return elems_[i];
}

// Inserts a weighted symbol in the container.
//
// \param[in] ws a weighted symbol
//
// We manage to sort the symbols in descending order, with respect to the
// weight, so the selection algorithm would run faster.
void sum_container::insert(const w_symbol &ws)
{
  elems_.push_back(ws);
  sum_ += ws.weight;

  std::sort(begin(), end(),
            [](auto s1, auto s2) { return s1.weight > s2.weight; });
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
//
// \see
// `test/speed_symbol_set.cc` compares various weighted random selection
// algorithms.
const symbol &sum_container::roulette() const
{
  Expects(sum());

  const auto slot(random::sup(sum()));

  std::size_t i(0);
  for (auto wedge(elems_[i].weight);
       wedge <= slot;
       wedge += elems_[++i].weight)
  {}

  assert(i < elems_.size());
  return *elems_[i].sym;
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

  if (is_terminal(ws.sym))
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

  if (std::ranges::any_of(functions, detail::is_function))
    return false;

  if (std::ranges::any_of(terminals, detail::is_terminal))
    return false;

  for (const auto &s : all)
  {
    if (is_terminal(s.sym))
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

}  // namespace detail

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
/// \param[in] wr      the weight of `s` (`1.0` means standard frequency, `2.0`
///                    double probability of selection)
/// \return            a raw pointer to the symbol just added (or `nullptr` in
///                    case of error)
///
/// A symbol with undefined category will be changed to the first free
/// category.
///
symbol *symbol_set::insert(std::unique_ptr<symbol> new_sym, double wr)
{
  Expects(new_sym);
  Expects(wr >= 0.0);

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

  const auto w(static_cast<weight_t>(wr * detail::w_symbol::base_weight));
  const detail::w_symbol ws(s, w);
  views_[category].insert(ws);

  return s;
}

///
/// \return number of categories in the symbol set (`>= 1`)
///
symbol::category_t symbol_set::categories() const
{
  return static_cast<symbol::category_t>(views_.size());
}

///
/// \param[in] c a category
/// \return      number of terminals in category `c`
///
std::size_t symbol_set::terminals(symbol::category_t c) const
{
  return c < categories() ? views_[c].terminals.size() : 0;
}

///
/// We want at least one terminal for every used category.
///
/// \return `true` if there are enough terminals for secure individual
///         generation
///
bool symbol_set::enough_terminals() const
{
  if (views_.empty())
    return true;

  std::set<symbol::category_t> need;

  for (const auto &s : symbols_)
    if (const auto *f(dynamic_cast<const function *>(s.get())); f)
    {
      const auto arity(f->arity());
      for (std::size_t i(0); i < arity; ++i)
        need.insert(f->arg_category(i));
    }

  for (const auto &i : need)
    if (i >= categories() || !views_[i].terminals.size())
      return false;

  return true;
}

///
/// Extracts a random symbol from the symbol set without bias between terminals
/// and functions .
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
const symbol &symbol_set::roulette(symbol::category_t c) const
{
  Expects(c < categories());
  Expects(views_[c].terminals.size());

  if (random::boolean() && views_[c].functions.size())
    return views_[c].functions.roulette();

  return views_[c].terminals.roulette();
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

/*
///
/// \param[in] c a category
/// \return      a random function of category `c`
///
const function &symbol_set::roulette_function(category_t c) const
{
  Expects(c < categories());
  Expects(views_[c].functions.size());

  return static_cast<const function &>(views_[c].functions.roulette());
}

///
/// \param[in] c a category
/// \return      a random terminal of category `c`
///
const terminal &symbol_set::roulette_terminal(category_t c) const
{
  Expects(c < categories());
  Expects(views_[c].terminals.size());

  return static_cast<const terminal &>(views_[c].terminals.roulette());
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
const symbol &symbol_set::roulette_free(category_t c) const
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
/// Prints the symbol set to an output stream.
///
/// \param[out] o output stream
/// \param[in] ss symbol set to be printed
/// \return       output stream including `ss`
///
/// \note Useful for debugging purpose.
///
std::ostream &operator<<(std::ostream &o, const symbol_set &ss)
{
  for (const auto &s : ss.symbols_)
  {
    o << s->name();

    auto arity(s->arity());
    if (arity)
    {
      o << '(';
      for (decltype(arity) j(0); j < arity; ++j)
        o << function::cast(s.get())->arg_category(j)
          << (j + 1 == arity ? "" : ", ");
      o << ')';
    }

    o << " -> " << s->category() << " (opcode " << s->opcode()
      << ", parametric "
      << (s->terminal() && terminal::cast(s.get())->parametric())
      << ", weight "
      << ss.weight(*s) << ")\n";
  }

  return o;
}

*/
}  // namespace ultra
