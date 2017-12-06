// Copyright (C) 2017-2017 ChaosForge Ltd
// http://chaosforge.org/

#include "nova-ecs/field_detection.hh"
#include "nova-ecs/ecs.hh"

struct position
{
	int x;
	int y;
};


int main( int argc, char* argv[] )
{

	ecs e;
	e.register_component< position >( "position" );

	handle being = e.create();
	e.add_component< position >( being, 3, 4 );

	e.update( 1.0f );

	return 0;
}
