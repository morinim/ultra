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

#include "kernel/gp/individual.h"

#include "utility/timer.h"

#include "test/fixture1.h"

#include <algorithm>
#include <queue>
#include <flat_set>

struct common_elems
{
  using iterator_category = std::forward_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = ultra::gene;
  using pointer = value_type *;
  using const_pointer = const value_type *;
  using reference = value_type &;
  using const_reference = const value_type &;

  using ptr = const_pointer;
  using ref = const_reference;
  using ind = const ultra::gp::individual;
};

class iterator1 : public common_elems
{
public:
  iterator1() = default;

  explicit iterator1(ind &id) : loci_({id.start()}), ind_(&id) {}

  iterator1 &operator++()
  {
    if (!loci_.empty())
    {
      const auto &g(operator*());

      const auto is_addr([](const auto &a)
                         { return a.index() == ultra::d_address; });

      if (const auto addr(std::ranges::find_if(g.args, is_addr));
          addr == g.args.end())
      {
        loci_.erase(loci_.begin());
      }
      else
      {
        auto node(loci_.extract(loci_.begin()));
        node.value() = g.locus_of_argument(*addr);
        loci_.insert(std::move(node));

        for (auto it(std::next(addr)); it != g.args.end(); ++it)
          if (is_addr(*it))
            loci_.insert(g.locus_of_argument(*it));
      }
    }

    return *this;
  }

  [[nodiscard]] bool operator==(const iterator1 &rhs) const noexcept
  {
    return (loci_.empty() && rhs.loci_.empty())
           || loci_.cbegin() == rhs.loci_.cbegin();
  }

  [[nodiscard]] ref operator*() const { return ind_->operator[](locus()); }
  [[nodiscard]] ptr operator->() const { return &operator*(); }
  [[nodiscard]] ultra::locus locus() const { return *loci_.cbegin(); }

private:
  std::set<ultra::locus, std::greater<ultra::locus>> loci_ {};
  ind *ind_ {};
};

class iterator2 : public common_elems
{
public:
  iterator2() = default;

  explicit iterator2(ind &id)
    : loci_(id.size(), id.categories()), current_(id.start()), ind_(&id)
  {
    loci_.fill(false);
    loci_(current_) = true;
  }

  iterator2 &operator++()
  {
    if (current_ != ultra::locus::npos())
    {
      const auto next_current([&]()
      {
        if (current_.category == 0)
          if (current_.index > 0)
          {
            --current_.index;
            current_.category = loci_.cols() - 1;
          }
          else
            current_ = ultra::locus::npos();
        else
          --current_.category;

        return current_;
      });

      while (loci_(current_) == false && next_current() != ultra::locus::npos())
        ;

      if (current_ != ultra::locus::npos())
      {
        const auto &g(operator*());
        for (const auto &a : g.args)
          if (a.index() == ultra::d_address)
            loci_(g.locus_of_argument(a)) = true;

        loci_(current_) = false;
        while (loci_(current_) == false
               && next_current() != ultra::locus::npos())
          ;
      }
    }

    return *this;
  }

  [[nodiscard]] bool operator==(const iterator2 &rhs) const noexcept
  {
    return current_ == rhs.current_;
  }

  [[nodiscard]] ref operator*() const { return ind_->operator[](locus()); }
  [[nodiscard]] ptr operator->() const { return &operator*(); }
  [[nodiscard]] ultra::locus locus() const { return current_; }

private:
  ultra::matrix<char> loci_ {};
  ultra::locus current_ {ultra::locus::npos()};
  ind *ind_ {nullptr};
};

class iterator3 : public common_elems
{
public:
  iterator3() = default;

  explicit iterator3(ind &id) : ind_(&id)
  {
    loci_.push({id.start()});
  }

  iterator3 &operator++()
  {
    if (!loci_.empty())
    {
      const auto &g(operator*());
      const auto first(loci_.top());

      // This traversal relies on the invariant that address arguments only
      // reference strictly larger loci.
      // Under this guarantee, all duplicates in the priority queue are
      // contiguous and can be removed locally.
      do
        loci_.pop();
      while (!loci_.empty() && loci_.top() == first);

      for (const auto &a : g.args)
        if (a.index() == ultra::d_address)
          loci_.push(g.locus_of_argument(a));
    }

    return *this;
  }

