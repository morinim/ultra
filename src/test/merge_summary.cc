/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2026 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

#include "tinyxml2/tinyxml2.h"
#include "utility/xml_tools.h"

#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#ifndef _WIN32
#include <sys/wait.h>
#endif


namespace fs = std::filesystem;
using tinyxml2::XMLDocument;
using tinyxml2::XMLElement;


const fs::path script {"merge_summary.py"};


// ------------------------------
// File helpers
// ------------------------------

[[nodiscard]] std::string read_all(const fs::path &p)
{
  std::ifstream in(p);
  REQUIRE_MESSAGE(in.good(), "Failed to open file: " << p.string());

  std::ostringstream oss;
  oss << in.rdbuf();
  return oss.str();
}

void write_all(const fs::path &p, const std::string &s)
{
  std::ofstream out(p);
  REQUIRE_MESSAGE(out.good(), "Failed to write file: " << p.string());

  out.write(s.data(), static_cast<std::streamsize>(s.size()));
  REQUIRE_MESSAGE(static_cast<bool>(out), "Write failed: " << p.string());
}

[[nodiscard]] fs::path make_temp_dir()
{
  const auto base(fs::temp_directory_path());

  for (int i(0); i < 2000; ++i)
  {
    const auto candidate(base
                         / ("ultra_merge_summary_test_"
                            + std::to_string(std::rand())
                            + "_" + std::to_string(i)));
    std::error_code ec;
    if (fs::create_directories(candidate, ec) && !ec)
      return candidate;
  }

  FAIL("Unable to create a temporary directory");
  return {};
}

int run_cli(const fs::path &a, const fs::path &b,
            const fs::path &out, const fs::path &err)
{
  std::ostringstream cmd;
  cmd << "python3 "
      << std::quoted(script.string()) << " "
      << std::quoted(a.string()) << " "
      << std::quoted(b.string()) << " "
      << std::quoted(out.string())
      << " 2> " << std::quoted(err.string());

  const int rc(std::system(cmd.str().c_str()));

#ifdef _WIN32
  return rc;
#else
  if (rc == -1) return rc;
  if (WIFEXITED(rc)) return WEXITSTATUS(rc);
  return rc;
#endif
}


// ------------------------------
// tinyxml2 navigation helpers
// ------------------------------
[[nodiscard]] XMLElement *require_child(XMLElement *parent, const char *name)
{
  REQUIRE_MESSAGE(parent != nullptr,
                  "Parent is null while looking for child: " << name);
  auto *child(parent->FirstChildElement(name));
  REQUIRE_MESSAGE(child != nullptr, "Missing element: " << name);

  return child;
}

[[nodiscard]] const char *require_text(XMLElement *el)
{
  REQUIRE_MESSAGE(el != nullptr, "Element is null while reading text");
  const char *t(el->GetText());
  REQUIRE_MESSAGE(t != nullptr, "Element has no text: " << el->Name());

  return t;
}

[[nodiscard]] long long require_ll(XMLElement *el)
{
  const char *t(require_text(el));
  char *end(nullptr);
  const long long v(std::strtoll(t, &end, 10));

  const bool valid_end(end != nullptr && *end == '\0');
  REQUIRE_MESSAGE(valid_end, "Not an integer: '" << t << "'");

  return v;
}

[[nodiscard]] double require_double(XMLElement *el)
{
  const char *t(require_text(el));
  char *end(nullptr);
  const double v(std::strtod(t, &end));

  const bool valid_end(end != nullptr && *end == '\0');
  REQUIRE_MESSAGE(valid_end, "Not a float: '" << t << "'");
  return v;
}

[[nodiscard]] std::vector<long long> read_solution_runs(XMLElement *summary)
{
  std::vector<long long> out;
  auto *solutions(require_child(summary, "solutions"));
  for (auto *r(solutions->FirstChildElement("run")); r;
       r = r->NextSiblingElement("run"))
    out.push_back(require_ll(r));
  return out;
}


