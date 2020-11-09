//---------------------------------------------------------------------------//
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
// Copyright (c) 2020 Nikita Kaskov <nbering@nil.foundation>
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
// @file Declaration of interfaces for a ppzkSNARK for BACS.
//
// This includes:
// - class for proving key
// - class for verification key
// - class for processed verification key
// - class for key pair (proving key & verification key)
// - class for proof
// - generator algorithm
// - prover algorithm
// - verifier algorithm (with strong or weak input consistency)
// - online verifier algorithm (with strong or weak input consistency)
//
// The implementation is a straightforward combination of:
// (1) a BACS-to-R1CS reduction, and
// (2) a ppzkSNARK for R1CS.
//
//
// Acronyms:
//
// - BACS = "Bilinear Arithmetic Circuit Satisfiability"
// - R1CS = "Rank-1 Constraint System"
// - ppzkSNARK = "PreProcessing Zero-Knowledge Succinct Non-interactive ARgument of Knowledge"
//---------------------------------------------------------------------------//

#ifndef CRYPTO3_ZK_R1CS_SE_PPZKSNARK_BASIC_VERIFIER_HPP
#define CRYPTO3_ZK_R1CS_SE_PPZKSNARK_BASIC_VERIFIER_HPP

#include <memory>

#include <nil/crypto3/zk/snark/accumulation_vector.hpp>
#include <nil/crypto3/zk/snark/relations/constraint_satisfaction_problems/r1cs.hpp>

#include <nil/crypto3/algebra/multiexp/multiexp.hpp>
#include <nil/crypto3/algebra/multiexp/policies.hpp>

#ifdef MULTICORE
#include <omp.h>
#endif

#include <nil/crypto3/zk/snark/reductions/r1cs_to_sap.hpp>
#include <nil/crypto3/zk/snark/proof_systems/detail/ppzksnark/r1cs_se_ppzksnark/types_policy.hpp>

namespace nil {
    namespace crypto3 {
        namespace zk {
            namespace snark {
                namespace policies {

                    /**
                     * Convert a (non-processed) verification key into a processed verification key.
                     */
                    template<typename CurveType>
                    class r1cs_se_ppzksnark_verifier_process_vk {
                        using types_policy = detail::r1cs_se_ppzksnark_types_policy<CurveType>;

                    public:
                        typedef typename types_policy::constraint_system constraint_system_type;
                        typedef typename types_policy::primary_input primary_input_type;
                        typedef typename types_policy::auxiliary_input auxiliary_input_type;

                        typedef typename types_policy::proving_key proving_key_type;
                        typedef typename types_policy::verification_key verification_key_type;
                        typedef typename types_policy::processed_verification_key processed_verification_key_type;

                        typedef typename types_policy::keypair keypair_type;
                        typedef typename types_policy::proof proof_type;

                        static inline processed_verification_key_type
                            process(const verification_key_type &verification_key) {
                            typedef typename CurveType::pairing_policy pairing_policy;

                            typename pairing_policy::G1_precomp G_alpha_pc =
                                pairing_policy::precompute_g1(verification_key.G_alpha);
                            typename pairing_policy::G2_precomp H_beta_pc =
                                pairing_policy::precompute_g2(verification_key.H_beta);

                            processed_verification_key_type processed_verification_key;
                            processed_verification_key.G_alpha = verification_key.G_alpha;
                            processed_verification_key.H_beta = verification_key.H_beta;
                            processed_verification_key.G_alpha_H_beta_ml =
                                pairing_policy::miller_loop(G_alpha_pc, H_beta_pc);
                            processed_verification_key.G_gamma_pc =
                                pairing_policy::precompute_g1(verification_key.G_gamma);
                            processed_verification_key.H_gamma_pc =
                                pairing_policy::precompute_g2(verification_key.H_gamma);
                            processed_verification_key.H_pc = pairing_policy::precompute_g2(verification_key.H);

                            processed_verification_key.query = verification_key.query;

                            return processed_verification_key;
                        }
                    };

