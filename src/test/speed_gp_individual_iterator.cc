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
#include <cstdlib>

#include "kernel/gp/individual.h"

#include "utility/timer.h"

#include "test/fixture1.h"

class iterator1
{
public:
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

  iterator1() : loci_(), ind_(nullptr) {}

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

  [[nodiscard]] bool operator==(const iterator1 &rhs) const
  {
    Ensures(!ind_ || !rhs.ind_ || ind_ == rhs.ind_);

    return (loci_.empty() && rhs.loci_.empty())
           || loci_.cbegin() == rhs.loci_.cbegin();
  }

  [[nodiscard]] ref operator*() const { return ind_->operator[](locus()); }
  [[nodiscard]] ptr operator->() const { return &operator*(); }
  [[nodiscard]] ultra::locus locus() const { return *loci_.cbegin(); }

private:
  std::set<ultra::locus, std::greater<ultra::locus>> loci_;
  ind *ind_;
};

class iterator2
{
public:
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

  iterator2() : loci_(), current_(ultra::locus::npos()), ind_(nullptr) {}

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
        while (loci_(current_) == false && next_current() != ultra::locus::npos())
          ;
      }
    }

    return *this;
  }

  [[nodiscard]] bool operator==(const iterator2 &rhs) const
  {
    Ensures(!ind_ || !rhs.ind_ || ind_ == rhs.ind_);

    return current_ == rhs.current_;
  }

  [[nodiscard]] ref operator*() const { return ind_->operator[](locus()); }
  [[nodiscard]] ptr operator->() const { return &operator*(); }
  [[nodiscard]] ultra::locus locus() const { return current_; }

private:
  ultra::matrix<char> loci_;
  ultra::locus current_;
  ind *ind_;
};

int main()
{
  using namespace ultra;

  fixture1 f;
  constexpr unsigned sup(1000);

  // Variable length random creation.
  std::vector<ultra::gp::individual> ind_db;
  for (unsigned i(0); i < sup; ++i)
  {
    f.prob.env.slp.code_length = random::between(1, 2000);
    ind_db.push_back(gp::individual(f.prob));
  }

  volatile std::size_t out(0);
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

  std::cout << "Default iterator     - Elapsed: " << t.elapsed().count()
            << "ms\n";
  out = count;

  // -------------------------------------------------------------------------

  volatile std::size_t out1(0);
  unsigned count1(0);

  t.restart();
  for (unsigned j(0); j < 10*sup; ++j)
    for (unsigned i(0); i < sup; ++i)
    {
      const auto &ind(ind_db[i]);

      iterator1 it1(ind);
      for (; it1 != iterator1(); ++it1)
        ++count1;
    }

  std::cout << "Set extract iterator - Elapsed: " << t.elapsed().count()
            << "ms\n";
  out1 = count1;

  // -------------------------------------------------------------------------

  volatile std::size_t out2(0);
  unsigned count2(0);

  t.restart();
  for (unsigned j(0); j < 10*sup; ++j)
    for (unsigned i(0); i < sup; ++i)
    {
      const auto &ind(ind_db[i]);

      iterator2 it2(ind);
      for (; it2 != iterator2(); ++it2)
        ++count2;
    }

  std::cout << "Bool matrix          - Elapsed: " << t.elapsed().count()
            << "ms\n";
  out2 = count2;

  const bool ok(out == out1 && out1 == out2);

  if (!ok)
    std::cout << "PROBLEM. Out: " << out << "  Out1: " << out1 << "  Out2: "
              << out2 << std::endl;
}
