/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

/*****************************************************************************
 * name:		be_ea.c
 *
 * desc:		elementary actions - Python AI Bridge aware
 *
 * The QVM still calls all EA_* functions, but they are no-ops by default.
 * When the Python AI bridge applies commands, it sets ea_bridge_active = 1
 * so the real implementations are used.
 *
 *****************************************************************************/

#include "../qcommon/q_shared.h"
#include "l_memory.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "botlib.h"
#include "be_interface.h"
#include "be_ea.h"

#define MAX_USERMOVE				400
#define MAX_COMMANDARGUMENTS		10

bot_input_t *botinputs;

// When set to 1, EA functions actually write to botinputs.
// When 0, they are no-ops (QVM calls are ignored).
// The bridge sets this to 1 before applying Python commands.
int ea_bridge_active = 0;

// ============================================================================
// QVM-called functions: no-ops unless bridge is active
// Communication functions are always no-ops (no QVM chat, no Python chat yet)
// ============================================================================

void EA_Say( int client, const char *str ) {}
void EA_SayTeam( int client, const char *str ) {}
void EA_Tell( int client, int clientto, const char *str ) {}
void EA_UseItem( int client, const char *it ) {}
void EA_DropItem( int client, const char *it ) {}
void EA_UseInv( int client, const char *inv ) {}
void EA_DropInv( int client, const char *inv ) {}
void EA_Gesture( int client ) {}
void EA_Command( int client, const char *command ) {}
void EA_Talk( int client ) {}
void EA_DelayedJump( int client ) {}
void EA_Walk( int client ) {}
void EA_MoveUp( int client ) {}
void EA_MoveDown( int client ) {}
void EA_MoveLeft( int client ) {}
void EA_MoveRight( int client ) {}
void EA_MoveForward( int client ) {}
void EA_MoveBack( int client ) {}

// ============================================================================
// Bridge-aware functions: work when ea_bridge_active == 1
// These are the functions the Python AI bridge uses to control bots
// ============================================================================

void EA_SelectWeapon( int client, int weapon ) {
	if ( !ea_bridge_active ) return;
	botinputs[client].weapon = weapon;
}

void EA_Attack( int client ) {
	if ( !ea_bridge_active ) return;
	botinputs[client].actionflags |= ACTION_ATTACK;
}

void EA_Use( int client ) {
	if ( !ea_bridge_active ) return;
	botinputs[client].actionflags |= ACTION_USE;
}

void EA_Respawn( int client ) {
	if ( !ea_bridge_active ) return;
	botinputs[client].actionflags |= ACTION_RESPAWN;
}

void EA_Jump( int client ) {
	if ( !ea_bridge_active ) return;
	if ( botinputs[client].actionflags & ACTION_JUMPEDLASTFRAME ) {
		botinputs[client].actionflags &= ~ACTION_JUMP;
	} else {
		botinputs[client].actionflags |= ACTION_JUMP;
	}
}

void EA_Crouch( int client ) {
	if ( !ea_bridge_active ) return;
	botinputs[client].actionflags |= ACTION_CROUCH;
}

void EA_Action( int client, int action ) {
	if ( !ea_bridge_active ) return;
	botinputs[client].actionflags |= action;
}

void EA_Move( int client, vec3_t dir, float speed ) {
	if ( !ea_bridge_active ) return;
	VectorCopy( dir, botinputs[client].dir );
	if ( speed > MAX_USERMOVE ) speed = MAX_USERMOVE;
	else if ( speed < -MAX_USERMOVE ) speed = -MAX_USERMOVE;
	botinputs[client].speed = speed;
}

void EA_View( int client, vec3_t viewangles ) {
	if ( !ea_bridge_active ) return;
	VectorCopy( viewangles, botinputs[client].viewangles );
}

// ============================================================================
// Infrastructure functions: always active (needed for engine stability)
// ============================================================================

void EA_EndRegular( int client, float thinktime ) {
}

void EA_GetInput( int client, float thinktime, bot_input_t *input ) {
	bot_input_t *bi;

	bi = &botinputs[client];
	bi->thinktime = thinktime;
	Com_Memcpy( input, bi, sizeof(bot_input_t) );
}

void EA_ResetInput( int client ) {
	bot_input_t *bi;
	int jumped;

	bi = &botinputs[client];

	bi->thinktime = 0;
	VectorClear( bi->dir );
	bi->speed = 0;
	jumped = bi->actionflags & ACTION_JUMP;
	bi->actionflags = 0;
	if ( jumped ) bi->actionflags |= ACTION_JUMPEDLASTFRAME;
}

int EA_Setup( void ) {
	botinputs = (bot_input_t *) GetClearedHunkMemory(
									botlibglobals.maxclients * sizeof(bot_input_t) );
	return BLERR_NOERROR;
}

void EA_Shutdown( void ) {
	FreeMemory( botinputs );
	botinputs = NULL;
}
