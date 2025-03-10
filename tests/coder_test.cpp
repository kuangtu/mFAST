// Copyright (c) 2013, 2014, Huang-Ming Huang,  Object Computing, Inc.
// All rights reserved.
//
// This file is part of mFAST.
//
//     mFAST is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     mFAST is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with mFast.  If not, see <http://www.gnu.org/licenses/>.
//
#include <mfast.h>
#include <mfast/field_comparator.h>
#include <mfast/xml_parser/dynamic_templates_description.h>
#include <mfast/coder/fast_encoder.h>
#include <mfast/coder/fast_decoder.h>

#define BOOST_TEST_DYN_LINK
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>
#include <cstring>
#include <stdexcept>

#include "byte_stream.h"
#include "debug_allocator.h"

using namespace mfast;

class fast_coding_test_case
{
  public:
    fast_coding_test_case(const char* xml_content)
      : description_(xml_content)
      , encoder_(&alloc_)
      , decoder_(&alloc_)
    {
      const templates_description* descriptions[] = { &description_ };
      encoder_.include(descriptions);
      decoder_.include(descriptions);
    }

    boost::test_tools::predicate_result
    encoding(const message_cref& msg_ref, const byte_stream& result, bool reset=false)
    {
      const int buffer_size = 128;
      char buffer[buffer_size];

      std::size_t encoded_size = encoder_.encode(msg_ref,
                                                 buffer,
                                                 buffer_size,
                                                 reset);

      if (result == byte_stream(buffer, encoded_size))
        return true;

      boost::test_tools::predicate_result res( false );
      res.message() << "Got \"" << byte_stream(buffer, encoded_size) << "\" instead.";
      return res;
    }

    boost::test_tools::predicate_result
    decoding(const byte_stream& bytes, const message_cref& result, bool reset=false)
    {
      const char* first = bytes.data();
      message_cref msg = decoder_.decode(first, first+bytes.size(), reset);

      if (msg == result)
        return true;

      boost::test_tools::predicate_result res( false );
      return res;
    }

    const template_instruction* template_with_id(uint32_t id)
    {
      return encoder_.template_with_id(id);
    }

  private:
    dynamic_templates_description description_;
    debug_allocator alloc_;
    fast_encoder encoder_;
    fast_decoder decoder_;
};

BOOST_AUTO_TEST_SUITE( test_fast_coder )


BOOST_AUTO_TEST_CASE(simple_coder_test)
{
  fast_coding_test_case test_case (
    "<?xml version=\" 1.0 \"?>\n"
    "<templates xmlns=\"http://www.fixprotocol.org/ns/template-definition\" "
    "templateNs=\"http://www.fixprotocol.org/ns/templates/sample\" ns=\"http://www.fixprotocol.org/ns/fix\">\n"
    "<template name=\"Test\" id=\"1\">\n"
    "<uInt32 name=\"field1\" id=\"11\"><copy/></uInt32>\n"
    "<uInt32 name=\"field2\" id=\"12\"><copy/></uInt32>\n"
    "<uInt32 name=\"field3\" id=\"13\"><copy/></uInt32>\n"
    "</template>\n"
    "</templates>\n");

  debug_allocator alloc;
  message_type msg(&alloc, test_case.template_with_id(1));
  message_mref msg_ref = msg.mref();

  msg_ref[0].as(1);
  msg_ref[1].as(2);
  msg_ref[2].as(3);



  BOOST_CHECK(test_case.encoding(msg_ref,"\xB8\x81\x82\x83"));
  BOOST_CHECK(test_case.decoding("\xB8\x81\x82\x83", msg_ref));
}

BOOST_AUTO_TEST_CASE(group_coder_test)
{
  fast_coding_test_case test_case (
    "<?xml version=\" 1.0 \"?>\n"
    "<templates xmlns=\"http://www.fixprotocol.org/ns/template-definition\" "
    "templateNs=\"http://www.fixprotocol.org/ns/templates/sample\" ns=\"http://www.fixprotocol.org/ns/fix\">\n"
    "<template name=\"Test\" id=\"1\">\n"
    "<uInt32 name=\"field1\" id=\"11\"><copy/></uInt32>\n"
    "<group name=\"group1\" presence=\"optional\">"
    "<uInt32 name=\"field2\" id=\"12\"><copy/></uInt32>\n"
    "<uInt32 name=\"field3\" id=\"13\"><copy/></uInt32>\n"
    "</group>"
    "</template>\n"
    "</templates>\n");

  debug_allocator alloc;
  message_type msg(&alloc, test_case.template_with_id(1));
  message_mref msg_ref = msg.mref();

  msg_ref[0].as(1);
  group_mref grp(msg_ref[1]);
  grp[0].as(2);
  grp[1].as(3);

  BOOST_CHECK(test_case.encoding(msg_ref, "\xB0\x81\xE0\x82\x83"));
  BOOST_CHECK(test_case.decoding("\xB0\x81\xE0\x82\x83", msg_ref));

}

