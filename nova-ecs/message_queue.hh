// Copyright (C) 2016-2017 ChaosForge Ltd
// http://chaosforge.org/
//
// This file is part of Nova libraries. 
// For conditions of distribution and use, see copying.txt file in root folder.

/**
* @file message_queue.hh
* @author Kornel Kisielewicz epyon@chaosforge.org
* @brief Data-driven Entity Component System
*/

#ifndef NV_ECS_MESSAGE_QUEUE_HH
#define NV_ECS_MESSAGE_QUEUE_HH

#include <queue>
#include <functional>
#include "handle.hh"
#include "mpl.hh"
#include "field_detection.hh"

template < typename Payload, typename Message >
static const Payload& message_cast( const Message& m )
{
	static_assert( sizeof( Payload ) < sizeof( Message::payload ), "Payload size over limit!" );
	assert( Payload::message_id == m.type && "Payload cast fail!" );
	return *reinterpret_cast<const Payload*>( &( m.payload ) );
}


template < typename MessageList >
class message_queue
{
public:
	typedef float       time_type;
	typedef MessageList message_list;
	typedef unsigned    message_type;

	constexpr static const int message_list_size = mpl::list_size< message_list >::value;

	message_queue()
	{
		m_handlers.resize( message_list_size );
	}

	struct message 
	{
		message_type type;
		int          recursive;
		time_type    time;
		char         payload[128 - sizeof( message_type ) - sizeof( time_type ) - sizeof( int ) ];
	};

	struct message_compare_type
	{
		bool operator()( const message& l, const message& r )
		{
			return l.time > r.time;
		}
	};

	using message_handler = std::function< void( const message& ) >;
				
	struct message_handlers
	{
		std::vector< message_handler > list;
	};

	typedef std::priority_queue< message, std::vector< message >, message_compare_type > queue_type;

	template< typename Handler >
	void register_handler( Handler* c )
	{
		register_messages< Handler > ( (Handler*)( c ), message_list{} );
	}

	void register_handler( void* )
	{
	}

	bool dispatch( const message& m )
	{
		for ( auto& h : m_handlers[m.type].list )
			h( m );
		return true;
	}

	template < typename Payload, typename ...Args >
	bool dispatch( Args&&... args )
	{
		message m{ Payload::message_id, 0, time_type( 0 ) };
		new( &m.payload ) Payload{ std::forward<Args>( args )... };
		return dispatch( m );
	}

	template < typename Payload, typename ...Args >
	bool dispatch_recursive( Args&&... args )
	{
		message m{ Payload::message_id, 1, time_type( 0 ) };
		new( &m.payload ) Payload{ std::forward<Args>( args )... };
		return dispatch( m );
	}

	bool queue( const message& m )
	{
		m_pqueue.push( m );
		return true;
	}

	template < typename Payload, typename ...Args >
	bool queue( time_type delay, Args&&... args )
	{
		message m{ Payload::message_id, 0, m_time + delay };
		new( &m.payload ) Payload{ std::forward<Args>( args )... };
		return queue( m );
	}

	template < typename Payload, typename ...Args >
	bool queue_recursive( time_type delay, Args&&... args )
	{
		message m{ Payload::message_id, 1, m_time + delay };
		new( &m.payload ) Payload{ nv::forward<Args>( args )... };
		return queue( m );
	}

	time_type get_time() const { return m_time; }

	bool events_pending() const
	{
		return !m_pqueue.empty();
	}

	const message& top_event() const
	{
		return m_pqueue.top();
	}

	void reset_events()
	{
		m_pqueue.clear();
		m_time = time_type( 0 );
	}

	void update_time( time_type dtime )
	{
		if ( dtime == time_type( 0 ) ) return;
		m_time += dtime;
		while ( !m_pqueue.empty() && m_pqueue.top().time <= m_time )
		{
			message msg = m_pqueue.top();
			m_pqueue.pop();
			dispatch( msg );
		}
	}

	time_type update_step()
	{
		if ( !m_pqueue.empty() )
		{
			message msg = m_pqueue.top();
			m_time = msg.time;
			m_pqueue.pop();
			dispatch( msg );
		}
		return m_time;
	}

	void register_callback( message_type msg, message_handler&& handler )
	{
		m_handlers[msg].list.push_back( handler );
	}

protected:

	template < typename System, template <class...> class List, typename... Messages >
	void register_messages( System* h, List<Messages...>&& )
	{
		int unused[] = { ( register_message<System,Messages>( h ), 0 )... };
	}


	template < typename System, typename Message >
	void register_message( System* s )
	{
		if constexpr( has_message< System, Message > )
			register_callback( Message::message_id, [=] ( const message& msg )
			{
				s->on( message_cast<Message>( msg ) );
			} );
	}

	time_type                       m_time = time_type( 0 );
	queue_type                      m_pqueue;
	std::vector< message_handlers > m_handlers;
};

#endif // NV_ECS_MESSAGE_QUEUE_HH
