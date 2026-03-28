/*
===========================================================================
sv_bot_bridge.c - Python AI Bridge for Quake3e

Non-blocking TCP socket server that streams game state to Python clients
and receives bot commands back.

Protocol: line-delimited JSON (\n terminated)
  -> To Python:   {"type":"state", "time":12345, "entities":[...], "events":[...]}
  -> From Python:  {"type":"cmd", "client":0, "move":[1,0,0], "speed":400, ...}
===========================================================================
*/

#include "server.h"
#include "../botlib/botlib.h"
#include "sv_bot_bridge.h"

#include <math.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET socket_t;
#define INVALID_SOCK INVALID_SOCKET
#define SOCK_ERROR   SOCKET_ERROR
#define BRIDGE_CLOSE closesocket
#define BRIDGE_IOCTL ioctlsocket
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
typedef int socket_t;
#define INVALID_SOCK (-1)
#define SOCK_ERROR   (-1)
#define BRIDGE_CLOSE close
#endif

// ============================================================================
// Bridge state
// ============================================================================

typedef struct {
    socket_t    sock;
    qboolean    active;
    qboolean    isAI;           // true = AI controller, false = monitor only
    int         controlledBot;  // clientNum this AI controls (-1 = monitor)
    char        *recvBuf;       // dynamically allocated [BRIDGE_BUFFER_SIZE]
    int         recvLen;
} bridge_client_t;

static struct {
    qboolean        initialized;
    socket_t        listenSock;
    bridge_client_t clients[BRIDGE_MAX_CLIENTS];
    int             numClients;

    // Entity state snapshot for current frame (enriched with playerState data)
    bridge_entity_t     *entities;          // dynamically allocated [BRIDGE_MAX_ENTITIES]
    qboolean            *entityUpdated;     // dynamically allocated [BRIDGE_MAX_ENTITIES]
    int                 numEntities;

    // Events queued this frame
    bridge_event_t  events[BRIDGE_MAX_EVENTS_PER_FRAME];
    int             numEvents;

    // Bot commands received from Python
    bridge_bot_cmd_t    botCmds[MAX_CLIENTS];
    qboolean            botCmdValid[MAX_CLIENTS];

    cvar_t  *port;
    cvar_t  *enabled;
} bridge;

// ============================================================================
// Socket helpers
// ============================================================================

static void Bridge_SetNonBlocking( socket_t sock ) {
#ifdef _WIN32
    u_long mode = 1;
    BRIDGE_IOCTL( sock, FIONBIO, &mode );
#else
    int flags = fcntl( sock, F_GETFL, 0 );
    fcntl( sock, F_SETFL, flags | O_NONBLOCK );
#endif
}

static void Bridge_SetNoDelay( socket_t sock ) {
    int flag = 1;
    setsockopt( sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&flag, sizeof(flag) );
}

// ============================================================================
// JSON helpers (minimal, no external dependency)
// ============================================================================

// Truncating sprintf that returns chars written
static int Com_sprintf_truncated( char *buf, int maxlen, const char *fmt, ... ) {
    va_list ap;
    int len;
    va_start( ap, fmt );
    len = Q_vsnprintf( buf, maxlen, fmt, ap );
    va_end( ap );
    if ( len >= maxlen ) len = maxlen - 1;
    if ( len < 0 ) len = 0;
    return len;
}

static int Bridge_WriteVec3( char *buf, int maxlen, const char *key, vec3_t v ) {
    return Com_sprintf_truncated( buf, maxlen, "\"%s\":[%.2f,%.2f,%.2f]", key, v[0], v[1], v[2] );
}

