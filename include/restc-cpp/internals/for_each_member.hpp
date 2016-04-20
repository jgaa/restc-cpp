#pragma once

/**
 * \brief Allows iteration on member name and values of a Fusion adapted struct.
 *
 *
 * BOOST_FUSION_ADAPT_STRUCT(ns::point,
 * 		(int, x)
 *		(int, y)
 *		(int, z));
 *
 * template<class T>
 * print_name_and_value(const char* name, T& value) const {
 *    std::cout << name << "=" << value << std::endl;
 * }
 *
 *
 * int main(void) {
 *
 *	ns::point mypoint;
 *
 *
 *      boost::boost::fusion::for_each_member(mypoint, &print_name_and_value);
 *
 *
 * }
 *
 * ********
 * Credits to Damien Buhl
 *     https://gist.github.com/daminetreg/6b539973817ed8f4f87d
 */
#ifndef BOOST_FUSION_FOR_EACH_MEMBER_HPP
#define BOOST_FUSION_FOR_EACH_MEMBER_HPP

#include <functional>

#include <boost/fusion/include/adapt_struct.hpp>

#include <boost/fusion/sequence/intrinsic/begin.hpp>
#include <boost/fusion/sequence/intrinsic/end.hpp>
#include <boost/fusion/sequence/intrinsic/front.hpp>
#include <boost/fusion/iterator/equal_to.hpp>
#include <boost/fusion/iterator/next.hpp>
#include <boost/fusion/iterator/deref.hpp>
#include <boost/fusion/iterator/distance.hpp>
#include <boost/fusion/support/category_of.hpp>
#include <boost/mpl/bool.hpp>

namespace restc_cpp {
namespace detail {

template <typename First, typename Last, typename F>
inline void
for_each_member_linear(First const& first,
    Last const& last,
    F const& f,
    boost::mpl::true_)
{
}

template <typename First, typename Last, typename F>
inline void
for_each_member_linear(First const& first,
    Last const& last,
    F const& f,
    boost::mpl::false_) {

    using type_t = typename std::remove_const<typename std::remove_reference<typename First::seq_type>::type>::type;

    auto name = boost::fusion::extension::struct_member_name<type_t, First::index::value>::call();

    f(name, *first);

    for_each_member_linear(
        next(first),
        last,
        f,
        boost::fusion::result_of::equal_to< typename boost::fusion::result_of::next<First>::type, Last>()
    );
}

template <typename Sequence, typename F>
inline void
for_each_member(Sequence& seq, F const& f) {

    detail::for_each_member_linear(
        boost::fusion::begin(seq),
        boost::fusion::end(seq),
        f,
        boost::fusion::result_of::equal_to<
        typename boost::fusion::result_of::begin<Sequence>::type,
        typename boost::fusion::result_of::end<Sequence>::type>()
    );
}

} // detail

template <typename Sequence, typename F>
inline void
for_each_member(Sequence& seq, F f) {
    detail::for_each_member(seq, f);
}

} // restc_cpp

#endif
