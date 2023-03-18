//---------------------------------------------------------------------------//
// Copyright (c) 2021 Mikhail Komarov <nemo@nil.foundation>
// Copyright (c) 2021 Nikita Kaskov <nbering@nil.foundation>
// Copyright (c) 2021-2022 Ilias Khairullin <ilias@nil.foundation>
// Copyright (c) 2022-2023 Elena Tatuzova <e.tatuzova@nil.foundation>
//
// MIT License
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//---------------------------------------------------------------------------//

#ifndef CRYPTO3_MARSHALLING_PLACEHOLDER_PROOF_HPP
#define CRYPTO3_MARSHALLING_PLACEHOLDER_PROOF_HPP

#include <ratio>
#include <limits>
#include <type_traits>

#include <boost/assert.hpp>

#include <nil/marshalling/types/bundle.hpp>
#include <nil/marshalling/types/array_list.hpp>
#include <nil/marshalling/types/integral.hpp>
#include <nil/marshalling/status_type.hpp>
#include <nil/marshalling/options.hpp>

#include <nil/crypto3/marshalling/algebra/types/field_element.hpp>
#include <nil/crypto3/marshalling/zk/types/commitments/lpc.hpp>

#include <nil/crypto3/zk/snark/systems/plonk/placeholder/proof.hpp>

namespace nil {
    namespace crypto3 {
        namespace marshalling {
            namespace types {
                template<typename TTypeBase, typename Proof,
//                         typename = typename std::enable_if<
//                             std::is_same<Proof, nil::crypto3::zk::snark::placeholder_proof<
//                                                     typename Proof::field_type, typename Proof::params_type>>::value,
//                             bool>::type,
                         typename... TOptions>
                using placeholder_evaluation_proof = nil::marshalling::types::bundle<
                    TTypeBase,
                    std::tuple<
                        // typename FieldType::value_type challenge
                        field_element<TTypeBase, typename Proof::field_type::value_type>,
                        // typename FieldType::value_type lagrange_0
                        field_element<TTypeBase, typename Proof::field_type::value_type>,

                        //typename Proof::runtime_size_commitment_scheme_type::proof_type combined_value;
                        lpc_proof<TTypeBase, typename Proof::runtime_size_commitment_scheme_type>,

                        // std::vector<typename quotient_commitment_scheme_type::proof_type> lookups
                        nil::marshalling::types::array_list<
                            TTypeBase,
                            lpc_proof<TTypeBase, typename Proof::quotient_commitment_scheme_type>,
                            nil::marshalling::option::sequence_size_field_prefix<
                                nil::marshalling::types::integral<TTypeBase, std::uint64_t>>>
                    >
                >;


                template<typename Endianness, typename Proof>
                placeholder_evaluation_proof<nil::marshalling::field_type<Endianness>, Proof>
                    fill_placeholder_evaluation_proof(const typename Proof::evaluation_proof &proof) {

                    using TTypeBase = nil::marshalling::field_type<Endianness>;
                    using uint64_t_marshalling_type = nil::marshalling::types::integral<TTypeBase, std::uint64_t>;

                    using field_marhsalling_type = field_element<TTypeBase, typename Proof::field_type::value_type>;
                    using lpc_quotient_proof_marshalling_type =
                        lpc_proof<TTypeBase, typename Proof::runtime_size_commitment_scheme_type>;

                    using lpc_quotient_proof_vector_marshalling_type = nil::marshalling::types::array_list<
                        TTypeBase, lpc_quotient_proof_marshalling_type,
                        nil::marshalling::option::sequence_size_field_prefix<uint64_t_marshalling_type>>;

                    // typename FieldType::value_type challenge
                    field_marhsalling_type filled_challenge = field_marhsalling_type(proof.challenge);

                    // typename FieldType::value_type lagrange_0
                    field_marhsalling_type filled_lagrange_0 = field_marhsalling_type(proof.lagrange_0);

                    // typename runtime_size_commitment_scheme_type::proof_type fixed_values
                    auto filled_combined_value =
                        fill_lpc_proof<Endianness, typename Proof::runtime_size_commitment_scheme_type>(proof.combined_value);

                    // std::vector<typename quotient_commitment_scheme_type::proof_type> lookups
                    lpc_quotient_proof_vector_marshalling_type filled_lookups;
                    for (const auto &p : proof.lookups) {
                        filled_lookups.value().push_back(
                            fill_lpc_proof<Endianness, typename Proof::quotient_commitment_scheme_type>(p));
                    }

                    return placeholder_evaluation_proof<TTypeBase, Proof>(std::make_tuple(
                        filled_challenge, filled_lagrange_0, 
                        filled_combined_value, filled_lookups
                    ));
                }

