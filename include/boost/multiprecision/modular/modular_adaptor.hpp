//---------------------------------------------------------------------------//
// Copyright (c) 2020 Mikhail Komarov <nemo@nil.foundation>
// Copyright (c) 2019 Alexey Moskvin
// Copyright (c) 2020 Ilias Khairullin <ilias@nil.foundation>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//---------------------------------------------------------------------------//

#ifndef BOOST_MULTIPRECISION_MODULAR_ADAPTOR_HPP
#define BOOST_MULTIPRECISION_MODULAR_ADAPTOR_HPP

#include <boost/cstdint.hpp>
#include <boost/multiprecision/detail/digits.hpp>
#include <boost/functional/hash_fwd.hpp>
#include <boost/multiprecision/number.hpp>

#include <boost/multiprecision/modular/modular_params.hpp>
#include <boost/container/small_vector.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

namespace boost {
namespace multiprecision {
namespace backends {

template <typename Backend>
struct modular_adaptor
{
   typedef Backend value_type;
   typedef modular_params<Backend> modulus_type;

 protected:
   value_type m_base;
   modulus_type m_mod;

 public:
   inline Backend& base_data() { return m_base; }

   inline Backend const& base_data() const { return m_base; }

   inline modular_params<Backend>& mod_data() { return m_mod; }

   inline const modular_params<Backend>& mod_data() const { return m_mod; }

   typedef typename Backend::signed_types   signed_types;
   typedef typename Backend::unsigned_types unsigned_types;

   modular_adaptor() {}

   modular_adaptor(const modular_adaptor& o) : m_base(o.base_data()), m_mod(o.mod_data())
   {
   }

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

   modular_adaptor(modular_adaptor&& o)
       : m_base(std::move(o.base_data())), m_mod(std::move(o.mod_data()))
   {}

#endif
   // TODO: implement create_internal_representation
   // modular_adaptor(const Backend& val, const modular_params<Backend>& mod) : m_mod(mod), m_base(mod.create_internal_representation(val))
   // {
   //
   // }

   modular_adaptor(const Backend& val, const Backend& mod)
       : m_base(val), m_mod(mod)
   {
   }

   modular_adaptor(Backend& val, Backend& mod)
       : m_base(val), m_mod(mod) {}

   modular_adaptor(const Backend& val) : m_base(val), m_mod(typename mpl::front<unsigned_types>::type(0u))
   {
   }

   modular_adaptor(const modular_params<Backend>& mod) : m_base(typename mpl::front<unsigned_types>::type(0u)), m_mod(mod)
   {
   }

   modular_adaptor& operator=(const modular_adaptor& o)
   {
      m_base = o.base_data();
      m_mod = o.mod_data();
      return *this;
   }

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

   modular_adaptor& operator=(modular_adaptor&& o)

   BOOST_NOEXCEPT
   {
      m_base = std::move(o.base_data());
      m_mod  = std::move(o.mod_data());
      return *this;
   }
#endif

   modular_adaptor& operator=(const char* s)
   {
      typedef typename mpl::front<unsigned_types>::type ui_type;
      ui_type                                           zero = 0u;

      using default_ops::eval_fpclassify;

      if (s && (*s == '('))
      {
         std::string part;
         const char* p = ++s;
         while (*p && (*p != ',') && (*p != ')'))
            ++p;
         part.assign(s, p);
         if (!part.empty())
            m_base() = part.c_str();
         else
            m_base() = zero;
         s = p;
         if (*p && (*p != ')'))
         {
            ++p;
            while (*p && (*p != ')'))
               ++p;
            part.assign(s + 1, p);
         }
         else
            part.erase();
         if (!part.empty())
            m_mod() = part.c_str();
         else
            m_mod() = zero;
      }
      else
      {
         base_data() = s;
         m_mod()     = zero;
      }
      return *this;
   }

   int compare(const modular_adaptor& o) const
   {
      // They are either equal or not:<
      if (m_mod.compare(o.mod_data()) != 0)
      {
         BOOST_THROW_EXCEPTION(std::runtime_error("Could not compare modular number with different mod."));
      }
      Backend tmp1, tmp2;
      mod_data().adjust_regular(tmp1, base_data());
      mod_data().adjust_regular(tmp2, o.base_data());
      return tmp1.compare(tmp2);
   }