static int Bridge_WriteEntityJson( char *buf, int maxlen, int entNum, bridge_entity_t *ent ) {
    int len = 0;
    int n;
    bot_entitystate_t *b = &ent->base;

    n = Com_sprintf_truncated( buf + len, maxlen - len,
        "{\"id\":%d,\"type\":%d,\"flags\":%d,",
        entNum, b->type, b->flags );
    len += n;

    n = Bridge_WriteVec3( buf + len, maxlen - len, "origin", b->origin );
    len += n;

    n = Com_sprintf_truncated( buf + len, maxlen - len, "," );
    len += n;

    n = Bridge_WriteVec3( buf + len, maxlen - len, "velocity", ent->velocity );
    len += n;

    n = Com_sprintf_truncated( buf + len, maxlen - len, "," );
    len += n;

    n = Bridge_WriteVec3( buf + len, maxlen - len, "angles", b->angles );
    len += n;

    n = Com_sprintf_truncated( buf + len, maxlen - len, "," );
    len += n;

    n = Bridge_WriteVec3( buf + len, maxlen - len, "viewangles", ent->viewangles );
    len += n;

    n = Com_sprintf_truncated( buf + len, maxlen - len,
        ",\"health\":%d,\"armor\":%d,\"weapon\":%d,\"ammo\":%d,"
        "\"clientNum\":%d,\"groundent\":%d,\"event\":%d,\"eventParm\":%d,"
        "\"powerups\":%d,\"modelindex\":%d,\"modelindex2\":%d,"
        "\"legsAnim\":%d,\"torsoAnim\":%d}",
        ent->health, ent->armor, b->weapon, ent->ammo,
        ent->clientNum, b->groundent, b->event, b->eventParm,
        b->powerups, b->modelindex, b->modelindex2,
        b->legsAnim, b->torsoAnim );
    len += n;

    return len;
}

static int Bridge_WriteEventJson( char *buf, int maxlen, bridge_event_t *evt ) {
    int len = 0;
    int n;

    n = Com_sprintf_truncated( buf + len, maxlen - len,
        "{\"type\":%d,\"time\":%d,\"ent\":%d,\"other\":%d,\"value\":%d,",
        evt->type, evt->serverTime, evt->entityNum, evt->otherEntityNum, evt->value );
    len += n;

    n = Bridge_WriteVec3( buf + len, maxlen - len, "origin", evt->origin );
    len += n;

    n = Com_sprintf_truncated( buf + len, maxlen - len, "}" );
    len += n;

    return len;
}

// ============================================================================
// JSON parsing (minimal for bot commands)
// ============================================================================

static float Bridge_ParseFloat( const char *json, const char *key, float defaultVal ) {
    char search[64];
    const char *p;

    Com_sprintf( search, sizeof(search), "\"%s\":", key );
    p = strstr( json, search );
    if ( !p ) return defaultVal;
    p += strlen( search );
    while ( *p == ' ' ) p++;
    return (float)atof( p );
}

static int Bridge_ParseInt( const char *json, const char *key, int defaultVal ) {
    char search[64];
    const char *p;

    Com_sprintf( search, sizeof(search), "\"%s\":", key );
    p = strstr( json, search );
    if ( !p ) return defaultVal;
    p += strlen( search );
    while ( *p == ' ' ) p++;
    return atoi( p );
}

static void Bridge_ParseVec3( const char *json, const char *key, vec3_t out ) {
    char search[64];
    const char *p;

    // Try compact format "key":[  then spaced format "key": [
    Com_sprintf( search, sizeof(search), "\"%s\":[", key );
    p = strstr( json, search );
    if ( !p ) {
        Com_sprintf( search, sizeof(search), "\"%s\": [", key );
        p = strstr( json, search );
    }
    if ( !p ) {
        VectorClear( out );
        return;
    }
    p += strlen( search );
    out[0] = (float)atof( p );
    p = strchr( p, ',' );
    if ( !p ) return;
    out[1] = (float)atof( p + 1 );
    p = strchr( p + 1, ',' );
    if ( !p ) return;
    out[2] = (float)atof( p + 1 );
}

static void Bridge_ProcessCommand( const char *json ) {
    int clientNum;
    bridge_bot_cmd_t *cmd;

    clientNum = Bridge_ParseInt( json, "client", -1 );
    if ( clientNum < 0 || clientNum >= sv_maxclients->integer ) return;

    cmd = &bridge.botCmds[clientNum];

    Bridge_ParseVec3( json, "move", cmd->moveDir );
    cmd->moveSpeed  = Bridge_ParseFloat( json, "speed", 0.0f );
    Bridge_ParseVec3( json, "view", cmd->viewAngles );
    cmd->attack     = Bridge_ParseInt( json, "attack", 0 );
    cmd->jump       = Bridge_ParseInt( json, "jump", 0 );
    cmd->crouch     = Bridge_ParseInt( json, "crouch", 0 );
    cmd->use        = Bridge_ParseInt( json, "use", 0 );
    cmd->weapon     = Bridge_ParseInt( json, "weapon", 0 );
    cmd->respawn    = Bridge_ParseInt( json, "respawn", 0 );
    cmd->clientNum  = clientNum;

    bridge.botCmdValid[clientNum] = qtrue;

    // Debug: log first received command per bot
    {
        static int cmdRecvCount[MAX_CLIENTS] = {0};
        cmdRecvCount[clientNum]++;
        if ( cmdRecvCount[clientNum] <= 3 || cmdRecvCount[clientNum] % 500 == 0 ) {
            Com_Printf( "Bridge DEBUG: received cmd #%d for bot %d: "
                "move=(%.2f,%.2f,%.2f) speed=%.0f view=(%.1f,%.1f,%.1f)\n",
                cmdRecvCount[clientNum], clientNum,
                cmd->moveDir[0], cmd->moveDir[1], cmd->moveDir[2],
                cmd->moveSpeed,
                cmd->viewAngles[0], cmd->viewAngles[1], cmd->viewAngles[2] );
        }
    }
}