BOOST_AUTO_TEST_CASE(sequence_coder_test)
{
  fast_coding_test_case test_case (
    "<?xml version=\" 1.0 \"?>\n"
    "<templates xmlns=\"http://www.fixprotocol.org/ns/template-definition\" "
    "templateNs=\"http://www.fixprotocol.org/ns/templates/sample\" ns=\"http://www.fixprotocol.org/ns/fix\">\n"
    "<template name=\"Test\" id=\"1\">\n"
    "<uInt32 name=\"field1\" id=\"11\"><copy/></uInt32>\n"
    "<sequence name=\"sequence1\" presence=\"optional\">"
    "<uInt32 name=\"field2\" id=\"12\"><copy/></uInt32>\n"
    "<uInt32 name=\"field3\" id=\"13\"><copy/></uInt32>\n"
    "</sequence>"
    "</template>\n"
    "</templates>\n");

  debug_allocator alloc;
  message_type msg(&alloc, test_case.template_with_id(1));
  message_mref msg_ref = msg.mref();

  msg_ref[0].as(1);
  sequence_mref seq(msg_ref[1]);
  seq.resize(2);
  seq[0][0].as(2);
  seq[0][1].as(3);

  seq[1][0].as(0);
  seq[1][1].as(1);

  BOOST_CHECK(test_case.encoding(msg_ref,
                                 // pmap | f1 | s1 len| elem1 pmap | f2 | f3 | elem2 pmap |  f2 | f3 |
                                 //  A0    81    83        E0        82   83     E0          80   81
                                 "\xA0\x81\x83\xE0\x82\x83\xE0\x80\x81"
                                 ));
  BOOST_CHECK(test_case.decoding("\xA0\x81\x83\xE0\x82\x83\xE0\x80\x81", msg_ref));

}

BOOST_AUTO_TEST_CASE(static_templateref_coder_test)
{
  fast_coding_test_case test_case (
    "<?xml version=\" 1.0 \"?>\n"
    "<templates xmlns=\"http://www.fixprotocol.org/ns/template-definition\" "
    "templateNs=\"http://www.fixprotocol.org/ns/templates/sample\" ns=\"http://www.fixprotocol.org/ns/fix\">\n"
    "<template name=\"Nested\">\n"
    "<uInt32 name=\"field2\" id=\"12\"><copy/></uInt32>\n"
    "<uInt32 name=\"field3\" id=\"13\"><copy/></uInt32>\n"
    "</template>"
    "<template name=\"Unused0\">\n"
      "<uInt32 name=\"field4\" id=\"15\"><copy/></uInt32>\n"
    "</template>"
    "<template name=\"Unused1\" id=\"01\">\n"
      "<uInt32 name=\"field4\" id=\"15\"><copy/></uInt32>\n"
    "</template>"
    "<template name=\"Test\" id=\"02\">\n"
    "<uInt32 name=\"field1\" id=\"11\"><copy/></uInt32>\n"
    // "<group name=\"nested\">"
    "  <templateRef name=\"Nested\" />"
    // "</group>"
    "</template>\n"
    "</templates>\n");

  debug_allocator alloc;
  message_type msg(&alloc, test_case.template_with_id(2));

  BOOST_CHECK_EQUAL(test_case.template_with_id(2)->segment_pmap_size(), 3U);
  message_mref msg_ref = msg.mref();

  msg_ref[0].as(1);
  msg_ref[1].as(2);
  msg_ref[2].as(3);
                                 // pmap | template id | field1 | field2 | field 3 |
                                 //  F8         82        81        82       83
  BOOST_CHECK(test_case.encoding(msg_ref,"\xF8\x82\x81\x82\x83"));
  BOOST_CHECK(test_case.decoding("\xF8\x82\x81\x82\x83", msg_ref));
}


BOOST_AUTO_TEST_CASE(dynamic_templateref_coder_test)
{
  fast_coding_test_case test_case (
    "<?xml version=\" 1.0 \"?>\n"
    "<templates xmlns=\"http://www.fixprotocol.org/ns/template-definition\" "
    "templateNs=\"http://www.fixprotocol.org/ns/templates/sample\" ns=\"http://www.fixprotocol.org/ns/fix\">\n"
    "<template name=\"Nested\" id=\"01\">\n"
    "<uInt32 name=\"field2\" id=\"12\"><copy/></uInt32>\n"
    "<uInt32 name=\"field3\" id=\"13\"><copy/></uInt32>\n"
    "</template>"
    "<template name=\"Test\" id=\"02\">\n"
    "<uInt32 name=\"field1\" id=\"11\"><copy/></uInt32>\n"
    "<group name=\"nested\">"
    "  <templateRef/>"
    "</group>"
    "</template>\n"
    "</templates>\n");

  debug_allocator alloc;
  message_type msg(&alloc, test_case.template_with_id(2));
  message_mref msg_ref = msg.mref();


  msg_ref[0].as(1);
  nested_message_mref nested(static_cast<group_mref>(msg_ref[1])[0]);

  message_mref target = nested.rebind(test_case.template_with_id(1));


  target[0].as(2);
  target[1].as(3);

  BOOST_CHECK(test_case.encoding(msg_ref,"\xE0\x82\x81\xF0\x81\x82\x83"));
  BOOST_CHECK(test_case.decoding("\xE0\x82\x81\xF0\x81\x82\x83", msg_ref));
}

