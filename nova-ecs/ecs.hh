// Copyright (C) 2016-2017 ChaosForge Ltd
// http://chaosforge.org/
//
// This file is part of Nova libraries.
// For conditions of distribution and use, see copying.txt file in root folder.

/**
* @file ecs.hh
* @author Kornel Kisielewicz epyon@chaosforge.org
* @brief Data-driven Entity Component System
*/

#ifndef NV_ECS_HH
#define NV_ECS_HH

#include <type_traits>
#include <string>
#include <typeinfo>
#include <vector>
#include <unordered_map>
#include "handle.hh"
#include "index_table.hh"
#include "message_queue.hh"
#include "handle_manager.hh"
#include "handle_tree_manager.hh"
#include "component_storage.hh"

template < typename Enumerator >
class enumerator_provider
{
public:
	struct terminator {};

	struct iterator : public Enumerator
	{
		using Enumerator::Enumerator;
		friend auto operator!=( iterator lhs, terminator ) { return !lhs.done(); }
		auto& operator*() { return Enumerator::current(); }
		auto& operator++() { Enumerator::next(); return *this; }
	};

	template< typename ...Args >
	enumerator_provider( Args&&... args )
		: m_begin( std::forward< Args >( args )... ) {}
	iterator begin() const { return m_begin; }
	terminator end() const { return terminator(); }
protected:
	iterator m_begin;
};

template < typename MessageList >
class ecs : public message_queue< MessageList >
{
public:
	typedef ecs< MessageList >   this_type;

	using message_queue< MessageList >::message_list;
	using message_queue< MessageList >::message_type;
	using message_queue< MessageList >::message;

	using destructor_handler = std::function< void() >;
	using update_handler     = std::function< void( float ) >;
	using destroy_handler    = std::function< void( void* ) >;
	using create_handler     = std::function< void( handle, void* ) >;

	class component_interface
	{
	public:
		void* get_raw( handle h )
		{
			int i = m_index->get( h );
			return i >= 0 ? m_storage->raw(i) : nullptr;
		}

		const void* get_raw( handle h ) const
		{
			int i = m_index->get( h );
			return i >= 0 ? m_storage->raw( i ) : nullptr;
		}

		bool               m_relational;
		index_table*       m_index;
		component_storage* m_storage;

		std::vector< create_handler >  m_create;
		std::vector< destroy_handler > m_destroy;
	};

	class enumerator
	{
	public:
		explicit enumerator( const ecs& aecs, handle current = handle() ) : m_ecs( aecs ), m_current( current ) {}
		bool done() const { return !m_current; }
		void next() { m_current = m_ecs.next( m_current ); }
		const handle& current() { return m_current; }
	private:
		const ecs& m_ecs;
		handle     m_current;
	};

	template< typename Component >
	class recursive_component_enumerator
	{
	public:
		explicit recursive_component_enumerator( ecs& aecs, handle root ) : m_ecs( aecs ), m_root( root )
		{
			m_current   = m_root;
			m_interface = m_ecs.get_interface< Component >();
			if ( m_interface && m_current )
				next();
		}
		bool done() const { return m_component == nullptr; }
		Component& current() { return *m_component; }
		void next()
		{
			uint32 index = next_index();
			m_component = index >= 0 ? (Component*)m_interface->m_storage->raw( index ) : nullptr;
		}

	private:
		int next_index()
		{
			for ( ;;)
			{
				if ( !m_current ) return -1;
				int index = m_interface->m_index->get( m_current );
				m_current = m_ecs.next_handle( m_current, m_root );
				if ( index >= 0 ) return index;
			}
		}

		ecs&  m_ecs;
		handle          m_root;
		handle          m_current;
		component_interface* m_interface = nullptr;
		Component*           m_component = nullptr;
	};

	template < typename Component >
	handle handle_cast( const Component& c )
	{
		return *reinterpret_cast<const handle*>( &c );
	}

	template < int, typename... Components >
	struct gather_components
	{
		static constexpr int SIZE = sizeof...( Components );
		component_interface* cis[SIZE];
		void* cmps[SIZE];

