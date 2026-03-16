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
 * name:		be_ai_move.c
 *
 * desc:		movement AI (stubbed - AI logic removed for Python bridge)
 *
 *****************************************************************************/

#include "../qcommon/q_shared.h"
#include "botlib.h"
#include "be_aas.h"
#include "be_aas_funcs.h"
#include "be_ai_goal.h"
#include "be_ai_move.h"

static int next_handle = 0;

// Internal functions used by be_interface.c (BotExportTest) under #ifdef DEBUG
int BotFuzzyPointReachabilityArea(vec3_t origin)
{
	return AAS_PointAreaNum(origin);
}

int BotGetReachabilityToGoal(vec3_t origin, int areanum,
								  int lastgoalareanum, int lastareanum,
								  int *avoidreach, float *avoidreachtimes, int *avoidreachtries,
								  bot_goal_t *goal, int travelflags,
								  struct bot_avoidspot_s *avoidspots, int numavoidspots, int *flags)
{
	return 0;
}

float BotGapDistance(vec3_t origin, vec3_t hordir, int entnum)
{
	return 0.0f;
}

// Exported functions

int BotSetupMoveAI(void)
{
	return BLERR_NOERROR;
}

void BotShutdownMoveAI(void)
{
	next_handle = 0;
}

int BotAllocMoveState(void)
{
	return ++next_handle;
}

void BotFreeMoveState(int handle)
{
}

void BotResetMoveState(int movestate)
{
}

void BotMoveToGoal(bot_moveresult_t *result, int movestate, bot_goal_t *goal, int travelflags)
{
	Com_Memset(result, 0, sizeof(*result));
}

int BotMoveInDirection(int movestate, vec3_t dir, float speed, int type)
{
	return 0;
}

void BotResetAvoidReach(int movestate)
{
}

void BotResetLastAvoidReach(int movestate)
{
}

int BotReachabilityArea(vec3_t origin, int client)
{
	return 0;
}

int BotMovementViewTarget(int movestate, bot_goal_t *goal, int travelflags, float lookahead, vec3_t target)
{
	return 0;
}

int BotPredictVisiblePosition(vec3_t origin, int areanum, bot_goal_t *goal, int travelflags, vec3_t target)
{
	return 0;
}

void BotInitMoveState(int handle, bot_initmove_t *initmove)
{
}

void BotAddAvoidSpot(int movestate, vec3_t origin, float radius, int type)
{
}

void BotSetBrushModelTypes(void)
{
}
