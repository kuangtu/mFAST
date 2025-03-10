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

#include <sstream>
#include "mfast.h"
#include <mfast/xml_parser/dynamic_templates_description.h>
#include "test5.h"

#define BOOST_TEST_DYN_LINK
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>
#include "debug_allocator.h"
#define MULTILINE(...) #__VA_ARGS__

const char* xml_content= MULTILINE(
<?xml version="1.0"?>
<templates xmlns="http://www.fixprotocol.org/ns/template-definition" templateNs="view_test" ns="http://www.ociweb.com/ns/mfast">
    <template name="LoginAccount">
        <string name="userName" />
        <string name="password" />
    </template>

    <template name="BankAccount">
        <uInt64 name="number" />
        <uInt64 name="routingNumber" />
        <string name="bank" presence="optional"/>
        <string name="alias" presence="optional"/>
    </template>

    <template name="Person" >
        <string name="firstName" />
        <string name="lastName" />
        <uInt32 name="age" />
        <group name="address" presence="optional">
            <string name="streetAddress" />
            <string name="city" />
            <string name="state" />
            <uInt32 name="postalCode" />
        </group>
        <sequence name="phoneNumbers">
            <string name="type" />
            <string name="number" />
        </sequence>
        <sequence name="emails">
            <string />
        </sequence>
        <group name="login" presence="optional">
            <templateRef name="LoginAccount"/>
        </group>
        <sequence name="bankAccounts" presence="optional">
            <templateRef name="BankAccount"/>
        </sequence>
    </template>
    <view name="PersonView" reference="Person">
      <field name="lastName"> <reference name="lastName" /> </field>
      <field name="firstName"> <reference name="firstName" /> </field>
      <field name="postalCode"> <reference name="address.postalCode" /> </field>
      <field name="city"> <reference name="address.city" /> </field>
      <field name="phoneNumber"> <reference name="phoneNumbers[0].number" /> </field>
      <field name="userName">
        <reference name="login.userName" />
        <reference name="firstName" />
      </field>
    </view>
</templates>
);


using namespace mfast;


struct simple_visitor
{
  std::ostream& os_;

  enum {
    visit_absent=true
  };

  simple_visitor(std::ostream& os)
    : os_(os)
  {
  }

  template <typename CRef>
  void visit(const CRef& ref)
  {
    os_ << ref.value() << "\t";
  }

  void visit(const byte_vector_cref&)
  {
  }

  template <typename IntType>
  void visit(const int_vector_cref<IntType>&)
  {
    // matches int32_vector_cref, uint32_vector_cref, int64_vector_cref, uint64_vector_cref
  }

  template <typename Composite>
  void visit(const Composite&, int)
  {
  }
};

BOOST_AUTO_TEST_SUITE( view_test_suite )

BOOST_AUTO_TEST_CASE(parse_view_test)
{
  try {
    dynamic_templates_description description(xml_content);
    BOOST_REQUIRE_EQUAL(description.view_infos().size(), 1U);

    const mfast::aggregate_view_info& info = description.view_infos()[0];
    BOOST_CHECK_EQUAL( boost::string_ref(info.name_) , boost::string_ref("PersonView") );
    BOOST_CHECK_EQUAL( info.data_.size(), 8U);

    BOOST_CHECK_EQUAL( info.data_[0].cont(),false);
    BOOST_CHECK_EQUAL( info.data_[0].prefix_diff(), 1U);
    BOOST_CHECK_EQUAL( info.data_[0].nest_indices[0], 1);
    BOOST_CHECK_EQUAL( info.data_[0].nest_indices[1], -1);

    BOOST_CHECK_EQUAL( info.data_[1].cont(),false);
    BOOST_CHECK_EQUAL( info.data_[1].prefix_diff(), 1U);
    BOOST_CHECK_EQUAL( info.data_[1].nest_indices[0], 0);
    BOOST_CHECK_EQUAL( info.data_[1].nest_indices[1], -1);

    BOOST_CHECK_EQUAL( info.data_[2].cont(),false);
    BOOST_CHECK_EQUAL( info.data_[2].prefix_diff(), 1U);
    BOOST_CHECK_EQUAL( info.data_[2].nest_indices[0], 3);
    BOOST_CHECK_EQUAL( info.data_[2].nest_indices[1], 3);
    BOOST_CHECK_EQUAL( info.data_[2].nest_indices[2], -1);

    BOOST_CHECK_EQUAL( info.data_[3].cont(),false);
    BOOST_CHECK_EQUAL( info.data_[3].prefix_diff(), 2U);
    BOOST_CHECK_EQUAL( info.data_[3].nest_indices[0], 3);
    BOOST_CHECK_EQUAL( info.data_[3].nest_indices[1], 1);
    BOOST_CHECK_EQUAL( info.data_[3].nest_indices[2], -1);

    BOOST_CHECK_EQUAL( info.data_[4].cont(),false);
    BOOST_CHECK_EQUAL( info.data_[4].prefix_diff(), 1U);
    BOOST_CHECK_EQUAL( info.data_[4].nest_indices[0], 4);
    BOOST_CHECK_EQUAL( info.data_[4].nest_indices[1], 0);
    BOOST_CHECK_EQUAL( info.data_[4].nest_indices[2], 1);
    BOOST_CHECK_EQUAL( info.data_[4].nest_indices[3], -1);

    BOOST_CHECK_EQUAL( info.data_[5].cont(), true);
    BOOST_CHECK_EQUAL( info.data_[5].prefix_diff(), 1U);
    BOOST_CHECK_EQUAL( info.data_[5].nest_indices[0], 6);
    BOOST_CHECK_EQUAL( info.data_[5].nest_indices[1], 0);
    BOOST_CHECK_EQUAL( info.data_[5].nest_indices[2], -1);

    BOOST_CHECK_EQUAL( info.data_[6].cont(), false);
    BOOST_CHECK_EQUAL( info.data_[6].prefix_diff(), 1U);
    BOOST_CHECK_EQUAL( info.data_[6].nest_indices[0], 0);
    BOOST_CHECK_EQUAL( info.data_[6].nest_indices[1], -1);

    BOOST_CHECK_EQUAL( info.data_[7].cont(), false);
    BOOST_CHECK_EQUAL( info.data_[7].nest_indices, (int64_t*) 0);
  }
  catch(boost::exception& e)
  {
    std::cerr << diagnostic_information(e);
  }
}

BOOST_AUTO_TEST_CASE(codegen_view_test)
{
  using namespace test5;

  Person person_holder;
  Person_mref person = person_holder.mref();
  person.set_firstName().as("John");
  person.set_lastName().as("Doe");
  person.set_age().as(5);
  person.set_address().set_streetAddress().as("Woodcrest Executive Dr.");
  person.set_address().set_city().as("St. Louis");
  person.set_address().set_state().as("MO");
  person.set_address().set_postalCode().as(63141);

  person.set_phoneNumbers().resize(1);
  person.set_phoneNumbers()[0].set_number().as("1234567");
  person.set_login().set_userName().as("John123");

  PersonView view(person_holder.cref());

  std::stringstream strm;

  simple_visitor visitor(strm);

  view.accept_accessor(visitor);

  BOOST_CHECK_EQUAL( strm.str(), std::string("Doe\tJohn\t63141\tSt. Louis\t1234567\tJohn123\t") );

}

BOOST_AUTO_TEST_SUITE_END()
