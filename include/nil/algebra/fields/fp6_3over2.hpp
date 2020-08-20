//---------------------------------------------------------------------------//
// Copyright (c) 2020 Mikhail Komarov <nemo@nil.foundation>
// Copyright (c) 2020 Nikita Kaskov <nbering@nil.foundation>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//---------------------------------------------------------------------------//

#ifndef ALGEBRA_FIELDS_FP6_3OVER2_HPP
#define ALGEBRA_FIELDS_FP6_3OVER2_HPP

#include <boost/multiprecision/number.hpp>
#include <boost/multiprecision/cpp_int.hpp>

namespace nil {
    namespace algebra {
        namespace fields {
                    
            /**
             * Arithmetic in the finite field F[(p^2)^3].
             *
             * Let p := modulus. This interface provides arithmetic for the extension field
             * Fp6 = Fp2[V]/(V^3-non_residue) where non_residue is in Fp2.
             *
             * ASSUMPTION: p = 1 (mod 6)
             */
            template<std::size_t ModulusBits, std::size_t GeneratorBits>
            struct fp6_3over2 {

                constexpr static const std::size_t modulus_bits = ModulusBits;
                typedef boost::multiprecision::number<boost::multiprecision::backends::cpp_int_backend<
                    modulus_bits, modulus_bits, boost::multiprecision::unsigned_magnitude, boost::multiprecision::unchecked,
                    void>>
                    modulus_type;

                constexpr static const std::size_t number_bits = ModulusBits;
                typedef boost::multiprecision::number<boost::multiprecision::backends::modular_adaptor <
                    boost::multiprecision::backends::cpp_int_backend<
                    //modulus_bits, modulus_bits, boost::multiprecision::unsigned_magnitude, boost::multiprecision::unchecked, void
                    >>>
                    number_type;

                constexpr static const std::size_t generator_bits = GeneratorBits;
                typedef boost::multiprecision::number<boost::multiprecision::backends::cpp_int_backend<
                    generator_bits, generator_bits, boost::multiprecision::unsigned_magnitude,
                    boost::multiprecision::unchecked, void>>
                    generator_type;
                    
            };

        }   // namespace fields
    }    // namespace algebra
}    // namespace nil

#endif    // ALGEBRA_FIELDS_FP6_3OVER2_HPP