                template<typename Endianness, typename Proof>
                typename Proof::evaluation_proof make_placeholder_evaluation_proof(
                    const placeholder_evaluation_proof<nil::marshalling::field_type<Endianness>, Proof> &filled_proof) {

                    typename Proof::evaluation_proof proof;
                    using TTypeBase = nil::marshalling::field_type<Endianness>;

                    // typename FieldType::value_type challenge
                    proof.challenge = std::get<0>(filled_proof.value()).value();

                    // typename FieldType::value_type lagrange_0
                    proof.lagrange_0 = std::get<1>(filled_proof.value()).value();

                    // typename runtime_size_commitment_scheme_type::proof_type fixed_values
                    proof.combined_value = make_lpc_proof<Endianness, typename Proof::runtime_size_commitment_scheme_type>(
                        std::get<2>(filled_proof.value()));

                    //  std::vector<typename quotient_commitment_scheme_type::proof_type> lookups
                    proof.lookups.reserve(std::get<3>(filled_proof.value()).value().size());
                    for (std::size_t i = 0; i < std::get<3>(filled_proof.value()).value().size(); ++i) {
                        proof.lookups.emplace_back(
                            make_lpc_proof<Endianness, typename Proof::quotient_commitment_scheme_type>(
                                std::get<3>(filled_proof.value()).value().at(i)));
                    }
                    
                    return proof;
                }

                template<typename TTypeBase, typename Proof,
//                         typename = typename std::enable_if<
//                             std::is_same<Proof, nil::crypto3::zk::snark::placeholder_proof<
//                                                     typename Proof::field_type, typename Proof::params_type>>::value,
//                             bool>::type,
                         typename... TOptions>
                using placeholder_proof = nil::marshalling::types::bundle<
                    TTypeBase,
                    std::tuple<
                        // typename variable_values_commitment_scheme_type::commitment_type variable_values_commitment
                        typename merkle_node_value<
                            TTypeBase, typename Proof::variable_values_commitment_scheme_type::commitment_type>::type,

                        //typename permutation_commitment_scheme_type::proof_type v_perm_commitment;
                        typename merkle_node_value<
                            TTypeBase, typename Proof::permutation_commitment_scheme_type::commitment_type>::type,

                        // typename permutation_commitment_scheme_type::commitment_type input_perm_commitment
                        typename merkle_node_value<
                            TTypeBase, typename Proof::permutation_commitment_scheme_type::commitment_type>::type,

                        // typename permutation_commitment_scheme_type::commitment_type value_perm_commitment
                        typename merkle_node_value<
                            TTypeBase, typename Proof::permutation_commitment_scheme_type::commitment_type>::type,

                        // typename permutation_commitment_scheme_type::commitment_type v_l_perm_commitment
                        typename merkle_node_value<
                            TTypeBase, typename Proof::permutation_commitment_scheme_type::commitment_type>::type,
                        
                        // typename runtime_size_commitment_scheme_type::commitment_type T_commitment
                        typename merkle_node_value<
                            TTypeBase, typename Proof::runtime_size_commitment_scheme_type::commitment_type>::type,
                        
                        // evaluation_proof eval_proof
                        placeholder_evaluation_proof<TTypeBase, Proof>
                    >
                >;