// ============================================================================
// Network I/O
// ============================================================================

static void Bridge_AcceptClients( void ) {
    struct sockaddr_in addr;
    socklen_t addrLen = sizeof(addr);
    socket_t newSock;
    int i;

    newSock = accept( bridge.listenSock, (struct sockaddr*)&addr, &addrLen );
    if ( newSock == INVALID_SOCK ) return;

    // Find free slot
    for ( i = 0; i < BRIDGE_MAX_CLIENTS; i++ ) {
        if ( !bridge.clients[i].active ) break;
    }

    if ( i == BRIDGE_MAX_CLIENTS ) {
        Com_Printf( S_COLOR_YELLOW "Bridge: max clients reached, rejecting connection\n" );
        BRIDGE_CLOSE( newSock );
        return;
    }

    Bridge_SetNonBlocking( newSock );
    Bridge_SetNoDelay( newSock );

    bridge.clients[i].sock = newSock;
    bridge.clients[i].active = qtrue;
    bridge.clients[i].isAI = qfalse;
    bridge.clients[i].controlledBot = -1;
    bridge.clients[i].recvLen = 0;
    bridge.numClients++;

    Com_Printf( "Bridge: client %d connected from %s\n", i, inet_ntoa(addr.sin_addr) );
}

static void Bridge_DisconnectClient( int idx ) {
    if ( !bridge.clients[idx].active ) return;

    BRIDGE_CLOSE( bridge.clients[idx].sock );
    bridge.clients[idx].active = qfalse;
    bridge.clients[idx].sock = INVALID_SOCK;

    // Clear bot command if this AI was controlling a bot
    if ( bridge.clients[idx].controlledBot >= 0 ) {
        bridge.botCmdValid[ bridge.clients[idx].controlledBot ] = qfalse;
    }

    bridge.clients[idx].controlledBot = -1;
    bridge.numClients--;

    Com_Printf( "Bridge: client %d disconnected\n", idx );
}

static void Bridge_ReceiveData( void ) {
    int i;

    for ( i = 0; i < BRIDGE_MAX_CLIENTS; i++ ) {
        bridge_client_t *cl = &bridge.clients[i];
        int bytesRead;
        char *lineEnd;

        if ( !cl->active ) continue;

        // Read available data
        bytesRead = recv( cl->sock, cl->recvBuf + cl->recvLen,
                          BRIDGE_BUFFER_SIZE - cl->recvLen - 1, 0 );

        if ( bytesRead == 0 ) {
            Bridge_DisconnectClient( i );
            continue;
        }

        if ( bytesRead > 0 ) {
            cl->recvLen += bytesRead;
            cl->recvBuf[cl->recvLen] = '\0';

            // Process complete lines
            while ( (lineEnd = strchr( cl->recvBuf, '\n' )) != NULL ) {
                *lineEnd = '\0';

                // Check for registration message
                // Support both "type":"register" and "type": "register" (with optional space)
                if ( strstr( cl->recvBuf, "\"type\":\"register\"" ) ||
                     strstr( cl->recvBuf, "\"type\": \"register\"" ) ) {
                    int botNum = Bridge_ParseInt( cl->recvBuf, "bot", -1 );
                    if ( botNum >= 0 && botNum < sv_maxclients->integer ) {
                        cl->isAI = qtrue;
                        cl->controlledBot = botNum;
                        Com_Printf( "Bridge: client %d registered as AI for bot %d\n", i, botNum );
                    } else {
                        cl->isAI = qfalse;
                        cl->controlledBot = -1;
                        Com_Printf( "Bridge: client %d registered as monitor\n", i );
                    }
                }
                // Check for bot command
                else if ( strstr( cl->recvBuf, "\"type\":\"cmd\"" ) ||
                          strstr( cl->recvBuf, "\"type\": \"cmd\"" ) ) {
                    if ( cl->isAI ) {
                        Bridge_ProcessCommand( cl->recvBuf );
                    }
                }

                // Shift buffer
                int consumed = (int)(lineEnd - cl->recvBuf) + 1;
                cl->recvLen -= consumed;
                if ( cl->recvLen > 0 ) {
                    memmove( cl->recvBuf, lineEnd + 1, cl->recvLen );
                }
                cl->recvBuf[cl->recvLen] = '\0';
            }

            // Prevent buffer overflow
            if ( cl->recvLen >= BRIDGE_BUFFER_SIZE - 1 ) {
                cl->recvLen = 0;
            }
        }
    }
}

