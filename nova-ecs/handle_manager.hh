// Copyright (C) 2017-2017 ChaosForge Ltd
// http://chaosforge.org/

#ifndef NV_ECS_HANDLE_MANAGER_HH
#define NV_ECS_HANDLE_MANAGER_HH

#include <vector>
#include <cassert>
#include "handle.hh"

class handle_manager
{
	typedef int      index_type;
	typedef unsigned value_type;
	static const index_type NONE = index_type( -1 );
	static const index_type USED = index_type( -2 );
public:

	handle_manager() : m_first_free( NONE ), m_last_free( NONE ) {}

	handle create_handle()
	{
		value_type i = get_free_entry();
		m_entries[i].counter++;
		assert( m_entries[i].counter != 0 && "Out of handles!" );
		m_entries[i].next_free = USED;
		return handle( i, m_entries[i].counter );
	}

	void free_handle( handle h )
	{
		value_type index = h.index;
		m_entries[index].next_free = NONE;
		if ( m_last_free == NONE )
		{
			m_first_free = m_last_free = index;
			return;
		}
		m_entries[ m_last_free ].next_free = index;
		m_last_free = index;
	}

	bool is_valid( handle h ) const
	{
		if ( !h.is_valid() ) return false;
		if ( h.index >= m_entries.size() ) return false;
		const index_entry& entry = m_entries[h.index];
		return entry.next_free == USED && entry.counter == h.counter;
	}

	void clear()
	{
		m_first_free = NONE;
		m_last_free = NONE;
		m_entries.clear();
	}

	handle get_handle( index_type i ) const
	{
		if ( i >= 0 && i < m_entries.size() )
			return handle( i, m_entries[i].counter );
		return {};
	}

private:
	struct index_entry
	{
		value_type counter;
		index_type next_free;

		index_entry() : counter( 0 ), next_free( NONE ) {}
	};

	value_type get_free_entry()
	{
		if ( m_first_free != NONE )
		{
			value_type result = m_first_free;
			m_first_free = m_entries[result].next_free;
			m_entries[result].next_free = USED;
			if ( m_first_free == NONE ) m_last_free = NONE;
			return result;
		}
		m_entries.emplace_back();
		return value_type( m_entries.size() - 1 );
	}

	index_type m_first_free;
	index_type m_last_free;
	std::vector< index_entry > m_entries;
};

#endif // NV_ECS_HANDLE_MANAGER_HH
