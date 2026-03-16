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
 * name:		be_ai_chat.c
 *
 * desc:		chat AI (stubbed - AI logic removed for Python bridge)
 *
 *****************************************************************************/

#include "../qcommon/q_shared.h"
#include "botlib.h"
#include "be_ai_chat.h"

static int next_handle = 0;

int BotSetupChatAI(void)
{
	return BLERR_NOERROR;
}

void BotShutdownChatAI(void)
{
	next_handle = 0;
}

int BotAllocChatState(void)
{
	return ++next_handle;
}

void BotFreeChatState(int handle)
{
}

void BotQueueConsoleMessage(int chatstate, int type, char *message)
{
}

void BotRemoveConsoleMessage(int chatstate, int handle)
{
}

int BotNextConsoleMessage(int chatstate, bot_consolemessage_t *cm)
{
	return 0;
}

int BotNumConsoleMessages(int chatstate)
{
	return 0;
}

void BotInitialChat(int chatstate, char *type, int mcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7)
{
}

int BotNumInitialChats(int chatstate, char *type)
{
	return 0;
}

int BotReplyChat(int chatstate, char *message, int mcontext, int vcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7)
{
	return 0;
}

int BotChatLength(int chatstate)
{
	return 0;
}

void BotEnterChat(int chatstate, int clientto, int sendto)
{
}

void BotGetChatMessage(int chatstate, char *buf, int size)
{
	if (size > 0) buf[0] = '\0';
}

int StringContains(char *str1, char *str2, int casesensitive)
{
	return -1;
}

int BotFindMatch(char *str, bot_match_t *match, unsigned long int context)
{
	return qfalse;
}

void BotMatchVariable(bot_match_t *match, int variable, char *buf, int size)
{
	if (size > 0) buf[0] = '\0';
}

void UnifyWhiteSpaces(char *string)
{
}

void BotReplaceSynonyms(char *string, unsigned long int context)
{
}

int BotLoadChatFile(int chatstate, char *chatfile, char *chatname)
{
	return BLERR_NOERROR;
}

void BotSetChatGender(int chatstate, int gender)
{
}

void BotSetChatName(int chatstate, char *name, int client)
{
}