static void Bridge_SendToAll( const char *data, int len ) {
    int i;

    for ( i = 0; i < BRIDGE_MAX_CLIENTS; i++ ) {
        if ( !bridge.clients[i].active ) continue;

        {
            int flags = 0;
#ifndef _WIN32
            flags = MSG_NOSIGNAL;  // prevent SIGPIPE on broken connection
#endif
            if ( send( bridge.clients[i].sock, data, len, flags ) == SOCK_ERROR ) {
#ifdef _WIN32
                if ( WSAGetLastError() != WSAEWOULDBLOCK ) {
#else
                if ( errno != EAGAIN && errno != EWOULDBLOCK ) {
#endif
                    Bridge_DisconnectClient( i );
                }
            }
        }
    }
}

// ============================================================================
// Public API
// ============================================================================

void SV_BridgeInit( void ) {
    struct sockaddr_in addr;
    int opt = 1;
    int i;

    // If the bridge is already running (map_restart), keep the listen socket
    // and all client connections alive — only reset per-frame entity state.
    if ( bridge.initialized && bridge.listenSock != INVALID_SOCK ) {
        Com_Printf( "Bridge: map restart — keeping listen socket and %d client(s)\n",
                     bridge.numClients );

        // Reset entity state for the new map
        if ( bridge.entities ) {
            Com_Memset( bridge.entities, 0, BRIDGE_MAX_ENTITIES * sizeof(bridge_entity_t) );
        }
        if ( bridge.entityUpdated ) {
            Com_Memset( bridge.entityUpdated, 0, BRIDGE_MAX_ENTITIES * sizeof(qboolean) );
        }
        bridge.numEntities = 0;
        bridge.numEvents = 0;

        // Clear pending bot commands (clients will re-send)
        Com_Memset( bridge.botCmds, 0, sizeof(bridge.botCmds) );
        Com_Memset( bridge.botCmdValid, 0, sizeof(bridge.botCmdValid) );

        return;
    }

    Com_Memset( &bridge, 0, sizeof(bridge) );
    bridge.listenSock = INVALID_SOCK;

    bridge.enabled = Cvar_Get( "bridge_enabled", "0", 0 );
    bridge.port = Cvar_Get( "bridge_port", va("%d", BRIDGE_DEFAULT_PORT), CVAR_ARCHIVE );

    if ( !bridge.enabled->integer ) {
        Com_Printf( "Bridge: disabled (set bridge_enabled 1 to enable)\n" );
        return;
    }

    // Allocate dynamic arrays
    bridge.entities = (bridge_entity_t *) Z_Malloc( BRIDGE_MAX_ENTITIES * sizeof(bridge_entity_t) );
    bridge.entityUpdated = (qboolean *) Z_Malloc( BRIDGE_MAX_ENTITIES * sizeof(qboolean) );
    Com_Memset( bridge.entities, 0, BRIDGE_MAX_ENTITIES * sizeof(bridge_entity_t) );
    Com_Memset( bridge.entityUpdated, 0, BRIDGE_MAX_ENTITIES * sizeof(qboolean) );

    // Allocate client receive buffers
    for ( i = 0; i < BRIDGE_MAX_CLIENTS; i++ ) {
        bridge.clients[i].recvBuf = (char *) Z_Malloc( BRIDGE_BUFFER_SIZE );
        bridge.clients[i].recvBuf[0] = '\0';
    }

#ifdef _WIN32
    {
        WSADATA wsaData;
        if ( WSAStartup( MAKEWORD(2,2), &wsaData ) != 0 ) {
            Com_Printf( S_COLOR_RED "Bridge: WSAStartup failed\n" );
            return;
        }
    }
#endif

#ifndef _WIN32
    // Ignore SIGPIPE so writing to a disconnected client doesn't kill the server
    signal( SIGPIPE, SIG_IGN );
#endif

    bridge.listenSock = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    if ( bridge.listenSock == INVALID_SOCK ) {
        Com_Printf( S_COLOR_RED "Bridge: failed to create socket\n" );
        return;
    }

    setsockopt( bridge.listenSock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt) );
    Bridge_SetNonBlocking( bridge.listenSock );

    Com_Memset( &addr, 0, sizeof(addr) );
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl( INADDR_ANY );
    addr.sin_port = htons( (unsigned short)bridge.port->integer );

    if ( bind( bridge.listenSock, (struct sockaddr*)&addr, sizeof(addr) ) == SOCK_ERROR ) {
        Com_Printf( S_COLOR_RED "Bridge: bind failed on port %d\n", bridge.port->integer );
        BRIDGE_CLOSE( bridge.listenSock );
        bridge.listenSock = INVALID_SOCK;
        return;
    }

    if ( listen( bridge.listenSock, BRIDGE_MAX_CLIENTS ) == SOCK_ERROR ) {
        Com_Printf( S_COLOR_RED "Bridge: listen failed\n" );
        BRIDGE_CLOSE( bridge.listenSock );
        bridge.listenSock = INVALID_SOCK;
        return;
    }

    bridge.initialized = qtrue;
    Com_Printf( "Bridge: listening on port %d (Python AI bot & monitoring)\n", bridge.port->integer );
    Com_Printf( "Bridge: waiting for connections...\n" );
    Com_Printf( "Bridge:   - Bot AI:     connect and send {\"type\":\"register\",\"bot\":N}\n" );
    Com_Printf( "Bridge:   - Monitor:    connect and send {\"type\":\"register\"}\n" );
}

