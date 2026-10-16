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
#include "kernel/gp/team.h"
#include "kernel/individual_format.h"

#include "test/fixture1.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

#include <cstdlib>
#include <numeric>
#include <sstream>

namespace
{

struct tracking_individual
{
  mutable unsigned sig_calls {0};
  mutable unsigned mutations_to_perform {0};
  ultra::hash_t sig {12345};
  ultra::individual::age_t age_val {0};
  unsigned active_slots_count {1};
  double last_temperature {1.0};

  [[nodiscard]] bool empty() const noexcept { return false; }

  [[nodiscard]] ultra::hash_t signature() const noexcept
  {
    ++sig_calls;
    return sig;
  }

  [[nodiscard]] ultra::individual::age_t age() const noexcept { return age_val; }
  void inc_age(unsigned delta = 1) noexcept { age_val += delta; }
  [[nodiscard]] bool is_valid() const noexcept { return true; }

  unsigned mutation(const ultra::problem &, double temperature = 1.0)
  {
    last_temperature = temperature;
    if (mutations_to_perform > 0)
    {
      sig.combine(ultra::hash_t(mutations_to_perform));
      return mutations_to_perform;
    }

    return 0;
  }

  bool operator==(const tracking_individual &) const = default;

  friend unsigned active_slots(const tracking_individual &ind) noexcept
  {
    return ind.active_slots_count;
  }
};

}  // namespace