   template <class T>
   int compare(const T& val) const
   {
      using default_ops::eval_lt;
      if (!eval_lt(m_mod, val))
      {
         BOOST_THROW_EXCEPTION(std::runtime_error("Could not compare modular number with mod bigger than compared number."));
      }
      Backend tmp;
      mod_data().adjust_regular(tmp, base_data());
      return tmp.compare(val);
   }

   inline void swap(modular_adaptor& o)
   {
      base_data().swap(o.base_data());
      std::swap(mod_data(), o.mod_data());
   }

   inline std::string str(std::streamsize dig, std::ios_base::fmtflags f) const
   {
      Backend tmp;
      mod_data().adjust_regular(tmp, base_data());
      return tmp.str(dig, f);
   }

   inline void negate()
   {
      base_data().negate();
      eval_add(base_data(), mod_data().get_mod().backend());
   }

   template <typename BackendT, expression_template_option ExpressionTemplates>
   operator number<BackendT, ExpressionTemplates>()
   {
      return base_data();
   };
};

// fixed precision modular backend which supports compile-time execution
template<unsigned MinBits, cpp_integer_type SignType, cpp_int_check_type Checked>
class modular_adaptor<cpp_int_backend<MinBits, MinBits, SignType, Checked, void>>
    : modular_policy<cpp_int_backend<MinBits, MinBits, SignType, Checked, void>>
{
   typedef modular_policy<cpp_int_backend<MinBits, MinBits, SignType, Checked, void>> policy_type;
   typedef modular_adaptor<cpp_int_backend<MinBits, MinBits, SignType, Checked, void>> self_type;
   typedef modular_params<cpp_int_backend<MinBits, MinBits, SignType, Checked, void>> modular_params_type;

   typedef typename policy_type::Backend Backend;
   typedef typename policy_type::Backend_doubled_limbs Backend_doubled_limbs;
   typedef typename policy_type::number_type number_type;

 public:
   typedef Backend_doubled_limbs value_type;
   typedef modular_params_type modulus_type;

 protected:
   value_type m_base;
   modulus_type m_mod;

 public:
   constexpr auto& base_data() { return m_base; }

   constexpr const auto& base_data() const { return m_base; }

   constexpr auto& mod_data() { return m_mod; }

   constexpr const auto& mod_data() const { return m_mod; }

   typedef typename Backend::signed_types   signed_types;
   typedef typename Backend::unsigned_types unsigned_types;

   constexpr modular_adaptor() {}

   constexpr modular_adaptor(const self_type& o)
       : m_base(o.base_data()), m_mod(o.mod_data())
   {}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

   constexpr modular_adaptor(self_type&& o)
       : m_base(std::move(o.base_data())), m_mod(std::move(o.mod_data()))
   {}

#endif
   // TODO: implement create_internal_representation
   // constexpr modular_adaptor(const Backend& val, const modular_params<Backend>& mod) : m_mod(mod), m_base(mod.create_internal_representation(val))
   // {
   //
   // }
   template<typename BackendT>
   constexpr modular_adaptor(const BackendT& cb, const modular_params_type& m)
       : m_base(static_cast<Backend_doubled_limbs>(cb)), m_mod(m)
   {
      mod_data().adjust_modular(base_data());
   }

   template<typename BackendT>
   constexpr modular_adaptor(const BackendT& cb, const Backend& m)
       : m_base(static_cast<Backend_doubled_limbs>(cb)), m_mod(m)
   {
      mod_data().adjust_modular(base_data());
   }

   constexpr modular_adaptor(const modular_params_type& m)
       : m_base(static_cast<typename mpl::front<unsigned_types>::type>(0u)), m_mod(m)
   {
      mod_data().adjust_modular(base_data());
   }

   constexpr modular_adaptor& operator=(const self_type& o)
   {
      m_base = o.base_data();
      m_mod = o.mod_data();
      return *this;
   }

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

   constexpr modular_adaptor& operator=(self_type&& o)

       BOOST_NOEXCEPT
   {
      m_base = std::move(o.base_data());
      m_mod  = std::move(o.mod_data());
      return *this;
   }
#endif

   // TODO: check correctness of the method
   modular_adaptor& operator=(const char* s)
   {
      typedef typename mpl::front<unsigned_types>::type ui_type;
      ui_type                                           zero = 0u;

      using default_ops::eval_fpclassify;

      if (s && (*s == '('))
      {
         std::string part;
         const char* p = ++s;
         while (*p && (*p != ',') && (*p != ')'))
            ++p;
         part.assign(s, p);
         if (!part.empty())
            m_base() = part.c_str();
         else
            m_base() = zero;
         s = p;
         if (*p && (*p != ')'))
         {
            ++p;
            while (*p && (*p != ')'))
               ++p;
            part.assign(s + 1, p);
         }
         else
            part.erase();
         if (!part.empty())
            m_mod() = part.c_str();
         else
            m_mod() = zero;
      }
      else
      {
         base_data() = s;
         m_mod()     = zero;
      }
      return *this;
   }

   // TODO: remove exception throwing
   constexpr int compare(const self_type& o) const
   {
      // They are either equal or not:<
      if (m_mod.compare(o.mod_data()) != 0)
      {
         BOOST_THROW_EXCEPTION(std::runtime_error("Could not compare modular number with different mod."));
      }
      auto tmp1 = base_data();
      auto tmp2 = o.base_data();
      mod_data().adjust_regular(tmp1, base_data());
      mod_data().adjust_regular(tmp2, o.base_data());
      return tmp1.compare(tmp2);
   }

   // TODO: remove exception throwing
   template <class T>
   constexpr int compare(const T& val) const
   {
      using default_ops::eval_lt;
      if (!eval_lt(m_mod, val))
      {
         BOOST_THROW_EXCEPTION(std::runtime_error("Could not compare modular number with mod bigger than compared number."));
      }
      value_type tmp;
      mod_data().adjust_regular(tmp, base_data());
      return tmp.compare(val);
   }

   constexpr void swap(modular_adaptor& o)
   {
      base_data().swap(o.base_data());
      std::swap(mod_data(), o.mod_data());
   }

   inline std::string str(std::streamsize dig, std::ios_base::fmtflags f) const
   {
      value_type tmp;
      mod_data().adjust_regular(tmp, base_data());
      return tmp.str(dig, f);
   }

   constexpr void negate()
   {
      base_data().negate();
      eval_add(base_data(), mod_data().get_mod().backend());
   }

   template <typename BackendT, expression_template_option ExpressionTemplates>
   constexpr operator number<BackendT, ExpressionTemplates>()
   {
      return static_cast<BackendT>(base_data());
   };
};

template <class Result, class Backend>
constexpr void eval_convert_to(Result* result, const modular_adaptor<Backend>& val)
{
   using default_ops::eval_convert_to;
   eval_convert_to(result, val.base_data());
}

template <class Backend, class T>
constexpr typename enable_if<is_arithmetic<T>, bool>