// Verify `<checksum>XXXXXXXX</checksum>` matches CRC32 of the full XML bytes
// with checksum text replaced by "00000000" (exactly what the Python script
// does).
void check_checksum_matches(const std::string &xml_bytes)
{
  constexpr std::string_view open("<checksum>");
  constexpr std::string_view close("</checksum>");

  static auto to_hex8_upper([](auto v)
  {
    std::ostringstream oss;
    oss << std::uppercase << std::hex << std::setw(8) << std::setfill('0')
        << v;
    return oss.str();
  });

  const auto p0(xml_bytes.find(open));
  REQUIRE_MESSAGE(p0 != std::string::npos, "Missing <checksum> tag in output");

  const auto p1(xml_bytes.find(close, p0 + open.size()));
  REQUIRE_MESSAGE(p1 != std::string::npos, "Missing </checksum> tag in output");

  const auto checksum(xml_bytes.substr(p0 + open.size(),
                                       p1 - (p0 + open.size())));
  CHECK_MESSAGE(checksum.size() == ultra::crc32::checksum_length,
                "Checksum length is not valid: '" << checksum << "'");

  for (auto c : checksum)
    CHECK_MESSAGE(std::isxdigit(static_cast<unsigned char>(c)) != 0,
                  "Checksum contains non-hex digit: '" << checksum << "'");

  const std::string zeroed(ultra::crc32::replace_checksum_value(xml_bytes));

  const auto crc(ultra::crc32::calculate(zeroed));
  CHECK(to_hex8_upper(crc) == checksum);
}


// ------------------------------
// Test data builder
// ------------------------------
[[nodiscard]] std::string make_input_xml(int runs,
                                         int elapsed,
                                         double success,
                                         double mean,
                                         double stddev,
                                         double best_fitness,
                                         double best_accuracy,
                                         int best_run,
                                         std::string_view best_code,
                                         std::initializer_list<int> solutions)
{
  // Inputs do not need a checksum tag; merge_summary parses only <summary>.
  std::ostringstream oss;
  oss << "<ultra>\n"
      << "  <summary>\n"
      << "    <runs>" << runs << "</runs>\n"
      << "    <elapsed_time>" << elapsed << "</elapsed_time>\n"
      << "    <success_rate>" << success << "</success_rate>\n"
      << "    <distributions>\n"
      << "      <fitness>\n"
      << "        <mean>" << mean << "</mean>\n"
      << "        <standard_deviation>" << stddev << "</standard_deviation>\n"
      << "      </fitness>\n"
      << "    </distributions>\n"
      << "    <best>\n"
      << "      <fitness>" << best_fitness << "</fitness>\n"
      << "      <accuracy>" << best_accuracy << "</accuracy>\n"
      << "      <run>" << best_run << "</run>\n"
      << "      <code>" << best_code << "</code>\n"
      << "    </best>\n"
      << "    <solutions>\n";
  for (int s : solutions)
    oss << "      <run>" << s << "</run>\n";
  oss << "    </solutions>\n"
      << "  </summary>\n"
      << "</ultra>\n";
  return oss.str();
}