void SV_BridgeShutdown( void ) {
    if ( !bridge.initialized ) return;

    // On map_restart, SV_BotLibShutdown is called followed by SV_BotInitBotLib.
    // We keep the bridge alive so clients don't lose their connections.
    // SV_BridgeInit will detect the live bridge and skip re-creation.
    // SV_BridgeForceShutdown is called from SV_Shutdown for a real server quit.
    Com_Printf( "Bridge: keeping alive across map restart (%d client(s) connected)\n",
                 bridge.numClients );
}

void SV_BridgeForceShutdown( void ) {
    int i;

    if ( !bridge.initialized ) return;

    Com_Printf( "Bridge: full shutdown\n" );

    for ( i = 0; i < BRIDGE_MAX_CLIENTS; i++ ) {
        if ( bridge.clients[i].active ) {
            Bridge_DisconnectClient( i );
        }
    }

    if ( bridge.listenSock != INVALID_SOCK ) {
        BRIDGE_CLOSE( bridge.listenSock );
        bridge.listenSock = INVALID_SOCK;
    }

#ifdef _WIN32
    WSACleanup();
#endif

    // Free dynamic arrays
    if ( bridge.entities ) {
        Z_Free( bridge.entities );
        bridge.entities = NULL;
    }
    if ( bridge.entityUpdated ) {
        Z_Free( bridge.entityUpdated );
        bridge.entityUpdated = NULL;
    }
    for ( i = 0; i < BRIDGE_MAX_CLIENTS; i++ ) {
        if ( bridge.clients[i].recvBuf ) {
            Z_Free( bridge.clients[i].recvBuf );
            bridge.clients[i].recvBuf = NULL;
        }
    }

    bridge.initialized = qfalse;
}