                template<typename Endianness, typename Proof>
                placeholder_proof<nil::marshalling::field_type<Endianness>, Proof>
                    fill_placeholder_proof(const Proof &proof) {

                    using TTypeBase = nil::marshalling::field_type<Endianness>;

                    // typename variable_values_commitment_scheme_type::commitment_type variable_values_commitment
                    auto filled_variable_values_commitment =
                        fill_merkle_node_value<typename Proof::variable_values_commitment_scheme_type::commitment_type,
                                               Endianness>(proof.variable_values_commitment);

                    // typename permutation_commitment_scheme_type::commitment_type v_perm_commitment
                    auto filled_v_perm_commitment =
                        fill_merkle_node_value<typename Proof::permutation_commitment_scheme_type::commitment_type,
                                               Endianness>(proof.v_perm_commitment);

                    // typename permutation_commitment_scheme_type::commitment_type input_perm_commitment
                    auto filled_input_perm_commitment =
                        fill_merkle_node_value<typename Proof::permutation_commitment_scheme_type::commitment_type,
                                               Endianness>(proof.input_perm_commitment);

                    // typename permutation_commitment_scheme_type::commitment_type value_perm_commitment
                    auto filled_value_perm_commitment =
                        fill_merkle_node_value<typename Proof::permutation_commitment_scheme_type::commitment_type,
                                               Endianness>(proof.value_perm_commitment);

                    // typename permutation_commitment_scheme_type::commitment_type v_l_perm_commitment
                    auto filled_v_l_perm_commitment =
                        fill_merkle_node_value<typename Proof::permutation_commitment_scheme_type::commitment_type,
                                               Endianness>(proof.v_l_perm_commitment);

                    // typename runtime_size_commitment_scheme_type::commitment_type T_commitment
                    auto filled_T_commitment =
                        fill_merkle_node_value<typename Proof::runtime_size_commitment_scheme_type::commitment_type,
                                               Endianness>(proof.T_commitment);

                    return placeholder_proof<TTypeBase, Proof>(std::make_tuple(
                        filled_variable_values_commitment,
                        filled_v_perm_commitment,
                        filled_input_perm_commitment,
                        filled_value_perm_commitment,
                        filled_v_l_perm_commitment,
                        filled_T_commitment,
                        fill_placeholder_evaluation_proof<Endianness, Proof>(proof.eval_proof)
                    ));
                }

                template<typename Endianness, typename Proof>
                Proof make_placeholder_proof( const placeholder_proof<nil::marshalling::field_type<Endianness>, Proof> &filled_proof) {

                    Proof proof;

                    // typename variable_values_commitment_scheme_type::commitment_type variable_values_commitment
                    proof.variable_values_commitment =
                        make_merkle_node_value<typename Proof::variable_values_commitment_scheme_type::commitment_type,
                                               Endianness>(std::get<0>(filled_proof.value()));

                    // typename permutation_commitment_scheme_type::commitment_type v_perm_commitment
                    proof.v_perm_commitment =
                        make_merkle_node_value<typename Proof::permutation_commitment_scheme_type::commitment_type,
                                               Endianness>(std::get<1>(filled_proof.value()));

                    // typename permutation_commitment_scheme_type::commitment_type input_perm_commitment
                    proof.input_perm_commitment =
                        make_merkle_node_value<typename Proof::permutation_commitment_scheme_type::commitment_type,
                                               Endianness>(std::get<2>(filled_proof.value()));

                    // typename permutation_commitment_scheme_type::commitment_type value_perm_commitment
                    proof.value_perm_commitment =
                        make_merkle_node_value<typename Proof::permutation_commitment_scheme_type::commitment_type,
                                               Endianness>(std::get<3>(filled_proof.value()));

                    // typename permutation_commitment_scheme_type::commitment_type v_l_perm_commitment
                    proof.v_l_perm_commitment =
                        make_merkle_node_value<typename Proof::permutation_commitment_scheme_type::commitment_type,
                                               Endianness>(std::get<4>(filled_proof.value()));

                    // typename runtime_size_commitment_scheme_type::commitment_type T_commitment
                    proof.T_commitment =
                        make_merkle_node_value<typename Proof::runtime_size_commitment_scheme_type::commitment_type,
                                               Endianness>(std::get<5>(filled_proof.value()));

                    // evaluation_proof eval_proof
                    proof.eval_proof =
                        make_placeholder_evaluation_proof<Endianness, Proof>(std::get<6>(filled_proof.value()));

                    return proof;
                }

            }    // namespace types
        }        // namespace marshalling
    }            // namespace crypto3
}    // namespace nil
#endif    // CRYPTO3_MARSHALLING_PLACEHOLDER_PROOF_HPP