    ::type eval_eq(const modular_adaptor<Backend>& a,
                   const T&                        b)

        BOOST_NOEXCEPT
{
   return a.compare(b) == 0;
}

template <class Backend1, class Backend2>
constexpr void eval_redc(Backend1& result, const modular_params<Backend2>& mod)
{
   mod.reduce(result);
   eval_modulus(result, mod.get_mod().backend());
}

template <class Backend>
constexpr void eval_add(modular_adaptor<Backend>&       result,
                     const modular_adaptor<Backend>& o)
{
   BOOST_ASSERT(result.mod_data().get_mod() == o.mod_data().get_mod());
   using default_ops::eval_gt;

   using default_ops::eval_eq;
   eval_add(result.base_data(), o.base_data());
   if (eval_gt(result.base_data(), result.mod_data().get_mod().backend()) ||
       eval_eq(result.base_data(), result.mod_data().get_mod().backend()))
   {
      eval_subtract(result.base_data(), result.mod_data().get_mod().backend());
   }
}

template <class Backend>
constexpr void eval_subtract(modular_adaptor<Backend>& result, const modular_adaptor<Backend>& o)
{
   BOOST_ASSERT(result.mod_data().get_mod() == o.mod_data().get_mod());
   typedef typename mpl::front<typename Backend::unsigned_types>::type ui_type;
   using default_ops::eval_lt;
   eval_subtract(result.base_data(), o.base_data());
   if (eval_lt(result.base_data(), ui_type(0u)))
   {
      eval_add(result.base_data(), result.mod_data().get_mod().backend());
   }
}

template <class Backend>
constexpr void eval_multiply(modular_adaptor<Backend>& result, const modular_adaptor<Backend>& o)
{
   BOOST_ASSERT(result.mod_data().get_mod() == o.mod_data().get_mod());
   eval_multiply(result.base_data(), o.base_data());
   eval_redc(result.base_data(), result.mod_data());
}

template <class Backend>
constexpr void eval_divide(modular_adaptor<Backend>& result, const modular_adaptor<Backend>& o)
{
   BOOST_ASSERT(result.mod_data().get_mod() == o.mod_data().get_mod());
   typename modular_adaptor<Backend>::value_type tmp1, tmp2;
   result.mod_data().adjust_regular(tmp1, result.base_data());
   result.mod_data().adjust_regular(tmp2, o.base_data());
   eval_divide(tmp1, tmp2);
   result.base_data() = tmp1;
   result.mod_data().adjust_modular(result.base_data());
   result.mod_data().adjust_regular(tmp2, result.base_data());
}

template <class Backend>
constexpr void eval_modulus(modular_adaptor<Backend>& result, const modular_adaptor<Backend>& o)
{
   BOOST_ASSERT(result.mod_data().get_mod() == o.mod_data().get_mod());
   typename modular_adaptor<Backend>::value_type tmp1, tmp2;
   result.mod_data().adjust_regular(tmp1, result.base_data());
   result.mod_data().adjust_regular(tmp2, o.base_data());
   eval_modulus(tmp1, tmp2);
   result.base_data() = tmp1;
   result.mod_data().adjust_modular(result.base_data());
   result.mod_data().adjust_regular(tmp2, result.base_data());
}

template <class Backend>
constexpr bool eval_is_zero(const modular_adaptor<Backend>& val)