void SV_BridgeUpdateEntity( int entNum, bot_entitystate_t *state ) {
    bridge_entity_t *bent;

    if ( !bridge.initialized ) return;
    if ( !state ) return;
    if ( !bridge.entities || !bridge.entityUpdated ) return;
    if ( entNum < 0 || entNum >= BRIDGE_MAX_ENTITIES ) return;

    bent = &bridge.entities[entNum];

    // Copy the base entity state (QVM-safe, original struct size)
    Com_Memcpy( &bent->base, state, sizeof(bot_entitystate_t) );

    // Enrich with playerState_t data if this is a client entity
    if ( sv_maxclients && entNum >= 0 && entNum < sv_maxclients->integer && sv.gameClients ) {
        playerState_t *ps = SV_GameClientNum( entNum );
        if ( ps ) {
            VectorCopy( ps->velocity, bent->velocity );
            bent->health = ps->stats[0];    // STAT_HEALTH
            bent->armor = ps->stats[3];      // STAT_ARMOR
            bent->clientNum = ps->clientNum;
            if ( ps->weapon >= 0 && ps->weapon < MAX_WEAPONS ) {
                bent->ammo = ps->ammo[ps->weapon];
            } else {
                bent->ammo = 0;
            }
            VectorCopy( ps->viewangles, bent->viewangles );
        } else {
            VectorClear( bent->velocity );
            bent->health = 0;
            bent->armor = 0;
            bent->clientNum = entNum;
            bent->ammo = 0;
            VectorClear( bent->viewangles );
        }
    } else {
        VectorClear( bent->velocity );
        bent->health = 0;
        bent->armor = 0;
        bent->clientNum = -1;
        bent->ammo = 0;
        VectorClear( bent->viewangles );
    }

    bridge.entityUpdated[entNum] = qtrue;
    if ( entNum >= bridge.numEntities ) {
        bridge.numEntities = entNum + 1;
    }
}

void SV_BridgePushEvent( bridge_event_t *event ) {
    if ( !bridge.initialized ) return;
    if ( bridge.numEvents >= BRIDGE_MAX_EVENTS_PER_FRAME ) return;

    Com_Memcpy( &bridge.events[bridge.numEvents], event, sizeof(bridge_event_t) );
    bridge.numEvents++;
}

void SV_BridgeFrame( int serverTime ) {
    static char sendBuf[BRIDGE_BUFFER_SIZE * 4];
    int len = 0;
    int n, i;
    qboolean firstEnt;

    if ( !bridge.initialized ) return;

    // Accept new connections
    Bridge_AcceptClients();

    // Receive data from Python clients
    Bridge_ReceiveData();

    // If no clients connected, skip building the JSON
    if ( bridge.numClients == 0 ) goto cleanup;

    // Build frame JSON
    n = Com_sprintf_truncated( sendBuf + len, sizeof(sendBuf) - len,
        "{\"type\":\"state\",\"time\":%d,\"mapname\":\"%s\",\"entities\":[",
        serverTime, sv_mapname->string );
    len += n;

    // Write updated entities
    firstEnt = qtrue;
    for ( i = 0; i < bridge.numEntities; i++ ) {
        if ( !bridge.entityUpdated[i] ) continue;

        if ( !firstEnt ) {
            sendBuf[len++] = ',';
        }
        firstEnt = qfalse;

        n = Bridge_WriteEntityJson( sendBuf + len, sizeof(sendBuf) - len, i, &bridge.entities[i] );
        len += n;
    }

    n = Com_sprintf_truncated( sendBuf + len, sizeof(sendBuf) - len, "],\"events\":[" );
    len += n;

    // Write events
    for ( i = 0; i < bridge.numEvents; i++ ) {
        if ( i > 0 ) sendBuf[len++] = ',';
        n = Bridge_WriteEventJson( sendBuf + len, sizeof(sendBuf) - len, &bridge.events[i] );
        len += n;
    }

    n = Com_sprintf_truncated( sendBuf + len, sizeof(sendBuf) - len, "]}\n" );
    len += n;

    // Send to all connected clients
    Bridge_SendToAll( sendBuf, len );

cleanup:
    // Reset per-frame state
    if ( bridge.entityUpdated ) {
        Com_Memset( bridge.entityUpdated, 0, BRIDGE_MAX_ENTITIES * sizeof(qboolean) );
    }
    bridge.numEntities = 0;
    bridge.numEvents = 0;
}

// Flag per client: bridge already called SV_ClientThink this frame
qboolean bridgeBotProcessed[MAX_CLIENTS];

static int bridgeDebugCounter = 0;

