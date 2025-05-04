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

#include "test/debug_datasets.h"

#include "kernel/random.h"
#include "kernel/gp/src/dataframe.h"

#include <sstream>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

class random_csv_line
{
public:
  explicit random_csv_line(const std::vector<ultra::domain_t> &format)
    : head_(format.front()), tail_(std::next(format.begin()), format.end())
  {
    Expects(format.size());
  }

  [[nodiscard]] std::string get() const
  {
    using namespace ultra;

    const auto random_field([](domain_t d)
    {
      switch (d)
      {
      case d_int:
        return std::to_string(random::between(0, 100000000));
      case d_double:
        return std::to_string(random::between(0.0, 1000.0));
      default:
      {
        const auto random_char([]
        {
          return random::element(std::string(
                                   "0123456789"
                                   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "abcdefghijklmnopqrstuvwxyz"));
        });

        std::string s;
        std::generate_n(std::back_inserter(s),
                        random::between(1, 40),
                        random_char);

        return "\"" + s + "\"";
      }
      }
    });

    std::string line(random_field(head_));

    for (auto d : tail_)
      line += "," + random_field(d);

    return line;
  }

private:
  const ultra::domain_t head_;
  const std::vector<ultra::domain_t> tail_;
};

TEST_SUITE("DATAFRAME")
{

TEST_CASE("push_back / insert")
{
  using namespace ultra;
  using ultra::src::dataframe;

  dataframe d1;

  const std::size_t nr(1000);

  for (std::size_t i(0); i < nr; ++i)
  {
    src::example ex;

    ex.input = std::vector<value_t>{random::sup(1000.0),
                                    random::sup(1000.0),
                                    random::sup(1000.0)};
    ex.output = random::sup(1000.0);

    d1.push_back(ex);
  }

  CHECK(d1.size() == nr);

  dataframe d2;
  d2.insert(d2.begin(), d1.begin(), d1.end());

  CHECK(std::ranges::equal(d1, d2));
}

TEST_CASE("swap")
{
  using namespace ultra;
  using ultra::src::dataframe;

  std::istringstream is(debug::sr);
  dataframe sr(is);
  CHECK(sr.size() == debug::SR_COUNT);

  const dataframe backup(sr);
  CHECK(backup.size() == sr.size());

  dataframe empty;
  CHECK(empty.empty());

  sr.swap(empty);

  CHECK(std::ranges::equal(empty, backup));
  CHECK(empty.columns.size() == backup.columns.size());
  CHECK(sr.empty());
  CHECK(sr.columns.empty());
}

TEST_CASE("Filtering")
{
  using namespace ultra;
  using ultra::src::dataframe;

  constexpr std::size_t LINES(1000);
  std::string random_csv;
  random_csv_line line({d_int, d_string, d_int, d_double, d_double, d_string});

  for (std::size_t lines(LINES); lines; --lines)
    random_csv += line.get() + "\n";

  std::istringstream ss(random_csv);

  SUBCASE("Random dataframe")
  {
    dataframe d;
    CHECK(d.read_csv(ss));
    CHECK(d.size() == LINES);
  }

  SUBCASE("Random dataframe / random filtering")
  {
    dataframe::params p;
    p.filter = [](auto &) { return random::boolean(); };

    dataframe d;
    d.read_csv(ss, p);

    const auto half(LINES / 2);
    CHECK(10 * d.size() <= 11 * half);
    CHECK(9 * half <= 10 * d.size());
  }
}

TEST_CASE("load_csv headers")
{
  using namespace ultra;
  using ultra::src::dataframe;

  std::istringstream wine(debug::wine);

  constexpr std::size_t ncol(12);

  dataframe d;
  dataframe::params p;

  CHECK(d.columns.size() == 0);
  CHECK(d.columns.empty());

  CHECK(d.read_csv(wine, p) == debug::WINE_COUNT);
  CHECK(d.is_valid());

  CHECK(d.columns.size() == ncol);
  CHECK(!d.columns.empty());

  CHECK(d.columns[ 0].name() ==        "fixed acidity");
  CHECK(d.columns[ 1].name() ==     "volatile acidity");
  CHECK(d.columns[ 2].name() ==          "citric acid");
  CHECK(d.columns[ 3].name() ==       "residual sugar");
  CHECK(d.columns[ 4].name() ==            "chlorides");
  CHECK(d.columns[ 5].name() ==  "free sulfur dioxide");
  CHECK(d.columns[ 6].name() == "total sulfur dioxide");
  CHECK(d.columns[ 7].name() ==              "density");
  CHECK(d.columns[ 8].name() ==                   "pH");
  CHECK(d.columns[ 9].name() ==            "sulphates");
  CHECK(d.columns[10].name() ==              "alcohol");
  CHECK(d.columns[11].name() ==              "quality");

  CHECK(d.columns.begin()->name() == d.columns[       0].name());
  CHECK(d.columns.begin()->name() ==   d.columns.front().name());
  CHECK(d.columns.back().name()   == d.columns[ncol - 1].name());

  std::size_t count(0);
  for (const auto &c: d.columns)
  {
    ++count;
    CHECK(c.domain() == d_double);
  }
  CHECK(count == ncol);

  CHECK(d.classes() == 0);
  CHECK(d.front().input.size() == ncol - 1);

  for (const auto &e : d)
  {
    CHECK(std::holds_alternative<D_DOUBLE>(e.output));

    for (const auto &i : e.input)
      CHECK(std::holds_alternative<D_DOUBLE>(i));
  }
}

TEST_CASE("load_csv output_index")
{
  using namespace ultra;
  using ultra::src::dataframe;

  std::istringstream abalone(debug::abalone);

  constexpr std::size_t ncol(9);

  dataframe d;
  dataframe::params p;
  p.output_index = 8;

  CHECK(d.columns.size() == 0);
  CHECK(d.columns.empty());

  CHECK(d.read_csv(abalone, p) == debug::ABALONE_COUNT);
  CHECK(d.is_valid());

  CHECK(d.columns.size() == ncol);
  CHECK(!d.columns.empty());

  CHECK(d.columns[ 0].name() ==          "rings");
  CHECK(d.columns[ 1].name() ==            "sex");
  CHECK(d.columns[ 2].name() ==         "length");
  CHECK(d.columns[ 3].name() ==       "diameter");
  CHECK(d.columns[ 4].name() ==         "height");
  CHECK(d.columns[ 5].name() ==   "whole weight");
  CHECK(d.columns[ 6].name() == "shucked weight");
  CHECK(d.columns[ 7].name() == "viscera weight");
  CHECK(d.columns[ 8].name() ==   "shell weight");

  CHECK(d.columns.begin()->name() == d.columns[       0].name());
  CHECK(d.columns.begin()->name() ==   d.columns.front().name());
  CHECK(d.columns.back().name()   == d.columns[ncol - 1].name());

  CHECK(d.columns[0].domain() == d_double);
  CHECK(d.columns[1].domain() == d_string);

  CHECK(d.classes() == 0);
  CHECK(d.front().input.size() == ncol - 1);

  CHECK(std::holds_alternative<D_DOUBLE>(d.front().output));
  CHECK(std::holds_alternative<D_STRING>(d.front().input[0]));
  CHECK(std::holds_alternative<D_DOUBLE>(d.front().input[1]));
}

TEST_CASE("load_csv_no_output_index")
{
  using namespace ultra;
  using ultra::src::dataframe;

  std::istringstream ecoli(debug::ecoli);

  constexpr std::size_t ncol(9);

  dataframe d;
  dataframe::params p;
  p.output_index = std::nullopt;

  CHECK(d.columns.size() == 0);
  CHECK(d.columns.empty());

  CHECK(d.read_csv(ecoli, p) == debug::ECOLI_COUNT);
  CHECK(d.is_valid());

  CHECK(d.columns.size() == ncol + 1);
  CHECK(!d.columns.empty());

  CHECK(d.columns[ 0].name() ==              "");
  CHECK(d.columns[ 1].name() == "sequence name");
  CHECK(d.columns[ 2].name() ==           "mcg");
  CHECK(d.columns[ 3].name() ==           "gvh");
  CHECK(d.columns[ 4].name() ==           "lip");
  CHECK(d.columns[ 5].name() ==           "chg");
  CHECK(d.columns[ 6].name() ==           "aac");
  CHECK(d.columns[ 7].name() ==          "alm1");
  CHECK(d.columns[ 8].name() ==          "alm2");
  CHECK(d.columns[ 9].name() ==  "localization");

  CHECK(d.columns.begin()->name() == d.columns[       0].name());
  CHECK(d.columns.begin()->name() ==   d.columns.front().name());
  CHECK(d.columns.back().name()   ==     d.columns[ncol].name());

  CHECK(d.columns[1].domain() == d_string);
  CHECK(d.columns[2].domain() == d_double);
  CHECK(d.columns[3].domain() == d_double);
  CHECK(d.columns[4].domain() == d_double);
  CHECK(d.columns[5].domain() == d_double);
  CHECK(d.columns[6].domain() == d_double);
  CHECK(d.columns[7].domain() == d_double);
  CHECK(d.columns[8].domain() == d_double);
  CHECK(d.columns[9].domain() == d_string);

  CHECK(d.classes() == 0);

  for (const auto &e : d)
  {
    CHECK(e.input.size() == ncol);
    CHECK(!has_value(e.output));
  }
}

TEST_CASE("load_csv_classification")
{
  using namespace ultra;
  using ultra::src::dataframe;

  std::istringstream iris(debug::iris);

  constexpr std::size_t ncol(5);

  dataframe d;
  dataframe::params p;
  p.output_index = 4;

  CHECK(d.columns.size() == 0);
  CHECK(d.columns.empty());

  CHECK(d.read_csv(iris, p) == debug::IRIS_COUNT);
  CHECK(d.is_valid());

  CHECK(d.columns.size() == ncol);
  CHECK(!d.columns.empty());

  CHECK(d.columns[ 0].name() ==        "class");
  CHECK(d.columns[ 1].name() == "sepal length");
  CHECK(d.columns[ 2].name() ==  "sepal width");
  CHECK(d.columns[ 3].name() == "petal length");
  CHECK(d.columns[ 4].name() ==  "petal width");

  CHECK(d.columns.begin()->name() == d.columns[       0].name());
  CHECK(d.columns.begin()->name() ==   d.columns.front().name());
  CHECK(d.columns.back().name()   == d.columns[ncol - 1].name());

  std::size_t count(0);
  for (const auto &c: d.columns)
  {
    ++count;
    CHECK(c.domain() == d_double);
  }
  CHECK(count == ncol);

  CHECK(d.classes() == 3);
  CHECK(d.front().input.size() == ncol - 1);

  CHECK(d.class_name(0) == "Iris-setosa");
  CHECK(d.class_name(1) == "Iris-versicolor");
  CHECK(d.class_name(2) == "Iris-virginica");
}

TEST_CASE("load_xrff_classification")
{
  using namespace ultra;
  using ultra::src::dataframe;

std::istringstream iris_xrff(R"(
<dataset name="iris">
  <header>
    <attributes>
      <attribute class="yes" name="class" type="nominal">
        <labels>
          <label>Iris-setosa</label>
          <label>Iris-versicolor</label>
          <label>Iris-virginica</label>
        </labels>
      </attribute>
      <attribute name="sepallength" type="numeric" />
      <attribute name="sepalwidth" type="numeric" />
      <attribute name="petallength" type="numeric" />
      <attribute name="petalwidth" type="numeric" />
    </attributes>
  </header>
  <body>
    <instances>
      <instance><value>Iris-setosa</value><value>5.1</value><value>3.5</value><value>1.4</value><value>0.2</value></instance>
      <instance><value>Iris-setosa</value><value>4.9</value><value>3</value><value>1.4</value><value>0.2</value></instance>
      <instance><value>Iris-setosa</value><value>4.7</value><value>3.2</value><value>1.3</value><value>0.2</value></instance>
      <instance><value>Iris-versicolor</value><value>7</value><value>3.2</value><value>4.7</value><value>1.4</value></instance>
      <instance><value>Iris-versicolor</value><value>6.4</value><value>3.2</value><value>4.5</value><value>1.5</value></instance>
      <instance><value>Iris-versicolor</value><value>6.9</value><value>3.1</value><value>4.9</value><value>1.5</value></instance>
      <instance><value>Iris-virginica</value><value>6.3</value><value>3.3</value><value>6</value><value>2.5</value></instance>
      <instance><value>Iris-virginica</value><value>5.8</value><value>2.7</value><value>5.1</value><value>1.9</value></instance>
      <instance><value>Iris-virginica</value><value>7.1</value><value>3</value><value>5.9</value><value>2.1</value></instance>
      <instance><value>Iris-virginica</value><value>6.3</value><value>2.9</value><value>5.6</value><value>1.8</value></instance>
    </instances>
  </body>
</dataset>)");

  constexpr std::size_t ncol(5);

  dataframe d;

  CHECK(d.columns.size() == 0);
  CHECK(d.columns.empty());

  CHECK(d.read_xrff(iris_xrff) == 10);
  CHECK(d.is_valid());

  CHECK(d.columns.size() == ncol);
  CHECK(!d.columns.empty());

  CHECK(d.columns[0].name() ==       "class");
  CHECK(d.columns[1].name() == "sepallength");
  CHECK(d.columns[2].name() ==  "sepalwidth");
  CHECK(d.columns[3].name() == "petallength");
  CHECK(d.columns[4].name() ==  "petalwidth");

  CHECK(d.columns.begin()->name() == d.columns[       0].name());
  CHECK(d.columns.begin()->name() ==   d.columns.front().name());
  CHECK(d.columns.back().name()   == d.columns[ncol - 1].name());

  std::size_t count(0);
  for (const auto &c: d.columns)
  {
    ++count;
    CHECK(c.domain() == d_double);
  }
  CHECK(count == ncol);

  CHECK(d.classes() == 3);
  CHECK(d.front().input.size() == ncol - 1);

  CHECK(d.class_name(0) == "Iris-setosa");
  CHECK(d.class_name(1) == "Iris-versicolor");
  CHECK(d.class_name(2) == "Iris-virginica");
}

}  // TEST_SUITE("DATAFRAME")
