/**
 *  \file
 *  \remark This file is part of ULTRA.
 *
 *  \copyright Copyright (C) 2025 EOS di Manlio Morini.
 *
 *  \license
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this file,
 *  You can obtain one at http://mozilla.org/MPL/2.0/
 */

#include "utility/xml_tools.h"
#include "utility/misc.h"

#include <chrono>
#include <filesystem>
#include <functional>
#include <thread>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

namespace
{

[[nodiscard]] std::string render_single_element_with_attrs(
  const std::function<void(tinyxml2::XMLPrinter &)> &fn)
{
  tinyxml2::XMLPrinter p;
  p.OpenElement("run");
  fn(p);
  p.CloseElement();
  return std::string(p.CStr());
}

}  // namespace

TEST_SUITE("XML TOOLS")
{

TEST_CASE("xml_closer")
{
  tinyxml2::XMLPrinter p;

  const std::string base("base");
  const std::string output("Test string");

  {
    ultra::xml_closer element(p, base.c_str());
    p.PushText(output.c_str());
  }

  CHECK(ultra::trim(p.CStr())
        == "<" + base + ">" + output + "</" + base + ">");
}

TEST_CASE("Base CRC32")
{
  CHECK(ultra::crc32::calculate("The quick brown fox jumps over the lazy dog")
        == 0x414FA339);
  CHECK(ultra::crc32::calculate("123456789") == 0xcbf43926);
  CHECK(ultra::crc32::calculate("") == 0);

  const std::string empty_xml(R"(<?xml version="1.0"?>
<checksum>00000000</checksum>)");

  const std::string signed_xml(ultra::crc32::embed_xml_signature(empty_xml));
  CHECK(signed_xml != empty_xml);
  CHECK(ultra::crc32::verify_xml_signature(signed_xml));
}

TEST_CASE("CRC32 with parallel processes")
{
  using namespace ultra;
  namespace fs = std::filesystem;

  const std::string base_xml(R"(<?xml version="1.0"?>
<customers>
   <customer id="55000">
      <name>Charter Group</name>
      <address>
         <street>100 Main</street>
         <city>Framingham</city>
         <state>MA</state>
         <zip>01701</zip>
      </address>
      <address>
         <street>720 Prospect</street>
         <city>Framingham</city>
         <state>MA</state>
         <zip>01701</zip>
      </address>
      <address>
         <street>120 Ridge</street>
         <state>MA</state>
         <zip>01760</zip>
      </address>
   </customer>
   <checksum>00000000</checksum>
</customers>)");

  const std::string xml(crc32::embed_xml_signature(base_xml));

  const fs::path data_file("data.xml");

  const auto cleanup([&]
  {
    if (fs::exists(data_file))
      fs::remove(data_file);
  });

  const auto reader([&]()
  {
    unsigned reads(100);

    while (reads)
      if (std::ifstream file(data_file); file)
      {
        const std::string data((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());

        if (crc32::verify_xml_signature(data))
        {
          CHECK(data == xml);
          --reads;
        }
      }
  });

  using namespace std::chrono_literals;
  const auto writer([&]()
  {
    unsigned writes(10);

    while (writes)
    {
      // Write to the file.
      std::ofstream file(data_file);
      CHECK(file);
      file << xml;

      --writes;
      std::this_thread::sleep_for(50ms);
    }
  });

  cleanup();

  {
    std::ofstream file(data_file);
    CHECK(file);
    file << xml;
  }

  std::thread read_thread(reader);
  writer();

  read_thread.join();

  CHECK(fs::exists(data_file));

  cleanup();
}


TEST_CASE("set_text - string-like values without NUL termination")
{
  tinyxml2::XMLPrinter pr;

  // Non NUL-terminated view (substring).
  const std::string backing("hello_world");
  std::string_view sv(backing.data(), 5);  // "hello"

  ultra::set_text(pr, "greeting", sv);

  const std::string out(pr.CStr());

  CHECK(out.find("<greeting>hello</greeting>") != std::string::npos);
  CHECK(out.find("hello_world") == std::string::npos);  // no overrun
}

TEST_CASE("set_text - non-string values")
{
  tinyxml2::XMLPrinter pr;

  ultra::set_text(pr, "n", 42);

  const std::string out(pr.CStr());
  CHECK(out.find("<n>42</n>") != std::string::npos);
}

struct streamy
{
  int x {};
};

std::ostream &operator<<(std::ostream &os, const streamy &s)
{
  return os << "streamy:" << s.x;
}

TEST_CASE("set_text - operator<< for custom stream-aware types")
{
  tinyxml2::XMLPrinter pr;
  ultra::set_text(pr, "val", streamy{7});

  const std::string out(pr.CStr());
  CHECK(out.find("<val>streamy:7</val>") != std::string::npos);
}

TEST_CASE("set_attr - integral attributes, including size_t")
{
  using ultra::set_attr;

  const auto out(render_single_element_with_attrs([](tinyxml2::XMLPrinter &p)
  {
    const auto id_sz(static_cast<std::size_t>(123));
    set_attr(p, "id", id_sz);

    set_attr(p, "u32", static_cast<std::uint32_t>(7));
    set_attr(p, "i32", static_cast<std::int32_t>(-9));
    set_attr(p, "u64", static_cast<std::uint64_t>(42));
    set_attr(p, "i64", static_cast<std::int64_t>(-43));
  }));

  // Attribute order is not important; just check presence.
  CHECK(out.find("<run") != std::string::npos);
  CHECK(out.find("id=\"123\"") != std::string::npos);
  CHECK(out.find("u32=\"7\"") != std::string::npos);
  CHECK(out.find("i32=\"-9\"") != std::string::npos);
  CHECK(out.find("u64=\"42\"") != std::string::npos);
  CHECK(out.find("i64=\"-43\"") != std::string::npos);

  const bool closing(out.find("/>") != std::string::npos
                     || out.find("</run>") != std::string::npos);
  CHECK(closing);
}

TEST_CASE("set_attr - floating-point attributes")
{
  using ultra::set_attr;

  const auto out(render_single_element_with_attrs([](tinyxml2::XMLPrinter &p)
  {
    set_attr(p, "f", 1.5f);
    set_attr(p, "d", 2.25);
  }));

  // tinyxml2 prints doubles with a reasonable default formatting, but exact
  // formatting can vary (e.g. "1.5" vs "1.500000"). So check the prefix.
  CHECK(out.find("f=\"") != std::string::npos);
  CHECK(out.find("d=\"") != std::string::npos);

  // Still try to be a bit more specific without being fragile.
  CHECK(out.find("f=\"1.5") != std::string::npos);
  CHECK(out.find("d=\"2.25") != std::string::npos);
}

TEST_CASE("set_attr - string attributes")
{
  const auto out(render_single_element_with_attrs([](tinyxml2::XMLPrinter &p)
  {
    ultra::set_attr(p, "name", std::string("elite"));
    ultra::set_attr(p, "empty", std::string(""));
  }));

  CHECK(out.find("name=\"elite\"") != std::string::npos);
  CHECK(out.find("empty=\"\"") != std::string::npos);
}

TEST_CASE("set_attr - accidentally triggering cdata overload")
{
  // This test guards against an earlier mistake where passing an `int` as the
  // 2nd argument could bind to `(const char *, bool cdata)`.
  const auto out(render_single_element_with_attrs([](tinyxml2::XMLPrinter &p)
  {
    // Any integral should become a numeric overload, not `(text, bool)`.
    ultra::set_attr(p, "id", static_cast<std::size_t>(5));
  }));

  CHECK(out.find("<![CDATA[") == std::string::npos);
}

}  // TEST_SUITE
