///////////////////////////////////////////////////////////////
//  Copyright 2012 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt

#ifdef _MSC_VER
#define _SCL_SECURE_NO_WARNINGS
#endif

#include "skeleton_backend.hpp"
#include "test_arithmetic.hpp"

int main() {
    test<boost::multiprecision::skeleton_number>();
    return boost::report_errors();
}