		gather_components( this_type& ecs )
		{
			fill< 0, Components... >( ecs );
		}

		bool run( handle h )
		{
			for ( uint32 i = 0; i < SIZE; ++i )
			{
				cmps[i] = cis[i]->get_raw( h );
				if ( !cmps[i] ) return false;
			}
			return true;
		}

		template< typename Component >
		bool runc( Component& c )
		{
			return run( *reinterpret_cast<const handle*>( &c ) );
		}

		template < typename SC >
		SC& get()
		{
			return run_get< 0, SC, Components...>();
		}

	private:
		template < int Index, typename C, typename... Cs >
		void fill( this_type& ecs )
		{
			cis[Index] = ecs.get_interface< C >();
			assert( cis[Index] && "What the f*** is this?" );
			fill< Index + 1, Cs... >( ecs );
		}

		template < int Index >
		void fill( this_type& ) {}

		template < int Index, typename SC, typename C, typename... Cs >
		C& run_get()
		{
			return get_impl< Index, SC, C, Cs... >( std::is_same< SC, C >{} );
		}

		template < int Index, typename SC, typename C, typename C2, typename... Cs >
		C& get_impl( std::false_type&& )
		{
			return get_impl< Index + 1, SC, C2, Cs...>( std::is_same< SC, C >() );
		}

		template < int Index, typename SC, typename C, typename... Cs >
		C& get_impl( std::true_type&& )
		{
			return *(C*)cmps[Index];
		}

	};

	template <int I>
	struct gather_components<I>
	{
		gather_components( this_type& ) {}
		bool run( handle ) { return true;  }
		template < typename Component >
		bool runc( Component& ) { return true; }
	};

	template< typename System, typename... Args >
	System* register_system( const std::string& name, Args&&... args )
	{
		System* result = new System( std::forward< Args >( args )... );

		this->template register_handler< System >( name, result );
		if constexpr ( has_components< System > )
			register_component_helper< System >( result );
		register_ecs_messages< System >( result, message_list() );
		if constexpr (has_ecs_update< this_type, System, float >)
			register_ecs_update< System >( result );
		m_cleanup.emplace_back( [=] () { delete result; } );
		return result;
	}

	template< typename System >
	void register_component_helper( System* c )
	{
		using component_list = typename System::components;
		register_component_messages< System, component_list >( c, message_list() );
		if constexpr(has_ecs_component_update< this_type, System, component_list, float >)
			register_ecs_component_update< System >( c, component_list() );
		if constexpr(has_component_update< System, component_list, float > )
			register_component_update< System >( c, component_list() );
		if constexpr(has_destroy< System, mpl::head<component_list> >)
			register_destroy< System, mpl::head<component_list> >( (System*)(c) );
		if constexpr(has_create< System, mpl::head<component_list>, handle >)
			register_create< System, mpl::head<component_list>, handle >( (System*)(c) );
	}

	template < typename Component, typename IndexTable = flat_index_table >
	void register_component( const std::string& name, bool relational = false )
	{
		component_interface* result = new component_interface;
		result->m_relational = relational;
		result->m_storage = new component_storage_handler< Component >( relational );
		result->m_index = new IndexTable( result->m_storage );

		m_cstorage.push_back( result->m_storage );
		m_cmap[&typeid(Component)] = result->m_storage;
		m_cmap_by_name[name] = result->m_storage;

		m_components.push_back( result );
		m_component_map[&typeid(Component)] = result;

		m_cleanup.emplace_back( [=] () { delete result; } );
	}

	handle create()
	{
		return m_handles.create_handle();
	}

	void update( float dtime )
	{
		this->update_time( dtime );
		for ( auto u : m_update_handlers )
			u( dtime );
		for ( auto h : m_dead_handles )
			remove( h );
		m_dead_handles.clear();
	}

	void clear()
	{
		this->reset_events();
		for ( auto c : m_components )
		{
			for ( int i = 0; i < c->m_storage->size(); ++i )
				call_destructors( c, c->m_storage->raw( i ) );
			c->m_storage->clear();
			c->m_index->clear();
		}
		m_handles.clear();
	}