                    /*
                     Below are four variants of verifier algorithm for the R1CS SEppzkSNARK.

                     These are the four cases that arise from the following two choices:

                     (1) The verifier accepts a (non-processed) verification key or, instead, a processed
                     verification key. In the latter case, we call the algorithm an "online verifier".

                     (2) The verifier checks for "weak" input consistency or, instead, "strong" input consistency.
                         Strong input consistency requires that |primary_input| = CS.num_inputs, whereas
                         weak input consistency requires that |primary_input| <= CS.num_inputs (and
                         the primary input is implicitly padded with zeros up to length CS.num_inputs).
                     */

                    /**
                     * A verifier algorithm for the R1CS ppzkSNARK that:
                     * (1) accepts a processed verification key, and
                     * (2) has weak input consistency.
                     */
                    template<typename CurveType>
                    class r1cs_se_ppzksnark_online_verifier_weak_input_consistency {
                        using types_policy = detail::r1cs_se_ppzksnark_types_policy<CurveType>;

                    public:
                        typedef typename types_policy::constraint_system constraint_system_type;
                        typedef typename types_policy::primary_input primary_input_type;
                        typedef typename types_policy::auxiliary_input auxiliary_input_type;

                        typedef typename types_policy::proving_key proving_key_type;
                        typedef typename types_policy::verification_key verification_key_type;
                        typedef typename types_policy::processed_verification_key processed_verification_key_type;

                        typedef typename types_policy::keypair keypair_type;
                        typedef typename types_policy::proof proof_type;

                        static inline bool process(const processed_verification_key_type &processed_verification_key,
                                                   const primary_input_type &primary_input,
                                                   const proof_type &proof) {

                            bool result = true;

                            if (!proof.is_well_formed()) {
                                result = false;
                            }

#ifdef MULTICORE
                            const std::size_t chunks = omp_get_max_threads();    // to override, set OMP_NUM_THREADS env
                                                                                 // var or call omp_set_num_threads()
#else
                            const std::size_t chunks = 1;
#endif

                            /**
                             * e(A*G^{alpha}, B*H^{beta}) = e(G^{alpha}, H^{beta}) * e(G^{psi}, H^{gamma})
                             *                              * e(C, H)
                             * where psi = \sum_{i=0}^l input_i processed_verification_key.query[i]
                             */
                            typename CurveType::g1_type::value_type G_psi =
                                processed_verification_key.query[0] +
                                algebra::multiexp<typename CurveType::g1_type, typename CurveType::scalar_field_type,
                                                  algebra::policies::multiexp_method_bos_coster<
                                                  typename CurveType::g1_type, typename CurveType::scalar_field_type>>(
                                    processed_verification_key.query.begin() + 1,
                                    processed_verification_key.query.end(), primary_input.begin(), primary_input.end(),
                                    chunks);

                            typename pairing_policy::Fqk_type
                                test1_l = pairing_policy::miller_loop(
                                    pairing_policy::precompute_g1(proof.A + processed_verification_key.G_alpha),
                                    pairing_policy::precompute_g2(proof.B + processed_verification_key.H_beta)),
                                test1_r1 = processed_verification_key.G_alpha_H_beta_ml,
                                test1_r2 = pairing_policy::miller_loop(pairing_policy::precompute_g1(G_psi),
                                                                       processed_verification_key.H_gamma_pc),
                                test1_r3 = pairing_policy::miller_loop(pairing_policy::precompute_g1(proof.C),
                                                                       processed_verification_key.H_pc);
                            typename CurveType::gt_type::value_type test1 = pairing_policy::final_exponentiation(
                                test1_l.unitary_inversed() * test1_r1 * test1_r2 * test1_r3);

                            if (test1 != typename CurveType::gt_type::one()) {
                                result = false;
                            }

                            /**
                             * e(A, H^{gamma}) = e(G^{gamma}, B)
                             */
                            typename pairing_policy::Fqk_type test2_l = pairing_policy::miller_loop(
                                                                  pairing_policy::precompute_g1(proof.A),
                                                                  processed_verification_key.H_gamma_pc),
                                                              test2_r = pairing_policy::miller_loop(
                                                                  processed_verification_key.G_gamma_pc,
                                                                  pairing_policy::precompute_g2(proof.B));
                            typename CurveType::gt_type::value_type test2 =
                                pairing_policy::final_exponentiation(test2_l * test2_r.unitary_inversed());

                            if (test2 != typename CurveType::gt_type::one()) {
                                result = false;
                            }

                            return result;
                        }
                    };

