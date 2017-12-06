// Copyright (C) 2017-2017 ChaosForge Ltd
// http://chaosforge.org/

#ifndef NV_MPL_HH
#define NV_MPL_HH

#include <type_traits>

namespace mpl
{

	template < typename... Ts > struct list {};

	using empty_list = mpl::list<>;

	namespace detail
	{

		template< typename T > struct list_size_impl;
		template< typename... T> struct list_size_impl< list<T...> >
		{
			using type = std::integral_constant< int, sizeof...( T )>;
		};

		template< typename T > struct head_impl;
		template< typename H, typename... T> struct head_impl< list<H,T...> >
		{
			using type = H;
		};

	}

	template< typename List > 
	using list_size = typename detail::list_size_impl<List>::type;

	template< typename List >
	using head = typename detail::head_impl<List>::type;

}

#endif // NV_MPL_HH