void SV_BridgeApplyBotCommands( void ) {
    int i;

    if ( !bridge.initialized ) return;

    // Reset processed flags at start of frame
    Com_Memset( bridgeBotProcessed, 0, sizeof(bridgeBotProcessed) );

    bridgeDebugCounter++;

    for ( i = 0; i < sv_maxclients->integer; i++ ) {
        bridge_bot_cmd_t *cmd;
        playerState_t *ps;
        usercmd_t ucmd;
        float yaw_rad, cos_yaw, sin_yaw;
        float fwd_dot, right_dot, speed_scale;

        if ( !bridge.botCmdValid[i] ) continue;

        // Verify this client slot is active
        if ( svs.clients[i].state != CS_ACTIVE ) {
            if ( bridgeDebugCounter % 200 == 1 ) {
                Com_Printf( "Bridge DEBUG: bot %d has cmd but state=%d (not ACTIVE)\n",
                    i, svs.clients[i].state );
            }
            continue;
        }

        cmd = &bridge.botCmds[i];
        ps = SV_GameClientNum( i );
        if ( !ps ) continue;

        // --- Respawn request ---
        // Python AI signals a stuck/stray bot: execute "kill" so the QVM
        // handles suicide + respawn to a spawn point, then skip this frame.
        if ( cmd->respawn ) {
            Com_Printf( "Bridge: respawning bot %d (kill command)\n", i );
            SV_ExecuteClientCommand( &svs.clients[i], "kill" );
            bridge.botCmdValid[i] = qfalse;
            cmd->respawn = 0;
            continue;
        }

        // Build usercmd_t directly — bypasses the entire EA/QVM bot AI pipeline
        Com_Memset( &ucmd, 0, sizeof(ucmd) );
        ucmd.serverTime = sv.time;

        // --- View angles ---
        // usercmd.angles = ANGLE2SHORT(desired) - ps->delta_angles
        ucmd.angles[0] = ANGLE2SHORT( cmd->viewAngles[0] ) - ps->delta_angles[0];
        ucmd.angles[1] = ANGLE2SHORT( cmd->viewAngles[1] ) - ps->delta_angles[1];
        ucmd.angles[2] = ANGLE2SHORT( cmd->viewAngles[2] ) - ps->delta_angles[2];

        // --- Movement ---
        // Convert world-space moveDir to player-relative forward/right
        yaw_rad = (float)( cmd->viewAngles[1] * M_PI / 180.0 );
        cos_yaw = (float)cos( yaw_rad );
        sin_yaw = (float)sin( yaw_rad );

        // Player's forward vector: [cos(yaw), sin(yaw)]
        // Player's right vector:   [sin(yaw), -cos(yaw)]
        fwd_dot   = cmd->moveDir[0] * cos_yaw + cmd->moveDir[1] * sin_yaw;
        right_dot = cmd->moveDir[0] * sin_yaw - cmd->moveDir[1] * cos_yaw;

        speed_scale = cmd->moveSpeed / 127.0f;
        if ( speed_scale > 1.0f ) speed_scale = 1.0f;

        ucmd.forwardmove = (signed char)( fwd_dot * speed_scale * 127.0f );
        ucmd.rightmove   = (signed char)( right_dot * speed_scale * 127.0f );

        // --- Vertical movement ---
        if ( cmd->jump )        ucmd.upmove = 127;
        else if ( cmd->crouch ) ucmd.upmove = -127;

        // --- Buttons ---
        if ( cmd->attack )  ucmd.buttons |= BUTTON_ATTACK;

        // --- Weapon ---
        ucmd.weapon = (byte)( cmd->weapon ? cmd->weapon : ps->weapon );

        // Debug logging (every 200 frames for bot 0)
        if ( i == 0 && bridgeDebugCounter % 200 == 1 ) {
            Com_Printf( "Bridge DEBUG: bot %d cmd fwd=%d right=%d up=%d "
                "svtime=%d cmdtime=%d pos=(%.0f,%.0f,%.0f) vel=(%.0f,%.0f,%.0f)\n",
                i, ucmd.forwardmove, ucmd.rightmove, ucmd.upmove,
                sv.time, ps->commandTime,
                ps->origin[0], ps->origin[1], ps->origin[2],
                ps->velocity[0], ps->velocity[1], ps->velocity[2] );
        }

        // Inject the command directly into the engine
        SV_ClientThink( &svs.clients[i], &ucmd );
        bridgeBotProcessed[i] = qtrue;
    }
}

qboolean SV_BridgeControlsBot( int clientNum ) {
    int i;

    if ( !bridge.initialized ) return qfalse;

    for ( i = 0; i < BRIDGE_MAX_CLIENTS; i++ ) {
        if ( bridge.clients[i].active && bridge.clients[i].isAI &&
             bridge.clients[i].controlledBot == clientNum ) {
            return qtrue;
        }
    }
    return qfalse;
}
