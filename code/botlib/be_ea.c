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
 * desc:		elementary actions
 *
 * $Archive: /MissionPack/code/botlib/be_ea.c $
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

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
// ============================================================================
// STUBBED Elementary Actions - All actions are no-ops.
// The bot is completely inert until the Python AI bridge takes over.
// The QVM still calls these functions, but they do nothing.
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
void EA_SelectWeapon( int client, int weapon ) {}
void EA_Attack( int client ) {}
void EA_Talk( int client ) {}
void EA_Use( int client ) {}
void EA_Respawn( int client ) {}
void EA_Jump( int client ) {}
void EA_DelayedJump( int client ) {}
void EA_Crouch( int client ) {}
void EA_Walk( int client ) {}
void EA_Action( int client, int action ) {}
void EA_MoveUp( int client ) {}
void EA_MoveDown( int client ) {}
void EA_MoveLeft( int client ) {}
void EA_MoveRight( int client ) {}
void EA_MoveForward( int client ) {}
void EA_MoveBack( int client ) {}
void EA_Move( int client, vec3_t dir, float speed ) {}
void EA_View( int client, vec3_t viewangles ) {}
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_EndRegular(int client, float thinktime)
{
} //end of the function EA_EndRegular
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_GetInput(int client, float thinktime, bot_input_t *input)
{
	bot_input_t *bi;

	bi = &botinputs[client];
	bi->thinktime = thinktime;
	Com_Memcpy(input, bi, sizeof(bot_input_t));
} //end of the function EA_GetInput
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_ResetInput(int client)
{
	bot_input_t *bi;
	int jumped;

	bi = &botinputs[client];

	bi->thinktime = 0;
	VectorClear(bi->dir);
	bi->speed = 0;
	jumped = bi->actionflags & ACTION_JUMP;
	bi->actionflags = 0;
	if (jumped) bi->actionflags |= ACTION_JUMPEDLASTFRAME;
} //end of the function EA_ResetInput
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int EA_Setup(void)
{
	//initialize the bot inputs
	botinputs = (bot_input_t *) GetClearedHunkMemory(
									botlibglobals.maxclients * sizeof(bot_input_t));
	return BLERR_NOERROR;
} //end of the function EA_Setup
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void EA_Shutdown(void)
{
	FreeMemory(botinputs);
	botinputs = NULL;
} //end of the function EA_Shutdown
