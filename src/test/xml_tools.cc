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

#include <chrono>
#include <filesystem>
#include <thread>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "third_party/doctest/doctest.h"

TEST_SUITE("XML TOOLS")
{

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

  const auto cleanup([&]()
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

}  // TEST_SUITE("XML TOOLS")
