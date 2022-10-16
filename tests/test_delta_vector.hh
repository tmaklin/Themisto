#pragma once

#include "setup_tests.hh"
#include "delta_vector.hh"

TEST(DELTA_VECTOR, basic) {
    vector<int64_t> v = {0, 3, 6, 7, 12, 13, 14, 1000, 1000000};
    Delta_Vector dv(v);

    ASSERT_EQ(dv.size(), v.size());
    ASSERT_EQ(v, dv.get_values());

    Delta_Vector dv2 = to_disk_and_back(dv);
    ASSERT_EQ(dv2.size(), dv.size());
    ASSERT_EQ(dv2.get_values(), dv.get_values());

    
}