                    /**
                     * A verifier algorithm for the R1CS SEppzkSNARK that:
                     * (1) accepts a non-processed verification key, and
                     * (2) has weak input consistency.
                     */
                    template<typename CurveType>
                    class r1cs_se_ppzksnark_verifier_weak_input_consistency {
                        using types_policy = detail::r1cs_se_ppzksnark_types_policy<CurveType>;

                    public:
                        typedef typename types_policy::constraint_system constraint_system_type;
                        typedef typename types_policy::primary_input primary_input_type;
                        typedef typename types_policy::auxiliary_input auxiliary_input_type;

                        typedef typename types_policy::proving_key proving_key_type;
                        typedef typename types_policy::verification_key verification_key_type;
                        typedef typename types_policy::processed_verification_key processed_verification_key_type;

                        typedef typename types_policy::keypair keypair_type;
                        typedef typename types_policy::proof proof_type;

                        static inline bool process(const verification_key_type &vk,
                                                   const primary_input_type &primary_input,
                                                   const proof_type &proof) {
                            processed_verification_key_type pvk =
                                r1cs_se_ppzksnark_verifier_process_vk<CurveType>::process(vk);
                            bool result = r1cs_se_ppzksnark_online_verifier_weak_input_consistency<CurveType>::process(
                                pvk, primary_input, proof);
                            return result;
                        }
                    };

                    /**
                     * A verifier algorithm for the R1CS ppzkSNARK that:
                     * (1) accepts a processed verification key, and
                     * (2) has strong input consistency.
                     */
                    template<typename CurveType>
                    class r1cs_se_ppzksnark_online_verifier_strong_input_consistency {
                        using types_policy = detail::r1cs_se_ppzksnark_types_policy<CurveType>;

                    public:
                        typedef typename types_policy::constraint_system constraint_system_type;
                        typedef typename types_policy::primary_input primary_input_type;
                        typedef typename types_policy::auxiliary_input auxiliary_input_type;

                        typedef typename types_policy::proving_key proving_key_type;
                        typedef typename types_policy::verification_key verification_key_type;
                        typedef typename types_policy::processed_verification_key processed_verification_key_type;

                        typedef typename types_policy::keypair keypair_type;
                        typedef typename types_policy::proof proof_type;

                        static inline bool process(const processed_verification_key_type &pvk,
                                                   const primary_input_type &primary_input,
                                                   const proof_type &proof) {

                            bool result = true;

                            if (pvk.query.size() != primary_input.size() + 1) {
                                result = false;
                            } else {
                                result = r1cs_se_ppzksnark_online_verifier_weak_input_consistency<CurveType>::process(
                                    pvk, primary_input, proof);
                            }

                            return result;
                        }
                    };

                    /**
                     * A verifier algorithm for the R1CS SEppzkSNARK that:
                     * (1) accepts a non-processed verification key, and
                     * (2) has strong input consistency.
                     */
                    template<typename CurveType>
                    class r1cs_se_ppzksnark_verifier_strong_input_consistency {
                        using types_policy = detail::r1cs_se_ppzksnark_types_policy<CurveType>;

                    public:
                        typedef typename types_policy::constraint_system constraint_system_type;
                        typedef typename types_policy::primary_input primary_input_type;
                        typedef typename types_policy::auxiliary_input auxiliary_input_type;

                        typedef typename types_policy::proving_key proving_key_type;
                        typedef typename types_policy::verification_key verification_key_type;
                        typedef typename types_policy::processed_verification_key processed_verification_key_type;

                        typedef typename types_policy::keypair keypair_type;
                        typedef typename types_policy::proof proof_type;

                        static inline bool process(const verification_key_type &vk,
                                                   const primary_input_type &primary_input,
                                                   const proof_type &proof) {
                            processed_verification_key_type pvk =
                                r1cs_se_ppzksnark_verifier_process_vk<CurveType>::process(vk);
                            bool result =
                                r1cs_se_ppzksnark_online_verifier_strong_input_consistency<CurveType>::process(
                                    pvk, primary_input, proof);
                            return result;
                        }
                    };
                }    // namespace policies
            }        // namespace snark
        }            // namespace zk
    }                // namespace crypto3
}    // namespace nil

#endif    // CRYPTO3_ZK_R1CS_SE_PPZKSNARK_BASIC_GENERATOR_HPP
