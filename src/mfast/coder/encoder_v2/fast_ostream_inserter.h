#ifndef FAST_OSTREAM_INSERTER_H_A3C4191F
#define FAST_OSTREAM_INSERTER_H_A3C4191F

#include "mfast/int_ref.h"
#include "mfast/string_ref.h"
#include "mfast/decimal_ref.h"
#include "mfast/ext_ref.h"
#include "../encoder/fast_ostream.h"

namespace mfast
{
  namespace coder
  {
    template <typename T>
    inline fast_ostream& operator << (fast_ostream& strm, const T& ext_ref)
    {
      typename T::cref_type cref = ext_ref.get();
      strm.encode(cref.value(), cref.absent(), ext_ref.nullable());
      return strm;
    }

    template <typename Operator, typename Properties>
    inline fast_ostream& operator << (fast_ostream& strm,
                                      const ext_cref<ascii_string_cref, Operator,Properties>& ext_ref)
    {
      ascii_string_cref cref = ext_ref.get();
      strm.encode(cref.c_str(),static_cast<uint32_t>( cref.size()), cref.instruction(), ext_ref.nullable());
      return strm;
    }

    template <typename Operator, typename Properties>
    inline fast_ostream& operator << (fast_ostream& strm,
                                      const ext_cref<unicode_string_cref, Operator,Properties>& ext_ref)
    {
      unicode_string_cref cref = ext_ref.get();
      strm.encode(cref.c_str(), static_cast<uint32_t>(cref.size()), cref.instruction(), ext_ref.nullable());
      return strm;
    }

    template <typename Operator, typename Properties>
    inline fast_ostream& operator << (fast_ostream& strm,
                                      const ext_cref<byte_vector_cref, Operator,Properties>& ext_ref)
    {
      byte_vector_cref cref = ext_ref.get();
      strm.encode(cref.begin(), static_cast<uint32_t>(cref.size()), cref.instruction(), ext_ref.nullable());
      return strm;
    }

    template <typename Operator, typename Properties>
    inline fast_ostream& operator << (fast_ostream& strm,
                                      const ext_cref<decimal_cref, Operator,Properties>& ext_ref)
    {
      decimal_cref cref = ext_ref.get();
      strm.encode(cref.exponent(), cref.absent(), ext_ref.nullable());
      if (cref.present()) {
        strm.encode(cref.mantissa(), false, false_type());
      }
      return strm;
    }

  } /* coder */

} /* mfast */


#endif /* end of include guard: FAST_OSTREAM_INSERTER_H_A3C4191F */