	~ecs()
	{
		for ( auto ci : m_components )
			delete ci->m_index;

		if ( !m_cleanup.empty() )
		for ( int i = (int)m_cleanup.size() - 1; i >= 0; --i )
			m_cleanup[i]();
		m_cleanup.clear();
		m_update_handlers.clear();

		for ( auto s : m_cstorage )
			delete s;
		m_cstorage.clear();
	}

	bool attach( handle parent, handle child )
	{
		if ( !m_handles.attach( parent, child ) )
			return false;
		for ( auto c : m_components )
			if ( c->m_relational )
				relational_recursive_rebuild( c, child );
		return true;
	}

	void detach( handle handle )
	{
		m_handles.detach( handle );
	}


	template< typename Component >
	int get_debug_index( handle h )
	{
		return get_interface<Component>()->m_index->get( h );
	}

	handle get_parent( handle h ) const
	{
		return h ? m_handles.get_parent( h ) : handle();
	}

	handle next( handle h ) const
	{
		return h ? m_handles.next( h ) : handle();
	}

	handle first_child( handle h ) const
	{
		return h ? m_handles.first( h ) : handle();
	}

	handle next_handle( handle current, handle root )
	{
		if ( handle child = first_child( current ) )
			return child;
		for ( ;;) {
			if ( !current || current == root ) return handle();
			if ( handle nxt = next( current ) )
				return nxt;
			current = get_parent( current );
		};
	}

	bool is_valid( handle h ) const
	{
		return m_handles.is_valid( h );
	}

	enumerator_provider< enumerator > children( handle h ) const
	{
		return enumerator_provider< enumerator >( *this, first_child( h ) );
	}

	template< typename Component >
	enumerator_provider< recursive_component_enumerator< Component > > recursive_components( handle h )
	{
		return enumerator_provider< recursive_component_enumerator< Component > >( *this, first_child( h ) );
	}

	template < typename C, typename F >
	void remove_component_if( F&& f )
	{
		auto storage = get_storage<C>();
		auto temp_component = get_interface<C>();
		uint32 i = 0;
		while ( i < storage->size() )
			if ( f( ( *storage )[i] ) )
				remove_component_by_index( temp_component, i );
			else
				++i;
	}

	template < typename C>
	void remove_component( handle h )
	{
		auto temp_component = get_interface<C>();
		remove_component( get_interface<C>(), h );
	}

	template < typename C, typename F >
	void for_each( F&& f )
	{
		auto storage = get_storage<C>();
		assert( storage && "Invalid component" );
		for ( auto& c : *storage )
		{
			f( c );
		}
	}


	template < typename F >
	void recursive_call( handle h, F&& f )
	{
		f( h );
		for ( auto c : children( h ) )
			recursive_call( c, f );
	}

	template < typename Component, typename F >
	void recursive_component_call( handle h, F&& f )
	{
		if ( auto* cmp = get<Component>( h ) )
			f( cmp );
		for ( auto c : children( h ) )
			recursive_component_call<Component>( c, f );
	}


	void mark_remove( handle h )
	{
		m_dead_handles.push_back( h );
	}

	void remove( handle h )
	{
		handle ch = m_handles.first( h );
		while ( handle r = ch )
		{
			ch = m_handles.next( ch );
			remove( r );
		}
		for ( auto c : m_components )
			remove_component( c, h );
		m_handles.free_handle( h );
	}

	bool exists( handle h ) const
	{
		return m_handles.is_valid( h );
	}

	template < typename Component >
	Component* get( handle h )
	{
		auto it = m_component_map.find( &typeid(Component) );
		assert( it != m_component_map.end() && "Get fail!" );
		return static_cast<Component*>( it->second->get_raw( h ) );
	}

	template < typename Component >
	const Component* get( handle h ) const
	{
		auto it = m_component_map.find( &typeid(Component) );
		assert( it != m_component_map.end() && "Get fail!" );
		return static_cast<const Component*>( it->second->get_raw( h ) );
	}

	template < typename Component >
	component_storage_handler< Component >*
		get_storage()
	{
		return storage_cast<Component>( m_cmap[&typeid(Component)] );
	}

