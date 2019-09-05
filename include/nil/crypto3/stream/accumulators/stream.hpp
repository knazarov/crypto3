//---------------------------------------------------------------------------//
// Copyright (c) 2018-2019 Nil Foundation AG
// Copyright (c) 2018-2019 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//---------------------------------------------------------------------------//

#ifndef CRYPTO3_ACCUMULATORS_STREAM_HPP
#define CRYPTO3_ACCUMULATORS_STREAM_HPP

#include <boost/parameter/value_type.hpp>

#include <boost/accumulators/framework/accumulator_base.hpp>
#include <boost/accumulators/framework/extractor.hpp>
#include <boost/accumulators/framework/depends_on.hpp>
#include <boost/accumulators/framework/parameters/sample.hpp>

#include <boost/container/static_vector.hpp>

#include <nil/crypto3/stream/accumulators/parameters/bits.hpp>

#include <nil/crypto3/stream/detail/make_array.hpp>
#include <nil/crypto3/stream/detail/digest.hpp>

namespace nil {
    namespace crypto3 {
        namespace accumulators {
            namespace impl {
                template<typename StreamCipherMode>
                struct stream_impl : boost::accumulators::accumulator_base {
                protected:
                    typedef StreamCipherMode stream_mode_type;

                    typedef typename stream_mode_type::finalizer_type finalizer_type;

                    constexpr static const std::size_t input_block_bits = stream_mode_type::input_block_bits;
                    constexpr static const std::size_t input_block_values = stream_mode_type::input_block_values;
                    typedef typename stream_mode_type::input_block_type input_block_type;

                    constexpr static const std::size_t input_value_bits = stream_mode_type::input_value_bits;
                    typedef typename input_block_type::value_type input_value_type;

                    constexpr static const std::size_t output_block_bits = stream_mode_type::output_block_bits;
                    constexpr static const std::size_t output_block_values = stream_mode_type::output_block_values;
                    typedef typename stream_mode_type::output_block_type output_block_type;

                    constexpr static const std::size_t output_value_bits = stream_mode_type::output_value_bits;
                    typedef typename output_block_type::value_type output_value_type;

                    typedef boost::container::static_vector<input_value_type, input_block_values> cache_type;

                public:
                    typedef stream::digest<output_block_bits> result_type;

                    template<typename Args>
                    // The constructor takes an argument pack.
                    stream_impl(const Args &args) : seen(0) {
                    }

                    template<typename ArgumentPack>
                    inline void operator()(const ArgumentPack &args) {
                        resolve_type(args[boost::accumulators::sample], args[bits | std::size_t()]);
                    }

                    template<typename ArgumentPack>
                    inline result_type result(const ArgumentPack &args) const {
                        result_type res = digest;

                        if (!cache.empty()) {
                            input_block_type ib = {0};
                            std::move(cache.begin(), cache.end(), ib.begin());
                            output_block_type ob = stream_mode_type::process_block(ib);
                            std::move(ob.begin(), ob.end(), std::inserter(res, res.end()));
                        }

                        if (seen % input_block_bits) {
                            finalizer_type(input_block_bits - seen % input_block_bits)(res);
                        } else {
                            finalizer_type(0)(res);
                        }

                        return res;
                    }

                protected:
                    inline void resolve_type(const input_value_type &value, std::size_t bits) {
                        if (bits == std::size_t()) {
                            process(value, input_value_bits);
                        } else {
                            process(value, bits);
                        }
                    }

                    inline void resolve_type(const input_block_type &value, std::size_t bits) {
                        if (bits == std::size_t()) {
                            process(value, input_block_bits);
                        } else {
                            process(value, bits);
                        }
                    }

                    inline void process(const input_value_type &value, std::size_t bits) {
                        if (cache.size() == cache.max_size()) {
                            input_block_type ib = {0};
                            std::move(cache.begin(), cache.end(), ib.begin());
                            output_block_type ob = stream_mode_type::process_block(ib);
                            std::move(ob.begin(), ob.end(), std::inserter(digest, digest.end()));

                            cache.clear();
                        }

                        cache.push_back(value);
                        seen += input_value_bits;
                    }

                    inline void process(const input_block_type &block, std::size_t bits) {
                        output_block_type ob;
                        if (cache.empty()) {
                            ob = stream_mode_type::process_block(block);
                        } else {
                            input_block_type b = stream::make_array<input_block_values>(cache.begin(), cache.end());
                            typename input_block_type::const_iterator itr =
                                block.begin() + (cache.max_size() - cache.size());

                            std::move(block.begin(), itr, b.end());

                            ob = stream_mode_type::process_block(b);

                            cache.clear();
                            cache.insert(cache.end(), itr, block.end());
                        }

                        std::move(ob.begin(), ob.end(), std::inserter(digest, digest.end()));
                        seen += input_block_bits;
                    }

                    std::size_t seen;
                    cache_type cache;
                    result_type digest;
                };
            }    // namespace impl

            namespace tag {
                template<typename ProcessingMode>
                struct stream : boost::accumulators::depends_on<> {
                    typedef ProcessingMode mode_type;

                    /// INTERNAL ONLY
                    ///

                    typedef boost::mpl::always<accumulators::impl::stream_impl<mode_type>> impl;
                };
            }    // namespace tag

            namespace extract {
                template<typename Mode, typename AccumulatorSet>
                typename boost::mpl::apply<AccumulatorSet, tag::stream<Mode>>::type::result_type
                    stream(const AccumulatorSet &acc) {
                    return boost::accumulators::extract_result<tag::stream<Mode>>(acc);
                }

                template<typename StreamCipher, typename AccumulatorSet>
                typename boost::mpl::apply<AccumulatorSet,
                                           tag::stream<typename StreamCipher::stream_encoder_type>>::type::result_type
                    encrypt(const AccumulatorSet &acc) {
                    return boost::accumulators::extract_result<tag::stream<typename StreamCipher::stream_encoder_type>>(
                        acc);
                }

                template<typename StreamCipher, typename AccumulatorSet>
                typename boost::mpl::apply<AccumulatorSet,
                                           tag::stream<typename StreamCipher::stream_decoder_type>>::type::result_type
                    decrypt(const AccumulatorSet &acc) {
                    return boost::accumulators::extract_result<tag::stream<typename StreamCipher::stream_decoder_type>>(
                        acc);
                }
            }    // namespace extract
        }        // namespace accumulators
    }            // namespace crypto3
}    // namespace nil

#endif    // CRYPTO3_ACCUMULATORS_STREAM_HPP
