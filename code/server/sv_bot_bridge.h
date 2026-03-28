/*
===========================================================================
sv_bot_bridge.h - Python AI Bridge for Quake3e

TCP socket bridge between the Quake3e server and external Python AI/monitoring tools.

Protocol (line-based JSON over TCP):
  Engine -> Python: Game state each frame (positions, velocity, health, events...)
  Python -> Engine: Bot commands (move, attack, view angles...)
===========================================================================
*/
#ifndef SV_BOT_BRIDGE_H
#define SV_BOT_BRIDGE_H

// This header expects server.h and botlib.h to be included BEFORE it.
// bot_entitystate_t, vec3_t, qboolean must already be defined.

// Bridge configuration
#define BRIDGE_DEFAULT_PORT     27961
#define BRIDGE_MAX_CLIENTS      72
#define BRIDGE_BUFFER_SIZE      65536
#define BRIDGE_MAX_ENTITIES     1024

// Game event types sent to Python
typedef enum {
    BRIDGE_EVT_NONE = 0,
    BRIDGE_EVT_PLAYER_DEATH,
    BRIDGE_EVT_ITEM_PICKUP,
    BRIDGE_EVT_FLAG_PICKUP,
    BRIDGE_EVT_FLAG_CAPTURE,
    BRIDGE_EVT_FLAG_RETURN,
    BRIDGE_EVT_WEAPON_FIRE,
    BRIDGE_EVT_DAMAGE,
    BRIDGE_EVT_ROUND_START,
    BRIDGE_EVT_ROUND_END,
} bridge_event_type_t;

// A game event to be sent to Python
typedef struct {
    bridge_event_type_t type;
    int                 serverTime;
    int                 entityNum;
    int                 otherEntityNum;
    int                 value;          // damage amount, weapon id, etc.
    vec3_t              origin;
} bridge_event_t;

#define BRIDGE_MAX_EVENTS_PER_FRAME 64

// Enriched entity state for the bridge (extends bot_entitystate_t with playerState data)
// This is BRIDGE-ONLY, never passed to/from the QVM
typedef struct {
    bot_entitystate_t   base;       // original entity state from QVM
    // Enriched data read directly from playerState_t by the bridge
    vec3_t              velocity;
    int                 health;
    int                 armor;
    int                 clientNum;
    int                 ammo;
    vec3_t              viewangles;
} bridge_entity_t;

// Bot command received from Python
typedef struct {
    int     clientNum;
    vec3_t  moveDir;        // normalized movement direction
    float   moveSpeed;      // 0-400
    vec3_t  viewAngles;     // desired view angles
    int     attack;         // 1 = fire
    int     jump;           // 1 = jump
    int     crouch;         // 1 = crouch
    int     use;            // 1 = use
    int     weapon;         // weapon to select (0 = no change)
    int     respawn;        // 1 = respawn
} bridge_bot_cmd_t;

// Bridge lifecycle
void    SV_BridgeInit( void );
void    SV_BridgeShutdown( void );       // soft — keeps listen socket alive across map_restart
void    SV_BridgeForceShutdown( void );  // hard — full teardown on server quit

// Called each server frame from SV_BotFrame
void    SV_BridgeFrame( int serverTime );

// Push entity state update to the bridge (called from BotLibUpdateEntity)
void    SV_BridgeUpdateEntity( int entNum, bot_entitystate_t *state );

// Push a game event to the bridge
void    SV_BridgePushEvent( bridge_event_t *event );

// Apply bot commands received from Python — directly injects usercmds
void    SV_BridgeApplyBotCommands( void );

// Check if the bridge has a connected Python AI controlling this bot
qboolean SV_BridgeControlsBot( int clientNum );

// Flag: bridge already called SV_ClientThink for this bot this frame
// Used to prevent the QVM's BOTLIB_USER_COMMAND from double-processing
extern qboolean bridgeBotProcessed[MAX_CLIENTS];

#endif // SV_BOT_BRIDGE_H
