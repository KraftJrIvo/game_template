#pragma once

#include "raylib.h"
#include "raymath.h"

#include "game_cfg.h"
#include <cstddef>
#include <string>

#ifdef USE_STEAMWORKS
#include "steam/steam_api.h"
#include "steam/steamnetworkingtypes.h"
#endif

#ifdef GAME_BASE_SHARED
#include "../../src/util/zpp_bits.h"
#endif

#include "util/arena.h"

#ifdef GAME_BASE_SHARED    
#define DO_NOT_SERIALIZE friend zpp::bits::access; using serialize = zpp::bits::members<0>;        
#else
#define DO_NOT_SERIALIZE ;
#endif

struct GameAssets {
    Texture2D sprite;
    Font font;
    Vector2 spriteSz;
};

struct Player {
    Vector2 pos, vel; 
    Color color;     
};

struct GameState {
    unsigned int seed;
    double time;
    double gameStartTime;
    
    Arena<MAX_PLAYERS, Player> players;
    size_t playerIdx;

    struct UserData {
        DO_NOT_SERIALIZE
        int spriteColor = 0;
        bool operator==(const UserData&) const = default;
    } usr;
    struct Temp {
        DO_NOT_SERIALIZE
        RenderTexture2D renderTexNative;
        RenderTexture2D renderTexFinal;
        bool timeOffsetSet = false;
        double timeOffset;
        float shTime;
    } tmp;
    struct AssetsPtr {
        DO_NOT_SERIALIZE
        const GameAssets* p;
    } ga;

#ifdef USE_STEAMWORKS
    struct NetCallbacks {
        DO_NOT_SERIALIZE
        GameState* gs = nullptr;
        void OnLobbyCreated(LobbyCreated_t* res, bool ioFailure);
        void OnLobbyEnter(LobbyEnter_t* res, bool ioFailure);
    };

    struct NetState {
        DO_NOT_SERIALIZE
        bool steamOk = false;
        bool inLobby = false;
        bool lobbyPending = false;
        CSteamID lobbyId;
        std::array<CSteamID, MAX_PLAYERS> peers;
        std::array<size_t, MAX_PLAYERS> peerPlayerIdx;
        size_t peerCount = 0;
        double lastSendTime = 0.0;
        NetCallbacks netCallbacks;
        CCallResult<NetCallbacks, LobbyCreated_t>* lobbyCreated;
        CCallResult<NetCallbacks, LobbyEnter_t>* lobbyEnter;
    } net;
#endif
};


#ifndef GAME_BASE_SHARED
extern "C" {
    void reset(GameState& gs);
    void init(GameAssets& ga, GameState& gs);
    void setState(GameState& gs, const GameState& ngs);
    void updateAndDraw(GameState& gs);
}
#endif
