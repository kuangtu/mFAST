
#include "json.h"

namespace mfast {

  namespace json {

    struct tag_reason;
    struct tag_got;
    struct tag_error_occurs_before;

    struct error_occurs_before_info
      : public boost::error_info<tag_error_occurs_before, std::string>
    {
      error_occurs_before_info(std::istream& is)
        : boost::error_info<tag_error_occurs_before, std::string>("")
      {
        std::streambuf* buf = is.rdbuf();
        char unconsumed[127];
        std::streamsize n = buf->sgetn( unconsumed, 127 );
        this->value().assign(unconsumed, n);
      }

    };

    typedef boost::error_info<tag_reason, std::string> reason_info;
    typedef boost::error_info<tag_got, std::string> got_info;


    class json_decode_error
      : public std::runtime_error
      , public boost::exception
    {
    public:
      json_decode_error(std::istream& is, const char* reason, const char* got=0)
        : std::runtime_error(reason)
      {
        if (got)
          *this << got_info(got);
        *this << error_occurs_before_info(is);
      }

      json_decode_error(std::istream& is, const char* reason, char got)
        : std::runtime_error(reason)
      {
        if (got) {
          char buf[2]= {'\0','\0'};
          buf[0] =  got;
          *this << got_info(buf);
        }
        *this << error_occurs_before_info(is);
      }

    };


    bool get_quoted_string(std::istream&                  strm,
                           std::string*                   pstr,
                           const mfast::byte_vector_mref* pref,
                           bool                           first_quote_extracted=false)
    {
      std::streambuf* buf = strm.rdbuf();

      char c;
      if (!first_quote_extracted) {
        // make sure the first non-whitespace character is a quote

        strm >> std::skipws >> c;

        if (c != '"') {
          strm.setstate(std::ios::failbit);
        }

        if (!strm)
          BOOST_THROW_EXCEPTION(json_decode_error(strm, "No opening quotation mark"));

        if (pref)
          pref->push_back(c);
      }

      if (pstr)
        pstr->clear();

      bool escaped = false;

      do {
        c = buf->sbumpc();

        if (pref)
          pref->push_back(c);

        if (c != EOF) {
          if (!escaped) {
            if (c=='\\') {
              escaped = true;
              continue;
            }
            else if (c == '"') {
              return true;
            }
          }
          else {
            // escape mode
            const char* escapables="\b\t\n\f\r\"/\\";
            const char* orig="btnfr\"/\\";
            const char* pch = strchr(orig, c);
            if (pch != 0) {
              c = escapables[pch-orig];
              escaped = false;
            }
            else if (c == 'u' ){
              // This is a unicode character
              // we need to read 4 hexadecimal digits
              uint32_t val = 0;
              char buf[5]={'\x0'};

              if (!strm.read(buf, 4))
                BOOST_THROW_EXCEPTION(json_decode_error(strm, "Not a valid unicode character"));

              escaped = false;

              if (pref) {
                pref->insert(pref->end(), buf, buf+4);
              }

              if (!pstr)
                continue;

              for (int i = 0; i < 4; ++i) {
                val <<=4;
                if (buf[i] >= '0' && buf[i] <= '9')
                  val |= buf[i] - '0';
                else if (buf[i] >= 'a' && buf[i] <= 'f')
                  val |= buf[i] - 'a' + 10;
                else if (buf[i] >= 'A' && buf[i] <= 'F')
                  val |= buf[i] - 'A' + 10;
                else {
                  BOOST_THROW_EXCEPTION(json_decode_error(strm, "Not a valid unicode character"));
                }
              }

              // now we need convert the unicode character to UTF8
              if (val < 0x7F)
                (*pstr) += static_cast<char>(val);
              else if (val < 0x07FF) {
                // 2 bytes encoding
                (*pstr) +=  static_cast<char>((val >> 6) | 0xC0);
                (*pstr) +=  static_cast<char>( (val & 0x3F) | 0x80);
              }
              else {
                // 3 bytes encoding
                (*pstr) +=  static_cast<char>((val >> 12) | 0xE0);
                (*pstr) +=  static_cast<char>(((val >> 6) & 0x3F) | 0x80);
                (*pstr) +=  static_cast<char>((val & 0x3F) | 0x80);
              }
              continue;

            }
            else {
              strm.setstate(std::ios::failbit);
              BOOST_THROW_EXCEPTION(json_decode_error(strm, "Not a valid escape character"));
            }
          }
          if (pstr)
            (*pstr) += static_cast<char>(c);
        }
        else {
          strm.setstate(std::ios::failbit);
          BOOST_THROW_EXCEPTION(json_decode_error(strm, "Expect closing quotation mark"));
        }
      }
      while (1);
      strm.setstate(std::ios::failbit);
      return false;
    }