	template < typename Component >
	const component_storage_handler< Component >* get_storage() const
	{
		return storage_cast<Component>( m_cmap[&typeid(Component)] );
	}

	template < typename Component, typename ...Args >
	Component& add_component( handle h, Args&&... args )
	{
		component_interface* ci = get_interface<Component>();
		auto* cs = get_storage<Component>();
		int i = ci->m_index->insert( h );
		assert( i == int( cs->size() ) && "Fail!" );
		return cs->append<Component>( h.index, std::forward<Args>( args )... );
	}

	template < typename Component, typename ...Args >
	Component& update_or_create( handle h, Args&&... args )
	{
		auto* hc = get< Component >( h );
		if ( hc == nullptr ) 
			return add_component<Component>( h, std::forward<Args>( args )... );
		*hc = Component{ std::forward<Args>( args )... };
		return *hc;
	}

	template < typename Component >
	Component& get_or_create( handle h )
	{
		auto* hc = get< Component >( h );
		if ( hc == nullptr )
			return add_component<Component>( h );
		return *hc;
	}

	template < typename System, typename Components, template <class...> class List, typename... Messages >
	void register_component_messages( System* h, List<Messages...>&& )
	{
		int unused_0[] = { ( register_ecs_component_message<System,Messages>( h, Components() , std::bool_constant< has_ecs_component_message< this_type, System, Components, Messages > >() ), 0 )... };
		int unused_1[] = { ( register_component_message<System,Messages>( h, Components(), std::bool_constant< has_component_message< System, Components, Messages > >() ), 0 )... };
	}

	template < typename System, template <class...> class List, typename... Messages >
	void register_ecs_messages( System* h, List<Messages...>&& )
	{
		int unused_0[] = { ( register_ecs_message<System,Messages>( h, std::bool_constant< has_ecs_message< this_type, System, Messages > >() ), 0 )... };
	}

	template < typename System, typename Message, typename C, typename... Cs >
	void register_component_message( System* s, mpl::list< C, Cs...>&&, std::true_type&& )
	{
		component_interface* ci = get_interface<C>();
		this->register_callback( Message::message_id, [=] ( const message& msg )
		{
			const Message& m = message_cast<Message>( msg );
			auto callback = [=] ( handle h )
			{
				if ( void* c = ci->get_raw( h ) )
				{
					gather_components<0, Cs... > gather( *this );
					if ( gather.run( h ) )
						s->on( m, *( (C*)( c ) ), gather.template get<Cs>()... );
				}
			};
			if ( msg.recursive )
				this->recursive_call( m.entity, std::move( callback ) );
			else
				callback( m.entity );

		} );
	}

	template < typename System, typename Message, typename C, typename... Cs >
	void register_ecs_component_message( System* s, mpl::list< C, Cs...>&&, std::true_type&& )
	{
		component_interface* ci = get_interface<C>();
		this->register_callback( Message::message_id, [=] ( const message& msg )
		{
			const Message& m = message_cast<Message>( msg );
			auto callback = [=] ( handle h )
			{
				if ( void* c = ci->get_raw( h ) )
				{
					gather_components<0, Cs... > gather( *this );
					if ( gather.run( h ) )
						s->on( m, *this, *( (C*)( c ) ), gather.template get<Cs>()... );
				}
			};
			if ( msg.recursive )
				this->recursive_call( m.entity, callback );
			else
				callback( m.entity );
		} );

	}

	template < typename System, typename Message >
	void register_ecs_message( System* s, std::true_type&& )
	{
		this->register_callback( Message::message_id, [=] ( const message& msg )
		{
			s->on( message_cast<Message>( msg ), *this );
		} );
	}

	template < typename System, typename Message, typename... Cs >
	void register_component_message( System*, mpl::list< Cs...>&&, std::false_type&& ) {}

	template < typename System, typename Message, typename... Cs >
	void register_ecs_component_message( System*, mpl::list< Cs...>&&, std::false_type&& ) {}

	template < typename System, typename Message >
	void register_ecs_message( System*, std::false_type&& ) {}