    BOOST_NOEXCEPT
{
   using default_ops::eval_is_zero;
   return eval_is_zero(val.base_data());
}

// TODO: check returned value
template <class Backend>
constexpr int eval_get_sign(const modular_adaptor<Backend>&)
{
   return 1;
}

template <class Result, class Backend>
constexpr typename disable_if_c<boost::is_complex<Result>::value>::type
eval_convert_to(Result* result, const modular_adaptor<Backend>& val)
{
   using default_ops::eval_convert_to;
   eval_convert_to(result, val.base_data());
}

template <class Backend, class T, class V>
constexpr void assign_components(modular_adaptor<Backend>& result,
                                 const T& a, const V& b)
{
   result.base_data() = a;
   result.mod_data()  = b;
   result.mod_data().adjust_modular(result.base_data());
}

template<unsigned MinBits, cpp_integer_type SignType, cpp_int_check_type Checked,
         class T, class V>
constexpr void assign_components(
    modular_adaptor<cpp_int_backend<MinBits, MinBits, SignType, Checked, void>>& result,
    const T& a, const V& b)
{
   result.mod_data() = b;
   result.mod_data().adjust_modular(result.base_data(), a);
}

template <class Backend>
constexpr void eval_sqrt(modular_adaptor<Backend>&       result,
                      const modular_adaptor<Backend>& val)
{
   eval_sqrt(result.base_data(), val.base_data());
}

template <class Backend>
constexpr void eval_abs(modular_adaptor<Backend>& result, const modular_adaptor<Backend>& val)
{
   result = val;
}

size_t window_bits(size_t exp_bits)
{
   BOOST_STATIC_CONSTEXPR size_t wsize_count           = 6;
   BOOST_STATIC_CONSTEXPR size_t wsize[wsize_count][2] = {
       {1434, 7},
       {539, 6},
       {197, 4},
       {70, 3},
       {17, 2},
       {0, 0}};

   size_t window_bits = 1;

   size_t j = wsize_count - 1;
   while (wsize[j][0] > exp_bits)
   {
      --j;
   }
   window_bits += wsize[j][1];

   return window_bits;
};

template <class Backend>
inline void find_modular_pow(modular_adaptor<Backend>&       result,
                             const modular_adaptor<Backend>& b,
                             const Backend&                  exp)
{
   using default_ops::eval_bit_set;
   using default_ops::eval_decrement;
   using default_ops::eval_multiply;
   using default_ops::eval_convert_to;

   typedef number<modular_adaptor<Backend> > modular_type;
   modular_params<Backend>                   mod = b.mod_data();
   size_t                                    m_window_bits;
   unsigned long                             cur_exp_index;
   size_t                                    exp_bits = eval_msb(exp);
   m_window_bits                                      = window_bits(exp_bits + 1);

   std::vector<modular_type> m_g(1U << m_window_bits);
   modular_type*             p_g = m_g.data();
   modular_type              x(1, mod);
   Backend nibble = exp;
   Backend mask;
   eval_bit_set(mask, m_window_bits);
   eval_decrement(mask);
   *p_g = x;
   ++p_g;
   *p_g = b;
   ++p_g;
   for (size_t i = 2; i < (1U << m_window_bits); i++)
   {
      eval_multiply((*p_g).backend(), m_g[i - 1].backend(), b);
      ++p_g;
   }
   size_t exp_nibbles = (exp_bits + 1 + m_window_bits - 1) / m_window_bits;
   std::vector<size_t> exp_index;

   for (size_t i = 0; i < exp_nibbles; ++i)
   {
      Backend tmp = nibble;
      eval_bitwise_and(tmp, mask);
      eval_convert_to(&cur_exp_index, tmp);
      eval_right_shift(nibble, m_window_bits);
      exp_index.push_back(cur_exp_index);
   }

   x = x * m_g[exp_index[exp_nibbles - 1]];
   for (size_t i = exp_nibbles - 1; i > 0; --i)
   {

      for (size_t j = 0; j != m_window_bits; ++j)
      {
         x = x * x;
      }

      x = x * m_g[exp_index[i - 1]];
   }
   result = x.backend();
}

template <class Backend>
constexpr void eval_pow(modular_adaptor<Backend>&       result,
                        const modular_adaptor<Backend>& b,
                        const modular_adaptor<Backend>& e)
{
   typename modular_adaptor<Backend>::value_type exp;
   e.mod_data().adjust_regular(exp, e.base_data());
   find_modular_pow(result, b, exp);
}

template <class Backend1, typename Backend2>
constexpr void eval_pow(modular_adaptor<Backend1>&       result,
                        const modular_adaptor<Backend1>& b,
                        const Backend2&                  e)
{
   find_modular_pow(result, b, e);
}

template<unsigned MinBits, cpp_integer_type SignType, cpp_int_check_type Checked, typename Backend>
constexpr void eval_pow(modular_adaptor<cpp_int_backend<MinBits, MinBits, SignType, Checked, void>>& result,
                        const modular_adaptor<cpp_int_backend<MinBits, MinBits, SignType, Checked, void>>& b,
                        const Backend& e)
{
   // BOOST_ASSERT(result.mod_data().get_mod() == b.mod_data().get_mod());
   result.mod_data() = b.mod_data();
   b.mod_data().mod_exp(result.base_data(), b.base_data(), e);
}

template<unsigned MinBits, cpp_integer_type SignType, cpp_int_check_type Checked>
constexpr void eval_pow(modular_adaptor<cpp_int_backend<MinBits, MinBits, SignType, Checked, void>>& result,
                        const modular_adaptor<cpp_int_backend<MinBits, MinBits, SignType, Checked, void>>& b,
                        const modular_adaptor<cpp_int_backend<MinBits, MinBits, SignType, Checked, void>>& e)
{
   using Backend = cpp_int_backend<MinBits, MinBits, SignType, Checked, void>;

   typename modular_adaptor<Backend>::value_type exp;
   e.mod_data().adjust_regular(exp, e.base_data());
   eval_pow(result, b, exp);
}

template <class Backend, class UI>
constexpr void eval_left_shift(modular_adaptor<Backend>& t, UI i)
{
   using default_ops::eval_left_shift;
   typename modular_adaptor<Backend>::value_type tmp;
   t.mod_data().adjust_regular(tmp, t.base_data());
   eval_left_shift(tmp, i);
   t.base_data() = tmp;
   t.mod_data().adjust_modular(t.base_data());
}

template <class Backend, class UI>
constexpr void eval_right_shift(modular_adaptor<Backend>& t, UI i)
{
   using default_ops::eval_right_shift;
   typename modular_adaptor<Backend>::value_type tmp;
   t.mod_data().adjust_regular(tmp, t.base_data());
   eval_right_shift(tmp, i);
   t.base_data() = tmp;
   t.mod_data().adjust_modular(t.base_data());
}

template <class Backend, class UI>
constexpr void eval_left_shift(modular_adaptor<Backend>& t, const modular_adaptor<Backend>& v, UI i)
{
   using default_ops::eval_left_shift;
   typename modular_adaptor<Backend>::value_type tmp1, tmp2;
   t.mod_data().adjust_regular(tmp1, t.base_data());
   t.mod_data().adjust_regular(tmp2, v.base_data());
   eval_left_shift(tmp1, tmp2, static_cast<unsigned long>(i));
   t.base_data() = tmp1;
   t.mod_data().adjust_modular(t.base_data());
}

template <class Backend, class UI>
constexpr void eval_right_shift(modular_adaptor<Backend>& t, const modular_adaptor<Backend>& v, UI i)
{
   using default_ops::eval_right_shift;
   typename modular_adaptor<Backend>::value_type tmp1, tmp2;
   t.mod_data().adjust_regular(tmp1, t.base_data());
   t.mod_data().adjust_regular(tmp2, v.base_data());
   eval_right_shift(tmp1, tmp2, static_cast<unsigned long>(i));
   t.base_data() = tmp1;
   t.mod_data().adjust_modular(t.base_data());
}

template <class Backend>
constexpr void eval_bitwise_and(modular_adaptor<Backend>& result, const modular_adaptor<Backend>& v)
{
   using default_ops::eval_bitwise_and;
   typename modular_adaptor<Backend>::value_type tmp1, tmp2;
   result.mod_data().adjust_regular(tmp1, result.base_data());
   result.mod_data().adjust_regular(tmp2, v.base_data());
   eval_bitwise_and(tmp1, tmp1, tmp2);
   result.base_data() = tmp1;
   result.mod_data().adjust_modular(result.base_data());
}

template <class Backend>
constexpr void eval_bitwise_or(modular_adaptor<Backend>& result, const modular_adaptor<Backend>& v)
{
   using default_ops::eval_bitwise_or;
   typename modular_adaptor<Backend>::value_type tmp1, tmp2;
   result.mod_data().adjust_regular(tmp1, result.base_data());
   result.mod_data().adjust_regular(tmp2, v.base_data());
   eval_bitwise_or(tmp1, tmp1, tmp2);
   result.base_data() = tmp1;
   result.mod_data().adjust_modular(result.base_data());
}

template <class Backend>
constexpr void eval_bitwise_xor(modular_adaptor<Backend>& result, const modular_adaptor<Backend>& v)
{
   using default_ops::eval_bitwise_xor;
   typename modular_adaptor<Backend>::value_type tmp1, tmp2;
   result.mod_data().adjust_regular(tmp1, result.base_data());
   result.mod_data().adjust_regular(tmp2, v.base_data());
   eval_bitwise_xor(tmp1, tmp1, tmp2);
   result.base_data() = tmp1;
   result.mod_data().adjust_modular(result.base_data());
}

} // namespace backends

using boost::multiprecision::backends::modular_adaptor;

template <class Backend>
struct number_category<modular_adaptor<Backend> > : public boost::mpl::int_<boost::multiprecision::number_kind_modular>
{};

template <class Backend, expression_template_option ExpressionTemplates>
struct component_type<number<modular_adaptor<Backend>, ExpressionTemplates> >
{
   typedef number<Backend, ExpressionTemplates> type;
};

}

} // namespace boost::multiprecision

#endif
