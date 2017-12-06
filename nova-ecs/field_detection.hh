// Copyright (C) 2016-2017 ChaosForge Ltd
// http://chaosforge.org/

#ifndef NV_ECS_FIELD_DETECTION_HH
#define NV_ECS_FIELD_DETECTION_HH

#include "mpl.hh"

namespace detail
{
	template< typename C, typename... Args >
	constexpr decltype(std::declval<C>().on( std::declval<Args>()... ), true) has_message( int ) { return true; }

	template< typename C, typename... Args >
	constexpr bool has_message( ... ) { return false; }

	template< typename C, typename... Args >
	constexpr decltype(std::declval<C>().update( std::declval<Args>()... ), true) has_update( int ) { return true; }

	template< typename C, typename... Args >
	constexpr bool has_update( ... ) { return false; }

	template< typename C, typename... Args >
	constexpr decltype(std::declval<C>().create( std::declval<Args>()... ), true) has_create( int ) { return true;  }

	template< typename C, typename... Args >
	constexpr bool has_create( ... ) { return false; }

	template< typename C, typename... Args >
	constexpr decltype(std::declval<C>().destroy( std::declval<Args>()... ), true) has_destroy( int ) { return true; }

	template< typename C, typename... Args >
	constexpr bool has_destroy( ... ) { return false; }

	template< typename C >
	constexpr decltype( std::declval< C::components >(), true) has_components( int ) { return true; }

	template< typename C >
	constexpr bool has_components( ... ) { return false; }

	template < typename S, typename T, typename Cs >
	struct has_ct_update_helper;

	template < typename S, typename T, typename... Cs >
	struct has_ct_update_helper< S, T, mpl::list< Cs... > >
	{
		static constexpr bool value = detail::has_update< S, Cs&..., T >( 0 );
	};

	template < typename S, typename E, typename T, typename Cs >
	struct has_ect_update_helper;

	template < typename S, typename E, typename T, typename... Cs >
	struct has_ect_update_helper< S, E, T, mpl::list< Cs... > >
	{
		static constexpr bool value = detail::has_update< S, E&, Cs&..., T >( 0 );
	};

	template < typename S, typename E, typename M, typename Cs >
	struct has_ec_message_helper;

	template < typename S, typename E, typename M, typename... Cs >
	struct has_ec_message_helper< S, E, M, mpl::list< Cs... > >
	{
		static constexpr bool value = detail::has_message< S, const M&, E&, Cs&... >( 0 );
	};

	template < typename S, typename M, typename Cs >
	struct has_c_message_helper;

	template < typename S, typename M, typename... Cs >
	struct has_c_message_helper< S, M, mpl::list< Cs... > >
	{
		static constexpr bool value = detail::has_message< S, const M&, Cs&... >( 0 );
	};

}

template < typename S, typename M >
constexpr bool has_message = detail::has_message<S, const M& >( 0 );

template < typename S >
constexpr bool has_components = detail::has_components<S>( 0 );

template < typename E, typename S, typename T >
constexpr bool has_ecs_update = detail::has_update<S, E&, T>( 0 );

template < typename E, typename S, typename M >
constexpr bool has_ecs_message = detail::has_message<S, const M&, E&>( 0 );

template < typename S, typename T >
constexpr bool has_destroy = detail::has_destroy<S, T& >( 0 );

template < typename S, typename T, typename H >
constexpr bool has_create = detail::has_create<S, H, T& >(0);

template < typename S, typename Cs, typename T >
constexpr bool has_component_update = detail::has_ct_update_helper<S, T, Cs >::value;

template < typename E, typename S, typename Cs, typename T >
constexpr bool has_ecs_component_update = detail::has_ect_update_helper<S, E, T, Cs >::value;

template < typename S, typename Cs, typename M >
constexpr bool has_component_message = detail::has_c_message_helper<S, M, Cs >::value;

template < typename E, typename S, typename Cs, typename M >
constexpr bool has_ecs_component_message = detail::has_ec_message_helper<S, E, M, Cs >::value;

#endif // NV_ECS_FIELD_DETECTION_HH