	template < typename System >
	void register_ecs_update( System* s )
	{
		register_update( [=] ( float dtime )
		{
			s->update( *this, dtime );
		} );
	}

	template < typename System, typename Component >
	void register_destroy( System* s )
	{
		component_interface* ci = get_interface<Component>();
		assert( ci && "Unregistered component!" );
		ci->m_destroy.push_back( [=] ( void* data )
		{
			s->destroy( *((Component*)data) );
		}
		);
	}

	template < typename System, typename Component, typename H >
	void register_create( System* s )
	{
		component_interface* ci = get_interface<Component>();
		assert( ci && "Unregistered component!" );
		ci->m_create.push_back( [=] ( H h, void* data, const lua::stack_proxy& p )
		{
			s->create( h, *( (Component*)data ), p );
		}
		);
	}

	template < typename System, typename C, typename... Cs >
	void register_component_update( System* s, mpl::list< C, Cs...>&& )
	{
		auto* storage = get_storage<C>();
		register_update( [=] ( float dtime )
		{
			gather_components<0, Cs... > gather( *this );
			for ( auto& c : *storage )
			{
				if ( gather.runc( c ) )
					s->update( c, gather.template get<Cs>()..., dtime );
			}
		} );
	}

	template < typename System, typename C, typename... Cs >
	void register_ecs_component_update( System* s, mpl::list< C, Cs...>&& )
	{
		auto* storage = get_storage<C>();
		register_update( [=] ( float dtime )
		{
			gather_components<0, Cs... > gather(*this);
			for ( auto& c : *storage )
			{
				if ( gather.runc( c ) )
					s->update( *this, c, gather.template get<Cs>()..., dtime );
			}
		} );
	}

	void register_update( update_handler&& handler )
	{
		m_update_handlers.push_back( handler );
	}

	template < typename Component >
	component_interface* get_interface()
	{
		return m_component_map[&typeid(Component)];
	}

	template < typename Component >
	const component_interface* get_interface() const
	{
		return m_component_map[&typeid(Component)];
	}

protected:

	void relational_rebuild( component_interface* ci, int i )
	{
		handle h = *(handle*)(ci->m_storage->raw( i ));
		handle p = get_parent( h );
		if ( !p ) return;
		if ( i < ci->m_index->get( p ) )
		{
			ci->m_index->swap( h, p );
			relational_rebuild( ci, i );
		}
	}

	void relational_recursive_rebuild( component_interface* ci, handle h )
	{
		handle p = get_parent( h );
		if ( !p ) return;
		if ( ci->m_index->get( h ) < ci->m_index->get( p ) )
		{
			ci->m_index->swap( h, p );
			for ( auto c : children( h ) )
				relational_recursive_rebuild( ci, c );
		}
	}

	void call_destructors( component_interface* ci, void* data )
	{
		for ( auto dh : ci->m_destroy )
			dh( data );
	}

	void remove_component_by_index( component_interface* ci, int i )
	{
		if ( i > ci->m_storage->size() ) return;
		call_destructors( ci, ci->m_storage->raw( i ) );
		int dead_eindex = ci->m_index->remove_swap_by_index( i );
		if ( ci->m_relational )
			relational_rebuild( ci, dead_eindex );
	}

	void remove_component( component_interface* ci, handle h )
	{
		void* v = ci->get_raw( h );
		if ( v == nullptr ) return;
		call_destructors( ci, v );
		int dead_eindex = ci->m_index->remove_swap( h );
		if ( ci->m_relational )
			relational_rebuild( ci, dead_eindex );
	}

	handle_tree_manager                              m_handles;
	std::vector< handle >                            m_dead_handles;
	std::vector< component_interface* >              m_components;
	std::unordered_map< const std::type_info*, component_interface* > m_component_map;
	std::vector< update_handler >                    m_update_handlers;

	std::vector< component_storage* >                m_cstorage;
	std::unordered_map< const std::type_info*, component_storage* >   m_cmap;
	std::unordered_map< std::string, component_storage* >   m_cmap_by_name;

	std::vector< component_interface* >              m_temp_components;
	std::vector< destructor_handler >                m_cleanup;
};

#endif // NV_ECS_HH
