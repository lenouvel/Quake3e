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
 * name:		be_ai_goal.c
 *
 * desc:		goal AI (stubbed - AI logic removed for Python bridge)
 *
 *****************************************************************************/

#include "../qcommon/q_shared.h"
#include "botlib.h"
#include "be_ai_goal.h"

static int next_handle = 0;

int BotSetupGoalAI(void)
{
	return BLERR_NOERROR;
}

void BotShutdownGoalAI(void)
{
	next_handle = 0;
}

int BotAllocGoalState(int client)
{
	return ++next_handle;
}

void BotFreeGoalState(int handle)
{
}

void BotResetGoalState(int goalstate)
{
}

void BotResetAvoidGoals(int goalstate)
{
}

void BotRemoveFromAvoidGoals(int goalstate, int number)
{
}

void BotPushGoal(int goalstate, bot_goal_t *goal)
{
}

void BotPopGoal(int goalstate)
{
}

void BotEmptyGoalStack(int goalstate)
{
}

void BotDumpAvoidGoals(int goalstate)
{
}

void BotDumpGoalStack(int goalstate)
{
}

void BotGoalName(int number, char *name, int size)
{
	if (size > 0) name[0] = '\0';
}

int BotGetTopGoal(int goalstate, bot_goal_t *goal)
{
	return 0;
}

int BotGetSecondGoal(int goalstate, bot_goal_t *goal)
{
	return 0;
}

int BotChooseLTGItem(int goalstate, vec3_t origin, int *inventory, int travelflags)
{
	return 0;
}

int BotChooseNBGItem(int goalstate, vec3_t origin, int *inventory, int travelflags, bot_goal_t *ltg, float maxtime)
{
	return 0;
}

int BotTouchingGoal(vec3_t origin, bot_goal_t *goal)
{
	return 0;
}

int BotItemGoalInVisButNotVisible(int viewer, vec3_t eye, vec3_t viewangles, bot_goal_t *goal)
{
	return 0;
}

int BotGetLevelItemGoal(int index, char *classname, bot_goal_t *goal)
{
	return -1;
}

int BotGetNextCampSpotGoal(int num, bot_goal_t *goal)
{
	return 0;
}

int BotGetMapLocationGoal(char *name, bot_goal_t *goal)
{
	return 0;
}

float BotAvoidGoalTime(int goalstate, int number)
{
	return 0.0f;
}

void BotSetAvoidGoalTime(int goalstate, int number, float avoidtime)
{
}

void BotInitLevelItems(void)
{
}

void BotUpdateEntityItems(void)
{
}

int BotLoadItemWeights(int goalstate, char *filename)
{
	return BLERR_NOERROR;
}

void BotFreeItemWeights(int goalstate)
{
}

void BotInterbreedGoalFuzzyLogic(int parent1, int parent2, int child)
{
}

void BotSaveGoalFuzzyLogic(int goalstate, char *filename)
{
}

void BotMutateGoalFuzzyLogic(int goalstate, float range)
{
}
