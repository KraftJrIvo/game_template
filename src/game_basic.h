#pragma once
#include <string>
#include <iostream>

#include "game.h"
#include "raylib.h"

#if (defined(_WIN32) || defined(_WIN64)) && defined(GAME_BASE_DLL)
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif

inline double getTime(const GameState& gs) {
    return (GetTime() + gs.tmp.timeOffset);
}

inline int getRandVal(GameState& gs, int min, int max) {
    SetRandomSeed(gs.seed++);
    return GetRandomValue(min, max);
}

inline void syncTime(GameState& gs) {
    if (!gs.tmp.timeOffsetSet) {
        if (gs.time == 0) gs.time = GetTime();
        gs.tmp.timeOffset = gs.time - GetTime();
        gs.tmp.timeOffsetSet = true;
    }
}

inline void saveUserData(const GameState& gs) {
    SaveFileData("userdata", (void*)&gs.usr, sizeof(GameState::UserData));
}

inline void loadUserData(GameState& gs) {
    int sz = sizeof(GameState::UserData);
    std::vector<unsigned char> buffer(sz);
    int datasz;
    unsigned char* ptr = LoadFileData("userdata", &datasz);
    if (ptr && datasz == sz)
        gs.usr = *((GameState::UserData*)ptr);
}

inline std::string replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    str.replace(start_pos, from.length(), to);
    return str;
}

inline std::string prepShader(unsigned char* shaderBytes) {
    auto res = std::string((const char*)shaderBytes);
#ifdef PLATFORM_ANDROID    
    return replace(res, "#version 330", "#version 300 es\nprecision mediump float;");
#else
    return res;
#endif
}

inline void setStuff(const GameAssets* ga, GameState& gs, RenderTexture* rt = NULL) {
    gs.ga.p = ga;
    loadUserData(gs);
    gs.tmp.renderTexNative = LoadRenderTexture(GAME_WIDTH, GAME_HEIGHT);
    gs.tmp.renderTexFinal = (rt && IsRenderTextureValid(*rt)) ? 
        *rt : 
        LoadRenderTexture(gs.tmp.renderTexNative.texture.width, gs.tmp.renderTexNative.texture.height);
    SetTextureWrap(gs.tmp.renderTexFinal.texture, TEXTURE_WRAP_CLAMP);
}

inline void renderPreUI(GameState& gs) {
    gs.tmp.shTime = getTime(gs);
    SetShaderValue(gs.ga.p->postProcFragShader, GetShaderLocation(gs.ga.p->postProcFragShader, "time"), &gs.tmp.shTime, SHADER_UNIFORM_FLOAT);
    
    BeginTextureMode(gs.tmp.renderTexFinal);
    if (IsRenderTextureValid(gs.tmp.renderTexNative)) {
        BeginShaderMode(gs.ga.p->postProcFragShader);
        DrawTextureRec(gs.tmp.renderTexNative.texture, Rectangle{0, 0, (float)gs.tmp.renderTexNative.texture.width, (float)-gs.tmp.renderTexNative.texture.height}, Vector2Zero(), WHITE);
        EndShaderMode();
    } else {
        ClearBackground(BLACK);
    }
}

inline void renderPostUI(GameState& gs) {
    EndTextureMode();
    BeginDrawing();
    if (IsRenderTextureValid(gs.tmp.renderTexFinal)) {
        float w = GAME_SCALE * gs.tmp.renderTexFinal.texture.width;
        float h = GAME_SCALE * gs.tmp.renderTexFinal.texture.height;
        DrawTexturePro(gs.tmp.renderTexFinal.texture, 
            Rectangle{0, 0, (float)gs.tmp.renderTexFinal.texture.width, (float)-gs.tmp.renderTexFinal.texture.height}, 
            {GetScreenWidth() * 0.5f - w * 0.5f, GetScreenHeight() * 0.5f - h * 0.5f, w, h}, 
            {0,0}, 0.0f, WHITE);
    } else {
        ClearBackground(BLACK);
    }
    EndDrawing();
}

extern "C" {

    void _loadAssets(GameAssets& ga, GameState& gs); 
    void loadAssets(GameAssets& ga, GameState& gs) 
    {
        _loadAssets(ga, gs);
        gs.ga.p = &ga;
    }

    DLL_EXPORT void setState(GameState& gs, const GameState& ngs)
    {
        const GameAssets* ga = gs.ga.p;
        auto rt = gs.tmp.renderTexFinal;
        gs = ngs;
        setStuff(ga, gs, &rt);
    }

    void _reset(GameState& gs);
    inline void reset(GameState& gs) {
        setState(gs, {0});
        gs.seed = rand() % std::numeric_limits<int>::max();
        _reset(gs);
        gs.gameStartTime = getTime(gs);
    }
    
    DLL_EXPORT void init(GameAssets& ga, GameState& gs)
    {
        if (!IsAudioDeviceReady()) {
            InitAudioDevice();
        } else if (IsMusicValid(ga.music) && IsMusicStreamPlaying(ga.music)) {
            StopMusicStream(ga.music);
        }

        loadAssets(ga, gs);
        PlayMusicStream(ga.music);

        reset(gs);
    }

    void _updateAndDraw(GameState& gs);
    void _drawUI(GameState& gs);
    DLL_EXPORT void updateAndDraw(GameState& gs)
    {
        syncTime(gs);

        if (IsWindowResized())
            setStuff(gs.ga.p, gs);

        BeginTextureMode(gs.tmp.renderTexNative);
        _updateAndDraw(gs);
        EndTextureMode();

        renderPreUI(gs);
        _drawUI(gs);
        renderPostUI(gs);

        gs.time = GetTime();
    }
}