    bool skip_matching(std::istream&                  strm,
                       char                           left_bracket,
                       const mfast::byte_vector_mref* pref)
    {
      assert(left_bracket == '{' || left_bracket == '[');

      char right_bracket;

      if (left_bracket == '{')
        right_bracket = '}';
      else // if (left_bracket == '[')
        right_bracket = ']';

      std::streambuf* buf = strm.rdbuf();
      int c;
      std::size_t count=1;

      while ( (c = buf->sbumpc()) != EOF )
      {
        if (pref)
          pref->push_back(c);
        if (c == left_bracket)
          ++count;
        else if (c == right_bracket) {
          if ( --count == 0 )
            return true;
        }
        else if (c == '"') {
          if (!get_quoted_string(strm, 0, pref, true))
            return false;
        }
      }
      BOOST_THROW_EXCEPTION(json_decode_error(strm, "Expect closing bracket"));
    }

    bool skip_value (std::istream& strm)
    {
      char c1;
      char rest[5];

      strm >> std::skipws >> c1;
      if (c1 == '{' || c1 == '[')
        return skip_matching(strm, c1, 0);
      else if (c1 == '"')
        return get_quoted_string(strm, 0, 0, true);
      else if (c1 == 'n') {
        // check if it's null
        strm.get(rest, 4);
        if (strcmp(rest, "ull")==0)
          return true;
      }
      else if (c1=='t') {
        // check if it's "true"
        strm.get(rest, 4);
        if (strcmp(rest, "rue")==0)
          return true;
      }
      else if (c1 == 'f') {
        // check if it's "false"
        strm.get(rest, 5);
        if (strcmp(rest, "alse")==0)
          return true;
      }
      else if (c1 == '-' || isdigit(c1)) {
        // skip number
        strm.putback(c1);
        double d;
        strm >> d;
        return strm.good();
      }
      strm.setstate(std::ios::failbit);
      BOOST_THROW_EXCEPTION(json_decode_error(strm, "Unknown value"));
      return false;

    }


    bool parse_array_preamble(std::istream& strm)
    {
      char c;
      strm >> std::skipws >> c;
      if (!strm.good() || c != '[') {
        strm.setstate(std::ios::failbit);
        BOOST_THROW_EXCEPTION(json_decode_error(strm, "Expect [ for an array"));
      }

      strm >> std::ws;
      if (strm.peek() == ']') {
        strm >> c;
        return false;
      }
      return true;
    }

    struct decode_visitor
    {
      std::istream& strm_;
      unsigned json_object_tag_mask_;

      enum {
        visit_absent = true
      };

      decode_visitor(std::istream& strm,
                     unsigned      json_object_tag_mask)
        : strm_(strm)
        , json_object_tag_mask_(json_object_tag_mask)
      {
      }

      template <typename NumericTypeMref>
      void visit(const NumericTypeMref& ref)
      {
        typedef typename NumericTypeMref::value_type value_type;
        value_type value;
        if (strm_ >> value, strm_.good())
          ref.as(value);
        else
          BOOST_THROW_EXCEPTION(json_decode_error(strm_, "Expect number value"));
      }

      void visit(const enum_mref& ref)
      {
        if (ref.is_boolean()) {
          char c1;
          char rest[5];

          strm_ >> std::skipws >> c1;

          switch (c1)
          {
          case '0':
            ref.as(false);
            return;
          case '1':
            ref.as(true);
            return;
          case 'f':
            strm_.get(rest, 5);
            if (strcmp(rest, "alse")==0) {
              ref.as(false);
              return;
            }
            break;
          case 't':
            strm_.get(rest, 4);
            if (strcmp(rest, "rue")==0) {
              ref.as(true);
              return;
            }
            break;
          }

          BOOST_THROW_EXCEPTION(json_decode_error(strm_,"Expect boolean value", c1));
        }
        else {
          // treat it is an integer
          this->visit(reinterpret_cast<const uint64_mref&>(ref));
        }
      }

      void visit(const mfast::decimal_mref& ref)
      {
        strm_ >> *reinterpret_cast<decimal_value_storage*>(field_mref_core_access::storage_of(ref));

        if (!strm_) {
          BOOST_THROW_EXCEPTION(json_decode_error(strm_,"Expect decimal value"));
        }

        ref.normalize();
      }

      template <typename Char>
      void visit(const mfast::string_mref<Char>& ref)
      {
        std::string str;
        if (get_quoted_string(strm_, &str, 0)) {
          ref.as(str);
        }
      }

      void visit(const mfast::byte_vector_mref& ref)
      {
        if (ref.instruction()->tag().to_uint64() & json_object_tag_mask_) {
          // if the json_object_tag_mask is on, that means the field shouldn't be unpakced
          char c;
          strm_ >> std::skipws >> c;
          if (strm_.good() && c == '{') {
            ref.push_back(c);
            if (skip_matching(strm_, c, &ref)) {
              return;
            }
          }
          strm_.setstate(std::ios::failbit);
          BOOST_THROW_EXCEPTION(json_decode_error(strm_, "Expect JSON object"));
        }
        else {
          // write it as a hex string
          std::string str;
          if (get_quoted_string(strm_, &str, 0)) {
            ref.resize(str.size()/2+1);
            ptrdiff_t len = byte_vector_field_instruction::hex2binary(str.c_str(), ref.data());
            ref.resize(len);
          }
        }
      }

