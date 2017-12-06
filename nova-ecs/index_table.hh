// Copyright (C) 2017-2017 ChaosForge Ltd
// http://chaosforge.org/

#ifndef NV_ECS_INDEX_TABLE_HH
#define NV_ECS_INDEX_TABLE_HH

#include <vector>
#include <unordered_map>
#include "component_storage.hh"

class index_table
{
public:
	virtual int insert( handle h ) = 0;
	virtual bool exists( handle h ) const = 0;
	virtual int get( handle h ) const = 0;
	virtual void swap( handle a, handle b ) = 0;
	virtual int remove_swap( handle h ) = 0;
	virtual int remove_swap_by_index( int dead_eindex ) = 0;
	virtual void clear() = 0;
	virtual int size() const = 0;
};

class flat_index_table : public index_table
{
public:
	explicit flat_index_table( component_storage* storage ) : m_storage( storage ) {}

	int insert( handle h )
	{
		assert( !exists( h ) && "Reinserting handle!" );
		resize_indexes_to( h.index );
		int lindex = m_storage->size();
		m_indexes[h.index] = lindex;
		return lindex;
	}

	bool exists( handle h ) const
	{
		if ( !h || h.index >= m_indexes.size() ) return false;
		return m_indexes[h.index] >= 0;
	}

	int get( handle h ) const
	{
		if ( !h || h.index >= m_indexes.size() ) return -1;
		return m_indexes[h.index];
	}

	void swap( handle a, handle b )
	{
		if ( !a || a.index >= m_indexes.size() || m_indexes[a.index] == -1 ) return;
		if ( !b || b.index >= m_indexes.size() || m_indexes[b.index] == -1 ) return;
		int a_idx = m_indexes[a.index];
		int b_idx = m_indexes[b.index];
		std::swap( m_indexes[a.index], m_indexes[b.index] );
		m_storage->swap( a_idx, b_idx );
	}
		
	int remove_swap( handle h )
	{
		return remove_swap_by_index( m_indexes[h.index] );
	}

	int remove_swap_by_index( int dead_eindex )
	{
		if ( dead_eindex >= m_storage->size() ) return -1;
		int dead_h_index = m_storage->index( dead_eindex );
		m_indexes[dead_h_index] = -1;
		int swap_handle = m_storage->remove_swap( dead_eindex );
		if ( swap_handle != -1 )
			m_indexes[swap_handle] = dead_eindex;
		return dead_eindex;
	}


	void clear()
	{
		m_indexes.clear();
		m_storage->clear();
	}

	int size() const { return m_storage->size(); }

	int find_index( int idx ) const
	{
		for ( int i = 0; i < m_indexes.size(); ++i )
			if ( m_indexes[i] == idx )
				return m_storage->index( i );
		return {};
	}

private:
	void resize_indexes_to( int i )
	{
		int size = m_indexes.size();
		if ( i >= size )
		{
			if ( size == 0 ) size = 1;
			while ( i >= size ) size = size * 2;
			m_indexes.resize( size, -1 );
		}
	}

	std::vector< int > m_indexes;
	component_storage* m_storage = nullptr;
};

class hashed_index_table : public index_table
{
public:
	explicit hashed_index_table( component_storage* storage ) : m_storage( storage ) {}

	int insert( handle h )
	{
		assert( m_indexes.find( h.index ) == m_indexes.end() && "Reinserting handle!" );
		int lindex = m_storage->size();
		m_indexes[h.index] = int( lindex );
		return lindex;
	}

	bool exists( handle h ) const
	{
		auto ih = m_indexes.find( h.index );
		return ( ih != m_indexes.end() );
	}

	int get( handle h ) const
	{
		auto ih = m_indexes.find( h.index );
		if ( ih == m_indexes.end() ) return -1;
		return ih->second;
	}

	void swap( handle a, handle b )
	{
		if ( !a || !b ) return;
		auto ai = m_indexes.find( a.index );
		auto bi = m_indexes.find( b.index );
		if ( ai == m_indexes.end() || bi == m_indexes.end() ) return;
		int a_idx = ai->second;
		int b_idx = bi->second;
		ai->second = b_idx;
		bi->second = a_idx;
		m_storage->swap( a_idx, b_idx );
	}

	int remove_swap( handle h )
	{
		if ( !h ) return -1;
		int dead_h_index = h.index;
		auto ih = m_indexes.find( dead_h_index );
		if ( ih == m_indexes.end() || ih->second == int(-1) ) return -1;
		return remove_swap_by_index( ih->second );
	}

	int remove_swap_by_index( int dead_eindex )
	{
		if ( dead_eindex >= m_storage->size() ) return -1;
		int dead_h_index = m_storage->index(dead_eindex);
		m_indexes.erase( dead_h_index );
		int swap_handle = m_storage->remove_swap( dead_eindex );
		if ( swap_handle != -1 )
			m_indexes[swap_handle] = dead_eindex;
		return dead_eindex;
	}


	void clear()
	{
		m_indexes.clear();
		m_storage->clear();
	}

	int size() const { return m_storage->size(); }

private:

	std::unordered_map< int, int > m_indexes;
	component_storage*             m_storage = nullptr;
};

#endif // NV_ECS_INDEX_TABLE_HH
