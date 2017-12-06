// Copyright (C) 2017-2017 ChaosForge Ltd
// http://chaosforge.org/

#ifndef NV_ECS_HANDLE_TREE_MANAGER_HH
#define NV_ECS_HANDLE_TREE_MANAGER_HH

#include <vector>
#include <cassert>
#include "handle.hh"

class handle_tree_manager
{
	typedef int index_type;
	static const index_type NONE = index_type( -1 );
	static const index_type USED = index_type( -2 );
public:

	typedef unsigned value_type;

	handle_tree_manager()
		: m_first_free( NONE ), m_last_free( NONE ) {}

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
		remove( h );
		value_type index = h.index;
		m_entries[index].next_free = NONE;
		if ( m_last_free == NONE )
		{
			m_first_free = m_last_free = index_type(index);
			return;
		}
		m_entries[value_type(m_last_free)].next_free = index_type(index);
		m_last_free = index_type(index);
	}

	bool attach( handle parent, handle child )
	{
		value_type pindex = parent.index;
		value_type cindex = child.index;
		if ( m_entries[cindex].parent == pindex )
			return false;
		if ( m_entries[cindex].parent != NONE )
			detach( child );
		m_entries[cindex].parent = pindex;
		m_entries[cindex].next_sibling = m_entries[pindex].first_child;
		value_type nindex = m_entries[cindex].next_sibling;
		if ( nindex != NONE )
			m_entries[nindex].prev_sibling = cindex;
		m_entries[cindex].prev_sibling = NONE;
		m_entries[pindex].first_child = cindex;
		return true;
	}

	handle get_parent( handle h ) const
	{
		assert( is_valid( h ) && "INVALID HANDLE" );
		value_type pindex = m_entries[h.index].parent;
		return pindex == NONE ? handle() : handle( pindex, m_entries[pindex].counter );
	}

	handle next( handle h ) const
	{
		assert( is_valid( h ) && "INVALID HANDLE" );
		value_type nindex = m_entries[h.index].next_sibling;
		return nindex == NONE ? handle() : handle( nindex, m_entries[nindex].counter );
	}

	handle first( handle h ) const
	{
		assert( is_valid( h ) && "INVALID HANDLE" );
		value_type nindex = m_entries[h.index].first_child;
		return nindex == NONE ? handle() : handle( nindex, m_entries[nindex].counter );
	}


	void remove( handle h )
	{
		assert( m_entries[h.index].first_child == NONE && "Remove called on handle with children!" );
		detach( h );
	}

	void remove_and_orphan( handle h )
	{
		detach( h );
		value_type index = h.index;
		while ( m_entries[index].first_child != NONE )
		{
			value_type child = m_entries[index].first_child;
			m_entries[index].first_child = m_entries[child].next_sibling;
			m_entries[child].parent = NONE;
			m_entries[child].next_sibling = NONE;
			m_entries[child].prev_sibling = NONE;
		}
	}

	void detach( handle h )
	{
		value_type index = h.index;
		index_type pindex = m_entries[index].parent;
		index_type next_index = m_entries[index].next_sibling;
		index_type prev_index = m_entries[index].prev_sibling;

		m_entries[index].parent = NONE;
		m_entries[index].next_sibling = NONE;
		m_entries[index].prev_sibling = NONE;

		if ( pindex == NONE )
		{
			assert( next_index == NONE && "Hierarchy fail! next_index" );
			assert( prev_index == NONE && "Hierarchy fail! prev_index" );
			return;
		}
		if ( value_type( m_entries[pindex].first_child ) == index )
		{
			assert( prev_index == NONE && "Hierarchy fail! prev_index" );
			m_entries[pindex].first_child = next_index;
			if ( next_index == NONE ) // only child
				return;
			m_entries[next_index].prev_sibling = NONE;
		}
		else
		{
			assert( prev_index != NONE && "Hierarchy fail! prev_index" );
			if ( next_index != NONE )
				m_entries[next_index].prev_sibling = prev_index;
			if ( prev_index != NONE )
				m_entries[prev_index].next_sibling = next_index;
		}
	}

	bool is_valid( handle h ) const
	{
		if ( !h ) return false;
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

private:
	struct index_entry
	{
		value_type counter;
		index_type next_free;

		index_type parent;
		index_type first_child;
		index_type next_sibling;
		index_type prev_sibling;

		index_entry()
			: counter( 0 )
			, next_free( NONE )
			, parent( NONE )
			, first_child( NONE )
			, next_sibling( NONE )
			, prev_sibling( NONE )
		{}
	};

	value_type get_free_entry()
	{
		if ( m_first_free != NONE )
		{
			value_type result = static_cast<value_type>(m_first_free);
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

#endif // NV_ECS_HANDLE_TREE_MANAGER_HH
