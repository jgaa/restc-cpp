
#pragma once

namespace {

template<typename T1, typename T2>
bool compare(const T1& left, const T2& right) {
    const auto state = (left == right);
    if (!state) {
        std::cerr << ">>>> '" << left << "' is not equal to '" << right << "'" << std::endl;
    }
    return state;
}

}


#define CHECK_EQUAL(a,b) EXPECT(compare(a,b))
#define TEST(name) CASE(#name)