BOOST_AUTO_TEST_CASE(manual_reset_test)
{
  fast_coding_test_case test_case (
    "<?xml version=\" 1.0 \"?>\n"
    "<templates xmlns=\"http://www.fixprotocol.org/ns/template-definition\" "
    "templateNs=\"http://www.fixprotocol.org/ns/templates/sample\" ns=\"http://www.fixprotocol.org/ns/fix\">\n"
    "<template name=\"Test\"  id=\"1\">\n"
    "<uInt32 name=\"field1\" id=\"11\"><copy value=\"11\"/></uInt32>\n"
    "<uInt32 name=\"field2\" id=\"12\"><copy value=\"12\"/></uInt32>\n"
    "<uInt32 name=\"field3\" id=\"13\"><copy value=\"13\"/></uInt32>\n"
    "</template>\n"
    "</templates>\n");

  debug_allocator alloc;
  message_type msg(&alloc, test_case.template_with_id(1));
  message_mref msg_ref = msg.mref();

  msg_ref[0].as(1);
  msg_ref[1].as(2);
  msg_ref[2].as(3);

  BOOST_CHECK(test_case.encoding(msg_ref, "\xB8\x81\x82\x83", false));
  BOOST_CHECK(test_case.decoding("\xB8\x81\x82\x83", msg_ref, false));


  // message not changed
  BOOST_CHECK(test_case.encoding(msg_ref, "\x80", false));
  BOOST_CHECK(test_case.decoding("\x80", msg_ref, false));

  msg_ref[0].as(11);
  msg_ref[1].as(12);
  msg_ref[2].as(13);

  uint32_cref f0(msg_ref[0]);
  BOOST_REQUIRE(!f0.absent());
  BOOST_REQUIRE(!f0.instruction()->initial_value().is_empty());

  BOOST_REQUIRE(f0.is_initial_value());

  BOOST_REQUIRE(f0.is_initial_value());
  uint32_cref f1(msg_ref[1]);
  BOOST_REQUIRE(f1.is_initial_value());
  uint32_cref f2(msg_ref[2]);
  BOOST_REQUIRE(f2.is_initial_value());

  // encoding with reset, all values are initial
  BOOST_CHECK(test_case.encoding(msg_ref, "\x80", true));
  BOOST_CHECK(test_case.decoding("\x80", msg_ref, true));

}

BOOST_AUTO_TEST_CASE(auto_reset_coder_test)
{
  fast_coding_test_case test_case (
    "<?xml version=\" 1.0 \"?>\n"
    "<templates xmlns=\"http://www.fixprotocol.org/ns/template-definition\" "
    "templateNs=\"http://www.fixprotocol.org/ns/templates/sample\" ns=\"http://www.fixprotocol.org/ns/fix\">\n"
    "<template name=\"Test\" id=\"1\" scp:reset=\"yes\">\n"
    "<uInt32 name=\"field1\" id=\"11\"><copy value=\"11\"/></uInt32>\n"
    "<uInt32 name=\"field2\" id=\"12\"><copy value=\"12\"/></uInt32>\n"
    "<uInt32 name=\"field3\" id=\"13\"><copy value=\"13\"/></uInt32>\n"
    "</template>\n"
    "</templates>\n");

  debug_allocator alloc;
  message_type msg(&alloc, test_case.template_with_id(1));
  message_mref msg_ref = msg.mref();

  msg_ref[0].as(1);
  msg_ref[1].as(2);
  msg_ref[2].as(3);

  BOOST_CHECK(test_case.encoding(msg_ref, "\xB8\x81\x82\x83"));
  BOOST_CHECK(test_case.decoding("\xB8\x81\x82\x83", msg_ref));

  msg_ref[0].as(11);
  msg_ref[1].as(12);
  msg_ref[2].as(13);
  // encoding with reset, all values are initial
  BOOST_CHECK(test_case.encoding(msg_ref, "\x80"));
  BOOST_CHECK(test_case.decoding("\x80", msg_ref));

  BOOST_CHECK(test_case.encoding(msg_ref, "\x80"));
  BOOST_CHECK(test_case.decoding("\x80", msg_ref));
}


BOOST_AUTO_TEST_SUITE_END()