      template <typename IntType>
      void visit(int_vector_mref<IntType> &ref)
      {
        ref.clear();
        if (!parse_array_preamble(strm_))
          return;

        std::size_t i = 0;
        char c;

        do {
          ref.resize(i + 1);

          strm_ >> ref[i] >> std::skipws >> c;

          if (!strm_.good())
            BOOST_THROW_EXCEPTION(json_decode_error(strm_, "Expect integer"));
        }
        while (c == ',');

        if (c == ']')
          return;

        strm_.setstate(std::ios::failbit);
        BOOST_THROW_EXCEPTION(json_decode_error(strm_, "Expect ] or ,"));
      }

      // return false only when the parsed result is empty or error
      bool visit_impl(const mfast::aggregate_mref& ref);

      void visit(const mfast::sequence_mref& ref, int);

      void visit(const mfast::sequence_element_mref& ref, int)
      {
        ref.accept_mutator(*this);
      }

      void visit(const mfast::group_mref& ref, int)
      {
        if (!visit_impl(ref))
          ref.omit();
      }

      void visit(const mfast::nested_message_mref&, int)
      {
        // unsupported;
      }

    };


    // return false only parsed failure
    bool decode_visitor::visit_impl(const mfast::aggregate_mref& ref)
    {

      for (std::size_t i = 0; i < ref.num_fields(); ++i)
      {
        ref[i].omit();
      }

      char c;
      strm_ >> std::skipws >> c;
      if (strm_.good() && c == '{') {
        // strm_ >> std::skipws >> c;

        if (strm_.peek() == '}') {
          strm_ >> c;
          return true;
        }

        do {
          std::string key;
          if (!get_quoted_string(strm_, &key, 0))
            BOOST_THROW_EXCEPTION(json_decode_error(strm_,"Invalid field name"));

          strm_ >> c;

          if (!strm_.good() || c != ':')
          {
            strm_.setstate(std::ios::failbit);
            BOOST_THROW_EXCEPTION(json_decode_error(strm_,"Expect :", c));
          }

          int index = ref.field_index_with_name(key.c_str());
          if (index != -1) {
            // we need to check if the value is null
            strm_ >> std::ws;
            if (strm_.good() && strm_.peek() == 'n') {
              char buf[5];
              strm_ >> std::noskipws >> std::setw(5) >> buf >> std::skipws;
              if (strncmp(buf, "null", 4) != 0) {
                strm_.setstate(std::ios::failbit);
                BOOST_THROW_EXCEPTION(json_decode_error(strm_,"Expect null", buf));
              }
            }
            else
              ref[index].accept_mutator(*this);
          }
          else {
            // the field is unkown to us,
            skip_value(strm_);
          }

          if (!strm_.good() )
            BOOST_THROW_EXCEPTION(json_decode_error(strm_,"Expect }"));

          strm_ >> c;

        } while (strm_.good() && c == ',');

        if (strm_.good() && c == '}') {
          return true;
        }
      }
      else if (c == 'n') {
        // check if the result is null
        char buf[4];
        strm_ >> std::noskipws >> std::setw(3) >> buf >> std::skipws;
        if (strncmp(buf, "ull", 3) == 0)
          return true;
      }
      strm_.setstate(std::ios::failbit);
      BOOST_THROW_EXCEPTION(json_decode_error(strm_,"Expect {", c));
    }

    void decode_visitor::visit(const mfast::sequence_mref& ref, int)
    {
      ref.clear();
      if (!parse_array_preamble(strm_))
        return;

      std::size_t i = 0;
      char c;

      do {
        ref.resize(i + 1);
        sequence_element_mref element = ref[i++];
        if (ref.element_unnamed())
          element.accept_mutator(*this);
        else
          this->visit_impl(element);

        strm_ >> std::skipws >> c;

        if (!strm_.good())
          return;
      }
      while (c == ',');

      if (c == ']')
        return;
      strm_.setstate(std::ios::failbit);
      BOOST_THROW_EXCEPTION(json_decode_error(strm_,"Expect ]", c));
    }

    void decode(std::istream&                is,
                const mfast::aggregate_mref& msg,
                unsigned                     json_object_tag_mask)
    {
      decode_visitor visitor(is, json_object_tag_mask);
      visitor.visit_impl(msg);
    }

    void decode(std::istream&               is,
                const mfast::sequence_mref& seq,
                unsigned                    json_object_tag_mask)
    {
      decode_visitor visitor(is, json_object_tag_mask);
      visitor.visit(seq, 0);
    }

  }
}