  [[nodiscard]] bool operator==(const iterator3 &rhs) const noexcept
  {
    return loci_.empty() == rhs.loci_.empty()
           && (loci_.empty() || locus() == rhs.locus());
  }

  [[nodiscard]] ref operator*() const { return ind_->operator[](locus()); }
  [[nodiscard]] ptr operator->() const { return &operator*(); }
  [[nodiscard]] ultra::locus locus() const { return loci_.top(); }

private:
  // Addresses only point to strictly larger loci, therefore all duplicates
  // in the heap are contiguous and can be removed locally.
  std::priority_queue<ultra::locus> loci_ {};
  ind *ind_ {};
};

class iterator4 : public common_elems
{
public:
  iterator4() = default;

  explicit iterator4(ind &id) : loci_({id.start()}), ind_(&id) {}

  iterator4 &operator++()
  {
    if (!loci_.empty())
    {
      const auto &g(operator*());

      loci_.erase(loci_.begin());

      for (const auto &a : g.args)
        if (a.index() == ultra::d_address)
          loci_.insert(g.locus_of_argument(a));
    }

    return *this;
  }

  [[nodiscard]] bool operator==(const iterator4 &rhs) const noexcept
  {
    return (loci_.empty() && rhs.loci_.empty())
           || loci_.cbegin() == rhs.loci_.cbegin();
  }

  [[nodiscard]] ref operator*() const { return ind_->operator[](locus()); }
  [[nodiscard]] ptr operator->() const { return &operator*(); }
  [[nodiscard]] ultra::locus locus() const { return *loci_.cbegin(); }

private:
  std::flat_set<ultra::locus, std::greater<ultra::locus>> loci_ {};
  ind *ind_ {};
};


template<class IT>
inline unsigned test_alternative(
  const std::string_view name, unsigned sup,
  const std::vector<ultra::gp::individual> &ind_db)
{
  unsigned count(0);

  ultra::timer t;
  for (unsigned j(0); j < 10*sup; ++j)
    for (unsigned i(0); i < sup; ++i)
    {
      const auto &ind(ind_db[i]);

      IT it(ind);
      for (; it != IT(); ++it)
        ++count;
    }

  const auto elapsed(t.elapsed().count());

  std::cout << std::setfill(' ') << std::setw(20) << std::left << name
            << " - Elapsed: " << elapsed << "ms\n";
  return count;
}

// Performance characteristics depend on the STL implementation:
// `std::set` performs slightly better on libc++, while `std::priority_queue`
// performs slightly better on libstdc++.
int main()
{
  using namespace ultra;

  fixture1 f;
  constexpr unsigned sup(1000);

  // Variable length random creation.
  std::vector<ultra::gp::individual> ind_db;
  for (unsigned i(0); i < sup; ++i)
  {
    f.prob.params.slp.code_length = random::between(1, 2000);
    ind_db.push_back(gp::individual(f.prob));
  }

  volatile std::size_t out(0);
  {
    unsigned count(0);

    ultra::timer t;
    for (unsigned j(0); j < 10*sup; ++j)
      for (unsigned i(0); i < sup; ++i)
      {
        const auto &ind(ind_db[i]);
        const auto exons(ind.cexons());

        for (auto it(exons.begin()); it != exons.end(); ++it)
          ++count;
      }
    const auto elapsed(t.elapsed().count());

    std::cout << "Default iterator     - Elapsed: " << elapsed << "ms\n";
    out = count;
  }

  // -------------------------------------------------------------------------

  volatile std::size_t out1(0);
  out1 = test_alternative<iterator1>("Set extract iterator", sup, ind_db);

  // -------------------------------------------------------------------------

  volatile std::size_t out2(0);
  out2 = test_alternative<iterator2>("Matrix<bool>", sup, ind_db);

  // -------------------------------------------------------------------------

  volatile std::size_t out3(0);
  out3 = test_alternative<iterator3>("Priority queue", sup, ind_db);

  // -------------------------------------------------------------------------

  volatile std::size_t out4(0);
  out4 = test_alternative<iterator3>("Flat set", sup, ind_db);

  const bool ok(out == out1 && out1 == out2 && out2 == out3 && out3 == out4);

  if (!ok)
    std::cout << "PROBLEM. Out: " << out << "  Out1: " << out1 << "  Out2: "
              << out2 << "  Out3: " << out3 << "  Out4: " << out4 << std::endl;
}
