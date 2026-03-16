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
 * name:		be_ai_weap.c
 *
 * desc:		weapon AI (stubbed - AI logic removed for Python bridge)
 *
 *****************************************************************************/

#include "../qcommon/q_shared.h"
#include "be_aas.h"
#include "botlib.h"
#include "be_ai_weap.h"

static int next_handle = 0;

int BotSetupWeaponAI(void)
{
	return BLERR_NOERROR;
}

void BotShutdownWeaponAI(void)
{
	next_handle = 0;
}

int BotChooseBestFightWeapon(int weaponstate, int *inventory)
{
	return 0;
}

void BotGetWeaponInfo(int weaponstate, int weapon, weaponinfo_t *weaponinfo)
{
	Com_Memset(weaponinfo, 0, sizeof(*weaponinfo));
}

int BotLoadWeaponWeights(int weaponstate, char *filename)
{
	return BLERR_NOERROR;
}

int BotAllocWeaponState(void)
{
	return ++next_handle;
}

void BotFreeWeaponState(int weaponstate)
{
}

void BotResetWeaponState(int weaponstate)
{
}