TEST_SUITE("TEAM")
{

TEST_CASE("Concept")
{
  using namespace ultra::gp;

  REQUIRE(ultra::Individual<team<individual>>);

  REQUIRE(Team<team<individual>>);
  REQUIRE(!Team<individual>);
}

TEST_CASE_FIXTURE(fixture1, "Empty team")
{
  using namespace ultra::gp;

  const team<individual> t;
  const team<individual> t0(0);
  const team<individual> tv(std::vector<individual>{});

  CHECK(t.empty());
  CHECK(t.size() == 0);
  CHECK(t.is_valid());
  CHECK(t.age() == 0);
  CHECK(t.begin() == t.end());
  CHECK(active_slots(t) == 0);

  CHECK(t0.empty());
  CHECK(tv.empty());
  CHECK(t == t0);
  CHECK(t == tv);
  CHECK(distance(t, t0) == 0);

  SUBCASE("Aging and mutation")
  {
    team<individual> mut_t;
    mut_t.inc_age();
    CHECK(mut_t.age() == 0);
    mut_t.inc_age(5);
    CHECK(mut_t.age() == 0);

    CHECK(mut_t.mutation(prob) == 0);
    CHECK(mut_t.is_valid());
  }

  SUBCASE("Comparison with populated team")
  {
    const team<individual> populated(prob);
    CHECK(t != populated);
  }

  SUBCASE("Crossover")
  {
    const auto crossed(crossover(prob, t, t0));
    CHECK(crossed.empty());
    CHECK(crossed.is_valid());
  }

  SUBCASE("Decision vector")
  {
    team<individual> dv_t;
    const auto dv(extract_decision_vector(dv_t));
    CHECK(dv.empty());
    CHECK(dv.is_valid());

    dv_t.apply_decision_vector(dv);
    CHECK(dv_t.empty());
    CHECK(dv_t.is_valid());
  }

  SUBCASE("Serialization")
  {
    std::stringstream ss;
    CHECK(t.save(ss));

    team<individual> loaded_empty;
    CHECK(loaded_empty.load(ss, prob.sset));
    CHECK(loaded_empty.empty());
    CHECK(loaded_empty.is_valid());
    CHECK(loaded_empty == t);
    CHECK(loaded_empty.age() == 0);

    // Overwriting a pre-populated team with an empty stream
    ss.clear();
    ss.str({});
    CHECK(t.save(ss));

    team<individual> populated(prob);
    REQUIRE(!populated.empty());
    CHECK(populated.load(ss, prob.sset));
    CHECK(populated.empty());
    CHECK(populated.is_valid());
    CHECK(populated == t);
    CHECK(populated.age() == 0);
  }

  SUBCASE("Stream output")
  {
    std::ostringstream ss;
    ss << t;
    CHECK(ss.str().empty());

    std::ostringstream ss_inline;
    ss_inline << ultra::out::in_line << t;
    CHECK(ss_inline.str().empty());
  }
}

TEST_CASE_FIXTURE(fixture1, "Random creation")
{
  using namespace ultra::gp;

  // Variable length random creation
  for (auto l(prob.sset.categories() + 2); l < 100; ++l)
  {
    prob.params.slp.code_length = l;
    team<individual> t(prob);

    CHECK(t.is_valid());
    CHECK(t.age() == 0);
  }
}

TEST_CASE_FIXTURE(fixture1, "Mutation")
{
  using namespace ultra::gp;

  prob.params.slp.code_length = 100;

  team<individual> t(prob);
  const auto orig(t);

  CHECK(t.size() > 0);

  const unsigned n(4000);

  SUBCASE("Zero probability mutation")
  {
    prob.params.evolution.p_mutation = 0.0;
    for (unsigned i(0); i < n; ++i)
    {
      t.mutation(prob);
      CHECK(t == orig);
    }
  }

  SUBCASE("50% probability mutation")
  {
    double diff(0.0), length(0.0);

    prob.params.evolution.p_mutation = 0.5;

    for (unsigned i(0); i < n; ++i)
    {
      const team<individual> t1{t};

      t.mutation(prob);
      diff += distance(t, t1);
      length += active_slots(t1);
    }

    const double perc(100.0 * diff / length);
    CHECK(perc >= 45.0);
    CHECK(perc <= 55.0);
  }

  SUBCASE("Non-default temperature mutation")
  {
    SUBCASE("Zero temperature produces no mutations")
    {
      prob.params.evolution.p_mutation = 0.5;
      for (unsigned i(0); i < 100; ++i)
      {
        const auto nm(t.mutation(prob, 0.0));
        CHECK(nm == 0);
        CHECK(t == orig);
      }
    }

    SUBCASE("Temperature 0.5")
    {
      double diff(0.0), length(0.0);
      prob.params.evolution.p_mutation = 0.5;

      const unsigned trials(2000);
      for (unsigned i(0); i < trials; ++i)
      {
        const team<individual> t1{t};

        t.mutation(prob, 0.5);
        diff += distance(t, t1);
        length += active_slots(t1);
      }

      // Expected rate: 1.0 - sqrt(1.0 - 0.5) = 1.0 - sqrt(0.5) ~ 29.29%
      const double perc(100.0 * diff / length);
      CHECK(perc >= 25.0);
      CHECK(perc <= 34.0);
      CHECK(t.is_valid());
    }

    SUBCASE("Temperature 2.0")
    {
      double diff(0.0), length(0.0);
      prob.params.evolution.p_mutation = 0.5;

      const unsigned trials(2000);
      for (unsigned i(0); i < trials; ++i)
      {
        const team<individual> t1{t};

        t.mutation(prob, 2.0);
        diff += distance(t, t1);
        length += active_slots(t1);
      }

      // Expected rate: 1.0 - (1.0 - 0.5)^2 = 75.0%
      const double perc(100.0 * diff / length);
      CHECK(perc >= 70.0);
      CHECK(perc <= 80.0);
      CHECK(t.is_valid());
    }
  }
}

TEST_CASE_FIXTURE(fixture1, "Signature cache validation")
{
  using namespace ultra;
  using namespace ultra::gp;
  using individual = ultra::gp::individual;

  SUBCASE("Repeated calls return cached hash without recomputing")
  {
    tracking_individual ind1;
    ind1.sig = hash_t(111);
    tracking_individual ind2;
    ind2.sig = hash_t(222);

    team<tracking_individual> t({ind1, ind2});

    // Before signature() is called, signature cache is empty and members'
    // signature() has not been invoked.
    CHECK(t[0].sig_calls == 0);
    CHECK(t[1].sig_calls == 0);

    // First call computes hash from member signatures and caches it.
    const auto s1(t.signature());
    CHECK(!s1.empty());
    CHECK(t[0].sig_calls == 1);
    CHECK(t[1].sig_calls == 1);

    // Repeated calls return the cached hash without recomputing.
    const auto s2(t.signature());
    CHECK(s2 == s1);
    CHECK(t[0].sig_calls == 1);
    CHECK(t[1].sig_calls == 1);

    const auto s3(t.signature());
    CHECK(s3 == s1);
    CHECK(t[0].sig_calls == 1);
    CHECK(t[1].sig_calls == 1);

    // Also verify with `gp::individual`.
    team<individual> real_team(prob);
    const auto r1(real_team.signature());
    const auto r2(real_team.signature());
    const auto r3(real_team.signature());
    CHECK(r1 == r2);
    CHECK(r2 == r3);
    CHECK(!r1.empty());

    // Age increment does not invalidate the signature cache.
    real_team.inc_age();
    CHECK(real_team.signature() == r1);
  }

  SUBCASE("Mutation with nm > 0 clears cached signature")
  {
    tracking_individual ind1;
    ind1.sig = hash_t(111);
    ind1.mutations_to_perform = 1;

    tracking_individual ind2;
    ind2.sig = hash_t(222);
    ind2.mutations_to_perform = 0;

    team<tracking_individual> t({ind1, ind2});

    const auto s1(t.signature());
    CHECK(t[0].sig_calls == 1);
    CHECK(t[1].sig_calls == 1);

    // Mutation with `nm > 0` clears `signature_`.
    const auto nm(t.mutation(prob));
    CHECK(nm == 1);

    // Next `signature()` call must recompute and invoke member signatures.
    const auto s2(t.signature());
    CHECK(s2 != s1);
    CHECK(t[0].sig_calls == 2);
    CHECK(t[1].sig_calls == 2);

    // Subsequent call uses newly cached signature without recomputing.
    const auto s3(t.signature());
    CHECK(s3 == s2);
    CHECK(t[0].sig_calls == 2);
    CHECK(t[1].sig_calls == 2);

    // Mutation with `nm == 0` does NOT clear the cached signature.
    tracking_individual ind_zero1;
    ind_zero1.sig = hash_t(333);
    ind_zero1.mutations_to_perform = 0;

    tracking_individual ind_zero2;
    ind_zero2.sig = hash_t(444);
    ind_zero2.mutations_to_perform = 0;

    team<tracking_individual> tz({ind_zero1, ind_zero2});

    const auto sz1(tz.signature());
    CHECK(tz[0].sig_calls == 1);
    CHECK(tz[1].sig_calls == 1);

    const auto nm0(tz.mutation(prob));
    CHECK(nm0 == 0);

    // `signature_` was not cleared, so no recomputation happens.
    const auto sz2(tz.signature());
    CHECK(sz2 == sz1);
    CHECK(tz[0].sig_calls == 1);
    CHECK(tz[1].sig_calls == 1);

    // Also verify with `gp::individual`.
    prob.params.slp.code_length = 32;
    prob.params.evolution.p_mutation = 0.0;
    team<individual> real_team(prob);
    const auto orig_sig(real_team.signature());

    // When `nm == 0`, cached signature remains unchanged.
    const auto zero_nm(real_team.mutation(prob));
    CHECK(zero_nm == 0);
    CHECK(real_team.signature() == orig_sig);

    // When `nm > 0`, cached signature is cleared and recomputed.
    prob.params.evolution.p_mutation = 1.0;
    const auto real_nm(real_team.mutation(prob));
    CHECK(real_nm > 0);
    CHECK(real_team.is_valid());
    CHECK(real_team.signature() != orig_sig);
  }

  SUBCASE("Mutation with non-default temperature values")
  {
    tracking_individual ind1;
    ind1.sig = hash_t(555);
    ind1.mutations_to_perform = 1;

    tracking_individual ind2;
    ind2.sig = hash_t(666);
    ind2.mutations_to_perform = 1;

    team<tracking_individual> t({ind1, ind2});

    const auto s1(t.signature());
    CHECK(t[0].sig_calls == 1);
    CHECK(t[1].sig_calls == 1);

    // Temperature 0.5 passes temperature to members and clears cache if nm > 0.
    const auto nm_half(t.mutation(prob, 0.5));
    CHECK(nm_half == 2);
    CHECK(t[0].last_temperature == doctest::Approx(0.5));
    CHECK(t[1].last_temperature == doctest::Approx(0.5));

    const auto s2(t.signature());
    CHECK(s2 != s1);
    CHECK(t[0].sig_calls == 2);
    CHECK(t[1].sig_calls == 2);

    // Temperature 2.0 passes temperature to members and clears cache if nm > 0.
    const auto nm_double(t.mutation(prob, 2.0));
    CHECK(nm_double == 2);
    CHECK(t[0].last_temperature == doctest::Approx(2.0));
    CHECK(t[1].last_temperature == doctest::Approx(2.0));

    const auto s3(t.signature());
    CHECK(s3 != s2);
    CHECK(t[0].sig_calls == 3);
    CHECK(t[1].sig_calls == 3);

    // Also verify with `gp::individual`.
    prob.params.evolution.p_mutation = 0.5;
    team<individual> real_team(prob);
    const auto r_sig1(real_team.signature());

    // Temperature 0.0 produces nm == 0, preserving cached signature.
    const auto r_nm0(real_team.mutation(prob, 0.0));
    CHECK(r_nm0 == 0);
    CHECK(real_team.signature() == r_sig1);

    // Temperature 0.5 with nm > 0 clears cached signature.
    prob.params.evolution.p_mutation = 1.0;
    const auto r_nm_half(real_team.mutation(prob, 0.5));
    CHECK(r_nm_half > 0);
    CHECK(real_team.is_valid());
    const auto r_sig2(real_team.signature());
    CHECK(r_sig2 != r_sig1);

    // Temperature 2.0 with nm > 0 clears cached signature.
    const auto r_nm_double(real_team.mutation(prob, 2.0));
    CHECK(r_nm_double > 0);
    CHECK(real_team.is_valid());
    const auto r_sig3(real_team.signature());
    CHECK(r_sig3 != r_sig2);
  }
}

TEST_CASE_FIXTURE(fixture1, "Active slots")
{
  using namespace ultra;
  using namespace ultra::gp;
  using individual = ultra::gp::individual;

  SUBCASE("Empty team")
  {
    const team<individual> t;
    const team<individual> t0(0);
    const team<individual> tv(std::vector<individual>{});

    CHECK(active_slots(t) == 0);
    CHECK(active_slots(t0) == 0);
    CHECK(active_slots(tv) == 0);
  }

  SUBCASE("Exact return values against known individual active slot counts")
  {
    // ind1: 1 active slot, no introns (size 1).
    const individual ind1({
      {f_add, {1.0, 2.0}}
    });
    REQUIRE(ind1.is_valid());
    CHECK(ind1.size() == 1);
    CHECK(active_slots(ind1) == 1);

    // ind1_intron: 1 active slot, 1 intron at locus 0 (size 2).
    const individual ind1_intron({
      {f_add, {1.0, 2.0}},       // [0] intron
      {f_sub, {3.0, 4.0}}        // [1] root
    });
    REQUIRE(ind1_intron.is_valid());
    CHECK(ind1_intron.size() == 2);
    CHECK(active_slots(ind1_intron) == 1);

    // ind2: 2 active slots, no introns (size 2).
    const individual ind2({
      {f_add, {1.0, 2.0}},       // [0] active
      {f_sub, {0_addr, 3.0}}     // [1] active root
    });
    REQUIRE(ind2.is_valid());
    CHECK(ind2.size() == 2);
    CHECK(active_slots(ind2) == 2);

    // ind2_intron: 2 active slots, 1 intron at locus 0 (size 3).
    const individual ind2_intron({
      {f_mul, {5.0, 6.0}},       // [0] intron
      {f_add, {1.0, 2.0}},       // [1] active
      {f_sub, {1_addr, 3.0}}     // [2] active root
    });
    REQUIRE(ind2_intron.is_valid());
    CHECK(ind2_intron.size() == 3);
    CHECK(active_slots(ind2_intron) == 2);

    // ind3: 3 active slots, no introns (size 3).
    const individual ind3({
      {f_add, {1.0, 2.0}},       // [0] active
      {f_sub, {3.0, 4.0}},       // [1] active
      {f_mul, {0_addr, 1_addr}}  // [2] active root
    });
    REQUIRE(ind3.is_valid());
    CHECK(ind3.size() == 3);
    CHECK(active_slots(ind3) == 3);

    // ind3_intron: 3 active slots, 1 intron at locus 0 (size 4).
    const individual ind3_intron({
      {f_add, {9.0, 9.0}},       // [0] intron
      {f_add, {1.0, 2.0}},       // [1] active
      {f_sub, {3.0, 4.0}},       // [2] active
      {f_mul, {1_addr, 2_addr}}  // [3] active root
    });
    REQUIRE(ind3_intron.is_valid());
    CHECK(ind3_intron.size() == 4);
    CHECK(active_slots(ind3_intron) == 3);

    // Single-member teams.
    const team<individual> t_single1({ind1});
    CHECK(active_slots(t_single1) == 1);
    CHECK(active_slots(t_single1) == active_slots(ind1));

    const team<individual> t_single2({ind2});
    CHECK(active_slots(t_single2) == 2);
    CHECK(active_slots(t_single2) == active_slots(ind2));

    const team<individual> t_single2_intron({ind2_intron});
    CHECK(active_slots(t_single2_intron) == 2);
    CHECK(active_slots(t_single2_intron) == active_slots(ind2_intron));

    // Two-member team.
    const team<individual> t_pair({ind1, ind2});
    CHECK(active_slots(t_pair) == 3);
    CHECK(active_slots(t_pair) == active_slots(ind1) + active_slots(ind2));

    // Two-member team with introns.
    const team<individual> t_pair_introns({ind1_intron, ind3_intron});
    CHECK(active_slots(t_pair_introns) == 4);
    CHECK(active_slots(t_pair_introns) == active_slots(ind1_intron)
                                        + active_slots(ind3_intron));

    // Six-member team with mixed introns and active slots.
    const team<individual> t_all(
      {ind1, ind1_intron, ind2, ind2_intron, ind3, ind3_intron});
    // Expected sum: 1 + 1 + 2 + 2 + 3 + 3 = 12
    // Total lines (size): 1 + 2 + 2 + 3 + 3 + 4 = 15
    CHECK(active_slots(t_all) == 12);
    CHECK(active_slots(t_all) == active_slots(ind1)
                               + active_slots(ind1_intron)
                               + active_slots(ind2)
                               + active_slots(ind2_intron)
                               + active_slots(ind3)
                               + active_slots(ind3_intron));
  }

  SUBCASE("Generic individual with exact slot counts")
  {
    tracking_individual m1;
    m1.active_slots_count = 5;
    tracking_individual m2;
    m2.active_slots_count = 11;
    tracking_individual m3;
    m3.active_slots_count = 26;

    CHECK(active_slots(m1) == 5);
    CHECK(active_slots(m2) == 11);
    CHECK(active_slots(m3) == 26);

    const team<tracking_individual> tm({m1, m2, m3});
    CHECK(active_slots(tm) == 42);
    CHECK(active_slots(tm) == active_slots(m1)
                            + active_slots(m2)
                            + active_slots(m3));
  }

  SUBCASE("Randomly generated teams match sum of member active slots")
  {
    for (unsigned i(0); i < 100; ++i)
    {
      const team<individual> tr(prob);
      const unsigned expected(
        std::accumulate(tr.begin(), tr.end(), 0u,
                        [](unsigned sum, const individual &ind)
                        {
                          return sum + active_slots(ind);
                        }));
      CHECK(active_slots(tr) == expected);
    }
  }
}

TEST_CASE_FIXTURE(fixture1, "Comparison")
{
  using namespace ultra::gp;

  for (unsigned i(0); i < 2000; ++i)
  {
    team<individual> a(prob);
    CHECK(a == a);
    CHECK(distance(a, a) == 0);

    team<individual> b(a);
    CHECK(a.signature() == b.signature());
    CHECK(a == b);
    CHECK(distance(a, b) == 0);

    b.inc_age(1u + ultra::random::sup(100u));
    CHECK(a.age() != b.age());
    CHECK(a == b);
    CHECK(distance(a, b) == 0);

    team<individual> c(prob);
    if (a.signature() != c.signature())
    {
      CHECK(a != c);
      CHECK(distance(a, c) > 0);
    }
  }
}

TEST_CASE_FIXTURE(fixture1, "Age")
{
  using namespace ultra::gp;

  SUBCASE("inc_age")
  {
    team<individual> t(prob);
    REQUIRE(t.size() > 0);
    CHECK(t.age() == 0);
    for (const auto &ind : t)
      CHECK(ind.age() == 0);

    const auto orig_sig(t.signature());

    // Default delta is 1
    t.inc_age();
    CHECK(t.age() == 1);
    for (const auto &ind : t)
      CHECK(ind.age() == 1);

    // Custom delta
    t.inc_age(4);
    CHECK(t.age() == 5);
    for (const auto &ind : t)
      CHECK(ind.age() == 5);

    // Zero delta leaves age unchanged
    t.inc_age(0);
    CHECK(t.age() == 5);
    for (const auto &ind : t)
      CHECK(ind.age() == 5);

    // Invariants and signature preserved
    CHECK(t.is_valid());
    CHECK(t.signature() == orig_sig);
  }

  SUBCASE("Averaging and integer division with non-uniform ages")
  {
    const individual i0(prob);
    const individual i1(prob);
    const individual i2(prob);
    const individual i3(prob);

    // Two members
    {
      auto ind0(i0);
      auto ind1(i1);
      ind1.inc_age(1);
      const team<individual> t2({ind0, ind1});
      // (0 + 1) / 2 = 0
      CHECK(t2.age() == 0);

      ind0.inc_age(1);
      ind1.inc_age(1);
      const team<individual> t2_b({ind0, ind1});
      // (1 + 2) / 2 = 1
      CHECK(t2_b.age() == 1);
    }

    // Three members
    {
      auto ind0(i0);
      auto ind1(i1);
      auto ind2(i2);
      ind0.inc_age(1);
      ind1.inc_age(2);
      ind2.inc_age(4);

      team<individual> t3({ind0, ind1, ind2});
      CHECK(t3[0].age() == 1);
      CHECK(t3[1].age() == 2);
      CHECK(t3[2].age() == 4);
      // (1 + 2 + 4) / 3 = 7 / 3 = 2
      CHECK(t3.age() == 2);

      // (1 + 2 + 5) / 3 = 8 / 3 = 2
      ind2.inc_age(1);
      const team<individual> t3_b({ind0, ind1, ind2});
      CHECK(t3_b.age() == 2);

      // (1 + 2 + 6) / 3 = 9 / 3 = 3
      ind2.inc_age(1);
      const team<individual> t3_c({ind0, ind1, ind2});
      CHECK(t3_c.age() == 3);

      // inc_age on team with non-uniform member ages
      // {1, 2, 4} + 1 -> {2, 3, 5}, sum 10, 10 / 3 = 3
      t3.inc_age();
      CHECK(t3[0].age() == 2);
      CHECK(t3[1].age() == 3);
      CHECK(t3[2].age() == 5);
      CHECK(t3.age() == 3);

      // {2, 3, 5} + 2 -> {4, 5, 7}, sum 16, 16 / 3 = 5
      t3.inc_age(2);
      CHECK(t3[0].age() == 4);
      CHECK(t3[1].age() == 5);
      CHECK(t3[2].age() == 7);
      CHECK(t3.age() == 5);
    }

    // Four members
    {
      auto ind0(i0);
      auto ind1(i1);
      auto ind2(i2);
      auto ind3(i3);

      ind3.inc_age(3);
      const team<individual> t4({ind0, ind1, ind2, ind3});
      // (0 + 0 + 0 + 3) / 4 = 0
      CHECK(t4.age() == 0);

      ind3.inc_age(1);
      const team<individual> t4_b({ind0, ind1, ind2, ind3});
      // (0 + 0 + 0 + 4) / 4 = 1
      CHECK(t4_b.age() == 1);

      ind0.inc_age(2);
      ind1.inc_age(3);
      ind2.inc_age(5);
      ind3.inc_age(3);  // ind3 was 4, now 7
      const team<individual> t4_c({ind0, ind1, ind2, ind3});
      // (2 + 3 + 5 + 7) / 4 = 17 / 4 = 4
      CHECK(t4_c.age() == 4);
    }
  }

  SUBCASE("Equality indifference to age")
  {
    const team<individual> t1(prob);
    team<individual> t2(t1);

    REQUIRE(t1 == t2);
    REQUIRE(t1.age() == t2.age());

    t2.inc_age(10);

    CHECK(t1.age() != t2.age());
    CHECK(t1 == t2);
    CHECK(t2 == t1);
    CHECK(!(t1 != t2));
    CHECK(!(t2 != t1));
    CHECK(distance(t1, t2) == 0);
    CHECK(t1.signature() == t2.signature());

    // Non-uniform ages between teams with identical member code
    individual i0(prob);
    individual i1(prob);
    individual i2(prob);

    const team<individual> uniform({i0, i1, i2});

    i0.inc_age(3);
    i1.inc_age(7);
    i2.inc_age(11);
    const team<individual> non_uniform({i0, i1, i2});

    CHECK(uniform.age() == 0);
    CHECK(non_uniform.age() == 7);  // (3 + 7 + 11) / 3 = 21 / 3 = 7
    CHECK(uniform == non_uniform);
    CHECK(non_uniform == uniform);
    CHECK(!(uniform != non_uniform));
    CHECK(distance(uniform, non_uniform) == 0);
    CHECK(uniform.signature() == non_uniform.signature());
  }
}

TEST_CASE_FIXTURE(fixture1, "Iterators")
{
  using namespace ultra::gp;

  for (unsigned j(0); j < 1000; ++j)
  {
    team<individual> t(prob);

    for (unsigned i(0); const auto &ind : t)
    {
      CHECK(ind == t[i]);
      ++i;
    }
  }
}

TEST_CASE_FIXTURE(fixture1, "Crossover")
{
  using namespace ultra;

  prob.params.slp.code_length = 100;

  gp::team<gp::individual> t1(prob), t2(prob);

  const unsigned n(2000);
  for (unsigned j(0); j < n; ++j)
  {
    const auto tc(crossover(prob, t1, t2));
    CHECK(tc.is_valid());

    for (std::size_t p(0); p < tc.size(); ++p)
      for (locus::index_t i(0); i < tc[p].size(); ++i)
        for (symbol::category_t c(0); c < tc[p].categories(); ++c)
        {
          const locus l{i, c};

          CHECK((tc[p][l] == t1[p][l] || tc[p][l] == t2[p][l]));
        }
  }
}

TEST_CASE_FIXTURE(fixture1, "Serialization")
{
  using namespace ultra;

  SUBCASE("Uniform team age")
  {
    for (unsigned i(0); i < 2000; ++i)
    {
      std::stringstream ss;
      gp::team<gp::individual> t1(prob);

      t1.inc_age(random::sup(100u));

      CHECK(t1.save(ss));

      gp::team<gp::individual> t2(prob);
      CHECK(t2.load(ss, prob.sset));
      CHECK(t2.is_valid());

      CHECK(t1 == t2);
      CHECK(t2.age() == t1.age());
      for (std::size_t j(0); j < t1.size(); ++j)
        CHECK(t2[j].age() == t1[j].age());
    }
  }

  SUBCASE("Non-uniform member ages")
  {
    for (unsigned i(0); i < 500; ++i)
    {
      std::vector<gp::individual> members;
      const auto team_size(prob.params.team.individuals);
      members.reserve(team_size);

      for (std::size_t j(0); j < team_size; ++j)
      {
        gp::individual ind(prob);
        ind.inc_age(random::sup(100u));
        members.push_back(std::move(ind));
      }

      const gp::team<gp::individual> t1(std::move(members));
      std::stringstream ss;
      CHECK(t1.save(ss));

      gp::team<gp::individual> t2(prob);
      CHECK(t2.load(ss, prob.sset));
      CHECK(t2.is_valid());

      CHECK(t1 == t2);
      CHECK(t2.age() == t1.age());
      for (std::size_t j(0); j < t1.size(); ++j)
        CHECK(t2[j].age() == t1[j].age());
    }
  }
}

TEST_CASE_FIXTURE(fixture1, "Decision vector")
{
  using namespace ultra;
  using pk = dv_param_kind;

  SUBCASE("Extraction from simple team")
  {
    const gp::team<gp::individual> t(
      {
        gp::individual(
          {
            {f_add, {3.0, 2.0}},       // member 0, [0]
            {f_add, {0_addr, 1}},      // member 0, [1]
            {f_sub, {1_addr, 0_addr}}  // member 0, [2]
          }),
        gp::individual(
          {
            {f_add, {4.0, 5.0}},       // member 1, [0]
            {f_sub, {0_addr, 2.0}}     // member 1, [1]
          })
      });

    const auto dv(extract_decision_vector(t));

    REQUIRE(dv.is_valid());
    CHECK(!dv.empty());
    CHECK(dv.size() == 6);

    CHECK(dv.values[0] == doctest::Approx(1.0));
    CHECK(dv.values[1] == doctest::Approx(3.0));
    CHECK(dv.values[2] == doctest::Approx(2.0));
    CHECK(dv.values[3] == doctest::Approx(2.0));
    CHECK(dv.values[4] == doctest::Approx(4.0));
    CHECK(dv.values[5] == doctest::Approx(5.0));

    CHECK(dv.coords[0].coord.ind_index == 0);
    CHECK(dv.coords[0].coord.loc == locus{1, 0});
    CHECK(dv.coords[0].coord.arg_index == 1);
    CHECK(dv.coords[0].kind == pk::integer);

    CHECK(dv.coords[1].coord.ind_index == 0);
    CHECK(dv.coords[1].coord.loc == locus{0, 0});
    CHECK(dv.coords[1].coord.arg_index == 0);
    CHECK(dv.coords[1].kind == pk::real);

    CHECK(dv.coords[3].coord.ind_index == 1);
    CHECK(dv.coords[3].coord.loc == locus{1, 0});
    CHECK(dv.coords[3].coord.arg_index == 1);
    CHECK(dv.coords[3].kind == pk::real);

    CHECK(dv.coords[5].coord.ind_index == 1);
    CHECK(dv.coords[5].coord.loc == locus{0, 0});
    CHECK(dv.coords[5].coord.arg_index == 1);
    CHECK(dv.coords[5].kind == pk::real);
  }

  SUBCASE("Apply modified real values")
  {
    gp::team<gp::individual> t(
      {
        gp::individual(
          {
            {f_add, {3.0, 2.0}},
            {f_add, {0_addr, 1.0}},
            {f_sub, {1_addr, 0_addr}}
          }),
        gp::individual(
          {
            {f_add, {4.0, 5.0}},
            {f_sub, {0_addr, 2.0}}
          })
      });

    auto dv(extract_decision_vector(t));
    REQUIRE(dv.is_valid());
    REQUIRE(dv.size() == 6);

    dv.values[0] = -4.25;
    dv.values[1] =  7.0;
    dv.values[2] =  9.5;
    dv.values[3] = -1.5;

    t.apply_decision_vector(dv);

    CHECK(std::get<D_DOUBLE>(t[0][{1, 0}].args[1]) == doctest::Approx(-4.25));
    CHECK(std::get<D_DOUBLE>(t[0][{0, 0}].args[0]) == doctest::Approx(7.0));
    CHECK(std::get<D_DOUBLE>(t[0][{0, 0}].args[1]) == doctest::Approx(9.5));
    CHECK(std::get<D_DOUBLE>(t[1][{1, 0}].args[1]) == doctest::Approx(-1.5));
  }

  SUBCASE("Apply rounds integer values")
  {
    gp::team<gp::individual> t(
      {
        gp::individual(
          {
            {f_add, {3.0, 2.0}},
            {f_add, {0_addr, 1}},
            {f_sub, {1_addr, 0_addr}}
          })
      });

    auto dv(extract_decision_vector(t));
    REQUIRE(dv.is_valid());
    REQUIRE(dv.size() == 3);

    dv.values[0] = 8.6;
    t.apply_decision_vector(dv);

    CHECK(std::get<D_INT>(t[0][{1, 0}].args[1]) == 9);
  }

  SUBCASE("Round trip preserves team")
  {
    const gp::team<gp::individual> original(
      {
        gp::individual(
          {
            {f_add, {3.0, 2.0}},
            {f_add, {0_addr, 1}},
            {f_sub, {1_addr, 0_addr}}
          }),
        gp::individual(
          {
            {f_add, {4.0, 5.0}},
            {f_sub, {0_addr, 2.0}}
          })
      });

    gp::team<gp::individual> copy(original);

    const auto dv(extract_decision_vector(copy));
    REQUIRE(dv.is_valid());

    copy.apply_decision_vector(dv);

    CHECK(copy == original);
    CHECK(copy.signature() == original.signature());
  }

  SUBCASE("Applying changed decision vector updates signature")
  {
    gp::team<gp::individual> t(
      {
        gp::individual(
          {
            {f_add, {3.0, 2.0}},
            {f_add, {0_addr, 1.0}},
            {f_sub, {1_addr, 0_addr}}
          }),
        gp::individual(
          {
            {f_add, {4.0, 5.0}},
            {f_sub, {0_addr, 2.0}}
          })
      });

    const auto old_sig(t.signature());

    auto dv(extract_decision_vector(t));
    REQUIRE(dv.is_valid());

    dv.values[0] += 1.0;
    t.apply_decision_vector(dv);

    CHECK(t.signature() != old_sig);
  }
}

TEST_CASE_FIXTURE(fixture1, "operator<< stream output")
{
  using namespace ultra;

  const gp::team<gp::individual> t(
    {
      gp::individual(
        {
          {f_add, {2.0, z}},         // [0]
          {f_add, {3.0, 4.0}},       // [1]
          {f_sub, {0_addr, 1_addr}}  // [2]
        }),
      gp::individual(
        {
          {f_add, {4.0, 5.0}},       // [0]
          {f_sub, {0_addr, 2.0}}     // [1]
        })
    });

  SUBCASE("multiline")
  {
    std::ostringstream ss;
    ss << t;

    std::ostringstream expected;
    expected << t[0] << '\n' << t[1] << '\n';

    CHECK(ss.str() == expected.str());
    CHECK(ss.str() == "[2] FSUB [0] [1]\n"
                      "[1] FADD 3 4\n"
                      "[0] FADD 2 Z()\n"
                      "\n"
                      "[1] FSUB [0] 2\n"
                      "[0] FADD 4 5\n"
                      "\n");

    const gp::team<gp::individual> single(
      {
        gp::individual({{f_add, {2.0, z}}})
      });

    std::ostringstream ss_single;
    ss_single << single;

    std::ostringstream expected_single;
    expected_single << single[0] << '\n';

    CHECK(ss_single.str() == expected_single.str());
  }

  SUBCASE("out::in_line_f")
  {
    std::ostringstream ss;
    ss << out::in_line << t;

    std::ostringstream expected;
    expected << '{' << out::in_line << t[0] << '}'
             << '{' << out::in_line << t[1] << '}';

    CHECK(ss.str() == expected.str());
    CHECK(ss.str() == "{FSUB FADD 2 Z() FADD 3 4}{FSUB FADD 4 5 2}");

    std::ostringstream ss_pf;
    ss_pf << out::print_format(out::in_line_f) << t;
    CHECK(ss_pf.str() == ss.str());

    const gp::team<gp::individual> single(
      {
        gp::individual({{f_add, {2.0, z}}})
      });

    std::ostringstream ss_single;
    ss_single << out::in_line << single;

    CHECK(ss_single.str() == "{FADD 2 Z()}");
  }

  SUBCASE("stream output with empty teams")
  {
    const gp::team<gp::individual> empty;
    const gp::team<gp::individual> empty_sized(0);
    const gp::team<gp::individual> empty_vec(std::vector<gp::individual>{});

    std::ostringstream ss_multi;
    ss_multi << empty;
    CHECK(ss_multi.str().empty());

    std::ostringstream ss_inline;
    ss_inline << out::in_line << empty;
    CHECK(ss_inline.str().empty());

    std::ostringstream ss_pf;
    ss_pf << out::print_format(out::in_line_f) << empty;
    CHECK(ss_pf.str().empty());

    std::ostringstream ss_sized_multi;
    ss_sized_multi << empty_sized;
    CHECK(ss_sized_multi.str().empty());

    std::ostringstream ss_sized_inline;
    ss_sized_inline << out::in_line << empty_sized;
    CHECK(ss_sized_inline.str().empty());

    std::ostringstream ss_vec_multi;
    ss_vec_multi << empty_vec;
    CHECK(ss_vec_multi.str().empty());

    std::ostringstream ss_vec_inline;
    ss_vec_inline << out::in_line << empty_vec;
    CHECK(ss_vec_inline.str().empty());
  }
}

}  // TEST_SUITE
