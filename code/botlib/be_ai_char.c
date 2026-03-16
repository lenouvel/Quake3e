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
 * name:		be_ai_char.c
 *
 * desc:		bot characters (stubbed - AI logic removed for Python bridge)
 *
 *****************************************************************************/

#include "../qcommon/q_shared.h"
#include "botlib.h"
#include "be_ai_char.h"

static int next_handle = 0;

int BotLoadCharacter(char *charfile, float skill)
{
	return ++next_handle;
}

void BotFreeCharacter(int character)
{
}

float Characteristic_Float(int character, int index)
{
	return 0.0f;
}

float Characteristic_BFloat(int character, int index, float min, float max)
{
	return min;
}

int Characteristic_Integer(int character, int index)
{
	return 0;
}

int Characteristic_BInteger(int character, int index, int min, int max)
{
	return min;
}

void Characteristic_String(int character, int index, char *buf, int size)
{
	if (size > 0) buf[0] = '\0';
}

void BotShutdownCharacters(void)
{
	next_handle = 0;
}
