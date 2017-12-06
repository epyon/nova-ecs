// Copyright (C) 2017-2017 ChaosForge Ltd
// http://chaosforge.org/

#include "nova-ecs/field_detection.hh"
#include "nova-ecs/ecs.hh"

struct position
{
	int x;
	int y;
};

enum class msg
{
	ACTION,
	UPDATE_TIME,
	DESTROY,
};

struct msg_action
{
	static const int message_id = int( msg::ACTION );
	handle entity;
};

struct msg_update_time
{
	static const int message_id = int( msg::UPDATE_TIME );
	float time;
};

struct msg_destroy
{
	static const int message_id = int( msg::DESTROY );
	handle entity;
};

using msg_list = mpl::list<
	msg_action,
	msg_update_time,
	msg_destroy
>;

using game_ecs = ecs< msg_list >;

struct position_system
{
	void update( game_ecs& ecs, position& p )
	{
		p.x++;
	}

	void on( const msg_action& a, position& p )
	{
		p.y--;
	}
};

int main( int argc, char* argv[] )
{
	game_ecs e;
	e.register_component< position >();
	e.register_system< position_system >();

	handle being = e.create();
	e.add_component< position >( being, 3, 4 );

	e.dispatch< msg_action >( being );

	e.update( 1.0f );

	return 0;
}
