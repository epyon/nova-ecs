// Copyright (C) 2017-2017 ChaosForge Ltd
// http://chaosforge.org/
//
// This file is part of Nova libraries. 
// For conditions of distribution and use, see copying.txt file in root folder.

/**
* @file component_storage.hh
* @author Kornel Kisielewicz epyon@chaosforge.org
* @brief Data-driven Entity Component System
*/

#ifndef NV_ECS_COMPONENT_STORAGE_HH
#define NV_ECS_COMPONENT_STORAGE_HH

#include <algorithm>
#include "handle.hh"
#include "handle_manager.hh"

template < typename T, typename ...Args >
inline void raw_construct_object( void* object, Args&&... params )
{
	new (object)T( std::forward<Args>( params )... );
}

template < typename T >
void raw_destroy_object( void* object )
{
	static_cast<T*>(object)->T::~T();
}

using constructor_t = void( *)(void*);
using destructor_t  = void( *)(void*);

class component_storage
{
protected:
	component_storage() {}
	template < typename T >
	void initialize( bool owner_included )
	{
		m_data = nullptr;
		m_size = 0;
		m_allocated = 0;
		m_constructor = raw_construct_object < T >;
		m_destructor  = raw_destroy_object < T >;
		m_csize = sizeof( T );
		m_owner_data = owner_included;
	}
public:
	void reserve( int count )
	{
		reallocate( count );
	}
	int size() const { return m_size; }
	int raw_size() const { return m_size * m_csize; }
	void reset()
	{
		clear();
		free( m_data );
		free( m_indices );
		m_data = nullptr;
		m_indices = nullptr;
		m_allocated = 0;
	}
	void clear()
	{
		int   count = m_size;
		char* d     = m_data;
		for ( ; count > 0; --count, d += m_csize )
			m_destructor(d);
		m_size = 0;
	}
	void* raw() { return m_data; }
	const void* raw() const { return m_data; }
	void* raw( int i ) { return m_data + m_csize * i; }
	const void* raw( int i ) const { return m_data + m_csize * i; }
	int index( int i ) const
	{
		return m_indices ? m_indices[i] : ((handle*)( m_data + m_csize * i ))->index;
	}
	template < typename T >
	T* as() { return (T*)m_data; }
	template < typename T >
	const T* as() const { return m_data; }

	void pop_back()
	{
		assert( m_size > 0 && "BAD OP!" );
		m_size--;
		m_destructor( m_data + m_size * m_csize );
	}


	template < typename T, typename... Args >
	T& append( int index, Args&&... args )
	{
		grow();
		T* result = (T*)m_data + sint32( m_size ) - 1;
		construct_object<T>( result, forward<Args>( args )... );
		if ( m_indices )
			m_indices[ m_size - 1 ] = index;
		return *result;
	}

	template < typename T >
	T& back()
	{
		assert( m_size > 0 && "EMPTY COMPONENT STORAGE" );
		T* result = (T*)m_data + sint32( m_size ) - 1;
		return *result;
	}

	void* raw_add( int index )
	{
		grow();
		void* result = m_data + ( m_size - 1 ) * m_csize;
		m_constructor( result );
		if ( m_indices )
			m_indices[m_size - 1] = index;
		return result;
	}

	int remove_swap( int dead_eindex )
	{
		if ( dead_eindex >= m_size ) return -1;
		int swap_handle  = index( m_size - 1 );
		if ( dead_eindex == m_size - 1 )
		{
			swap_handle = -1;
		}
		pop_swap( dead_eindex );
		return swap_handle;
	}

	void pop_swap( int a )
	{
		if ( m_size == 0 ) return;
		m_size--;
		char* ia = m_data + m_csize * a;
		char* ie = m_data + m_csize * m_size;
		m_destructor( ia );
		if ( ia >= ie ) return;
		memmove( ia, ie, m_csize );
		if ( m_indices )
			m_indices[a] = m_indices[m_size];
	}

	void swap( int a, int b )
	{
		// this could be optimized
		char* ia = m_data + m_csize * a;
		char* ib = m_data + m_csize * b;
		std::swap_ranges( ia, ia + m_csize, ib );
		if ( m_indices )
			std::swap( m_indices[a], m_indices[b] );
	}



	~component_storage()
	{
		reset();
	}
protected:
	void grow()
	{
		int new_size = m_size + 1;
		if ( new_size > m_allocated )
			reallocate( m_allocated > 3 ? m_allocated * 2 : 8 );
		m_size = new_size;
	}

	void reallocate( int new_size )
	{
		m_data    = (char*)( realloc( m_data, new_size * m_csize ) );
		if ( !m_owner_data )
			m_indices = (int*)(realloc( m_indices, new_size * sizeof( int ) ));
		m_allocated = new_size;
		assert( m_data );
	}

	int       m_csize = 0;
	int       m_allocated = 0;
	int       m_size = 0;
	bool      m_owner_data = false;
	char*    m_data = nullptr;
	int*     m_indices = nullptr;

	constructor_t m_constructor = nullptr;
	destructor_t  m_destructor = nullptr;
};

template < typename Component >
class component_storage_handler : public component_storage
{
public:
	typedef Component         value_type;
	typedef Component*        iterator;
	typedef const Component*  const_iterator;
	typedef Component&        reference;
	typedef const Component&  const_reference;

	component_storage_handler( bool owner_stored ) 
	{
		initialize<Component>( owner_stored );
	}
	Component* data() { return (Component*)m_data; }
	const Component* data() const { return (Component*)m_data; }
	inline const Component& operator[] ( int i ) const { return ((Component*)m_data)[i]; }
	inline Component& operator[] ( int i ) { return ( (Component*)m_data )[i]; }

	inline iterator        begin() { return (Component*)m_data; }
	inline const_iterator  begin()  const { return (const Component*)m_data; }
	inline void*           raw_begin() { return m_data; }
	inline const void*     raw_begin() const { return m_data; }

	inline iterator        end() { return ((Component*)m_data)+m_size; }
	inline const_iterator  end()  const { return ( (Component*)m_data ) + m_size; }
	inline void*           raw_end() { return ( (Component*)m_data ) + m_size; }
	inline const void*     raw_end() const { return ( (Component*)m_data ) + m_size; }
};

template < typename Component >
component_storage_handler< Component >* storage_cast( component_storage* storage )
{
	// TODO: error checking
	return ( component_storage_handler< Component >* )storage;
}

#endif // NV_ECS_COMPONENT_STORAGE_HH