TEST_SUITE("merge_summary")
{

TEST_CASE("merges two summaries and produces a checksum-valid output")
{
  REQUIRE_MESSAGE(fs::exists(script),
                  "Script not found at: " << script.string());

  const auto tmp(make_temp_dir());
  const auto a(tmp / "a.xml");
  const auto b(tmp / "b.xml");
  const auto out(tmp / "out.xml");
  const auto err(tmp / "err.txt");

  // A: `runs=3`.
  write_all(a, make_input_xml(/*runs*/3, /*elapsed*/10,
                              /*success*/0.5,
                              /*mean*/2.0, /*std*/1.0,
                              /*best_fitness*/5.0, /*best_accuracy*/0.90,
                              /*best_run*/1, /*best_code*/"A_CODE",
                              /*solutions*/{0, 2}));

  // B: `runs=2`, best_fitness higher => should win; `best_run` offset by
  // `A.runs`.
  write_all(b, make_input_xml(/*runs*/2, /*elapsed*/7,
                              /*success*/1.0,
                              /*mean*/4.0, /*std*/2.0,
                              /*best_fitness*/6.0, /*best_accuracy*/0.95,
                              /*best_run*/0, /*best_code*/"B_CODE",
                              /*solutions*/{1}));

  const int rc(run_cli(a, b, out, err));
  CHECK_MESSAGE(rc == 0, "CLI failed; stderr:\n" << read_all(err));

  const auto xml_bytes(read_all(out));

  // Parse with tinyxml2.
  XMLDocument doc;
  const auto parse_rc(doc.Parse(xml_bytes.c_str(), xml_bytes.size()));
  REQUIRE_MESSAGE(parse_rc == tinyxml2::XML_SUCCESS,
                  "Output XML parse failed: " << doc.ErrorStr());

  auto *ultra(doc.FirstChildElement("ultra"));
  REQUIRE(ultra != nullptr);

  auto *summary(require_child(ultra, "summary"));

  // `runs`, `elapsed_time`.
  CHECK(require_ll(require_child(summary, "runs")) == 5);
  CHECK(require_ll(require_child(summary, "elapsed_time")) == 17);

  // `success_rate = (0.5*3 + 1.0*2) / 5 = 0.7`.
  CHECK(doctest::Approx(require_double(require_child(summary, "success_rate")))
        .epsilon(1e-12) == 0.7);

  // `mean = (2*3 + 4*2)/5 = 2.8`.
  {
    auto *distributions(require_child(summary, "distributions"));
    auto *fitness(require_child(distributions, "fitness"));

    CHECK(doctest::Approx(require_double(require_child(fitness, "mean")))
          .epsilon(1e-12) == 2.8);
    // std is pooled; we don't hard-code it here, but we do require it to exist
    // and be finite.
    const double sd(require_double(require_child(fitness,
                                                 "standard_deviation")));
    CHECK(std::isfinite(sd));
    CHECK(sd >= 0.0);
  }

  // `best` comes from B; run offset by 3 => 3.
  {
    auto *best(require_child(summary, "best"));
    CHECK(doctest::Approx(require_double(require_child(best, "fitness")))
          .epsilon(1e-12) == 6.0);
    CHECK(doctest::Approx(require_double(require_child(best, "accuracy")))
          .epsilon(1e-12) == 0.95);
    CHECK(require_ll(require_child(best, "run")) == 3);
    CHECK(std::string(require_text(require_child(best, "code"))) == "B_CODE");
  }

  // Solutions merged with offset for B: [0,2] + [1+3] => [0,2,4].
  {
    const auto sol(read_solution_runs(summary));
    REQUIRE(sol.size() == 3);
    CHECK(sol[0] == 0);
    CHECK(sol[1] == 2);
    CHECK(sol[2] == 4);
  }

  // checksum validates against exact output bytes
  check_checksum_matches(xml_bytes);

  [[maybe_unused]] std::error_code ec;
  fs::remove_all(tmp, ec);
}

TEST_CASE("Fails cleanly on missing required nodes (exit code 2)")
{
  REQUIRE_MESSAGE(fs::exists(script),
                  "Script not found at: " << script.string());

  const auto tmp(make_temp_dir());
  const auto a(tmp / "a.xml");
  const auto b(tmp / "b.xml");
  const auto out(tmp / "out.xml");
  const auto err(tmp / "err.txt");

  // Missing <solutions> in A -> UltraParseError -> exit 2.
  const std::string bad(
      "<ultra><summary>"
      "<runs>1</runs>"
      "<elapsed_time>1</elapsed_time>"
      "<success_rate>1</success_rate>"
      "<distributions><fitness><mean>0</mean><standard_deviation>0</standard_deviation></fitness></distributions>"
      "<best><fitness>0</fitness><accuracy>1</accuracy><run>0</run><code>x</code></best>"
      "</summary></ultra>");
  write_all(a, bad);

  write_all(b, make_input_xml(1, 1, 1.0, 0.0, 0.0, 0.0, 1.0, 0, "OK", {0}));

  const int rc(run_cli(a, b, out, err));
  CHECK(rc == 2);

  const auto stderr_txt(read_all(err));
  CHECK_MESSAGE(stderr_txt.find("missing required node 'solutions'")
                != std::string::npos,
                "Unexpected stderr:\n" << stderr_txt);

  [[maybe_unused]] std::error_code ec;
  fs::remove_all(tmp, ec);
}

}  // TEST_SUITE
