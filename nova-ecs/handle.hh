// Copyright (C) 2016-2017 ChaosForge Ltd
// http://chaosforge.org/

#ifndef NV_ECS_HANDLE_HH
#define NV_ECS_HANDLE_HH

#include <functional> // hash

class handle
{
public:
	static constexpr int INDEX_BITS   = 16;
	static constexpr int COUNTER_BITS = 16;

	constexpr handle() : index( 0 ), counter( 0 ) {}
	constexpr handle( unsigned a_index, unsigned a_counter )
		: index( a_index ), counter( a_counter ) {}
	
	constexpr inline bool operator==( const handle& rhs ) const	{
		return index == rhs.index && counter == rhs.counter;
	}
	constexpr inline bool operator!=( const handle& rhs ) const { return !(*this == rhs); }

	constexpr bool is_valid()    const { return !(index == 0 && counter == 0); }
	constexpr operator bool()    const { return is_valid(); }
	constexpr unsigned hash()    const { return counter << INDEX_BITS | index; }
	unsigned index   : INDEX_BITS;
	unsigned counter : COUNTER_BITS;
};

namespace std
{
	template<> struct hash<handle>
	{
		size_t operator()( const handle& s ) const noexcept
		{
			return s.hash();
		}
	};
}

#endif // NV_ECS_HANDLE_HH
