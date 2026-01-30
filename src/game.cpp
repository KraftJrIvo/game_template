#include "game.h"
#include "game_basic.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#include "util/vec_ops.h"
#include <cstdint>
#include <string>

#include "resources.h"

#include "steam/isteamnetworkingmessages.h"

bool _parseSteamIdText(const char* text, CSteamID& out) {
    if (!text) 
        return false;
    std::string s(text);
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return false;
    auto end = s.find_last_not_of(" \t\r\n");
    s = s.substr(start, end - start + 1);
    if (s.empty()) 
        return false;
    std::string digits;
    digits.reserve(s.size());
    for (char c : s)
        if (c >= '0' && c <= '9') 
            digits.push_back(c);
    if (digits.empty()) 
        return false;
    uint64 id64 = 0;
    try {
        id64 = std::stoull(digits);
    } catch (...) {
        return false;
    }
    CSteamID id(id64);
    if (!id.IsValid() || !id.BIndividualAccount()) 
        return false;
    out = id;
    return true;
}

void _netConnectByClipboard(GameState& gs) {
    const char* clip = GetClipboardText();
    std::string clipStr = clip ? clip : "";
    CSteamID peerId;
    if (!_parseSteamIdText(clipStr.c_str(), peerId)) return;
    if (SteamUser() && peerId == SteamUser()->GetSteamID()) return;
    gs.net.peerIdentity.Clear();
    gs.net.peerIdentity.SetSteamID(peerId);
    gs.net.hasPeer = true;
    if (gs.net.remoteIdx == SIZE_MAX)
        gs.net.remoteIdx = gs.players.acquire(Player{Vector2Zero(), Vector2Zero(), COLORS[0]});
}

void _netInit(GameState& gs) {
    gs.net.steamOk = SteamAPI_Init();
    if (gs.net.steamOk){
        gs.net.peerIdentity.Clear();
        _netConnectByClipboard(gs);
    }
}

void _netReceive(GameState& gs) {
    auto* msgs = SteamNetworkingMessages();
    if (!msgs || !gs.net.hasPeer) return;
    SteamNetworkingMessage_t* incoming[8];
    int count = msgs->ReceiveMessagesOnChannel(0, incoming, 8);
    if (count <= 0) return;

    CSteamID expected = gs.net.peerIdentity.GetSteamID();
    for (int i = 0; i < count; ++i) {
        auto* msg = incoming[i];
        if (msg->m_cbSize == sizeof(Player)) {
            CSteamID from = msg->m_identityPeer.GetSteamID();
            if (from == expected) {
                auto* state = (const Player*)msg->m_pData;
                if (gs.net.remoteIdx == SIZE_MAX) 
                    gs.net.remoteIdx = gs.players.acquire(Player{Vector2Zero(), Vector2Zero(), COLORS[0]});
                gs.players.at(gs.net.remoteIdx) = *state;
            }
        }
        msg->Release();
    }
}

void _netSend(GameState& gs) {
    auto* msgs = SteamNetworkingMessages();
    if (!msgs || !gs.net.hasPeer) return;
    auto now = GetTime();
    if (now - gs.net.lastSendTime < 0.05) return;
    gs.net.lastSendTime = now;

    if (gs.playerIdx >= gs.players.count()) return;
    auto& player = gs.players.at(gs.playerIdx);
    msgs->SendMessageToUser(gs.net.peerIdentity, &player, sizeof(player),
        k_nSteamNetworkingSend_UnreliableNoDelay, 0);
}

void _netUpdate(GameState& gs) {
    if (!gs.net.steamOk) return;
    SteamAPI_RunCallbacks();
    _netReceive(gs);
    _netSend(gs);
}

void _loadAssets(GameAssets& ga, GameState& gs) 
{
    ga.sprite = LoadTextureFromImage(LoadImageFromMemory(".png", res_sprite_png, res_sprite_png_len));
    
    const char8_t allChars[228] = u8" !\"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~абвгдеёжзийклмнопрстуфхцчшщъыьэюяАБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ";
    int c; auto cdpts = LoadCodepoints((const char*)allChars, &c);
    ga.font = LoadFontFromMemory(".otf", res_font_otf, res_font_otf_len, 13, cdpts, c);

    ga.spriteSz = Vector2{100.0f, 100.0f};
}

void _reset(GameState& gs) {
    auto pos = Vector2{float(GetScreenWidth()), float(GetScreenHeight())} * 0.5f;
    gs.playerIdx = gs.players.acquire(Player{pos, Vector2Zero(), COLORS[gs.usr.spriteColor]});
    _netInit(gs);
}

void _updateAndDraw(GameState& gs) 
{
    _netUpdate(gs);

    auto delta = GetFrameTime();
    if (delta < 0.1f) {
        auto& player = gs.players.at(gs.playerIdx);
        auto vel = Vector2{
            float(IsKeyDown(KEY_RIGHT) - IsKeyDown(KEY_LEFT)),
            float(IsKeyDown(KEY_DOWN) - IsKeyDown(KEY_UP))
        } * SPEED;
        player.vel = Vector2Lerp(player.vel, vel, 0.1f);
        player.pos += player.vel * delta;
        auto r = gs.ga.p->spriteSz.x * 0.5f;
        auto w = gs.tmp.renderTexNative.texture.width;
        auto h = gs.tmp.renderTexNative.texture.height;
        player.pos.x = std::max(player.pos.x, r);
        player.pos.y = std::max(player.pos.y, r);
        player.pos.x = std::min(player.pos.x, w - r);
        player.pos.y = std::min(player.pos.y, h - r);


        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            gs.usr.spriteColor = (gs.usr.spriteColor + 1) % COLORS.size();
            saveUserData(gs);
            player.color = COLORS[gs.usr.spriteColor];
        }
    }

    ClearBackground(BLACK);
    for (int i = 0; i < gs.players.count(); ++i) {
        auto& player = gs.players.at_raw(i);
        DrawTextureV(gs.ga.p->sprite, player.pos - gs.ga.p->spriteSz * 0.5f, player.color);
    }
}

void _drawUI(GameState& gs) {
    DrawRectangleLines(0, 0, GAME_WIDTH, GAME_HEIGHT, WHITE);
    DrawTextEx(gs.ga.p->font, std::to_string(gs.time).c_str(), {10.0f, 10.0f}, 13, 1, WHITE);
    if (gs.net.steamOk) {
        auto myId = SteamUser()->GetSteamID().ConvertToUint64();
        std::string myText = "SteamID: " + std::to_string(myId);
        DrawTextEx(gs.ga.p->font, myText.c_str(), {10.0f, 26.0f}, 13, 1, WHITE);

        std::string peerText = "Peer: (clipboard invalid)";
        if (gs.net.hasPeer) {
            auto peerId = gs.net.peerIdentity.GetSteamID().ConvertToUint64();
            peerText = "Peer: " + std::to_string(peerId);
        }
        DrawTextEx(gs.ga.p->font, peerText.c_str(), {10.0f, 42.0f}, 13, 1, WHITE);
    } else {
        DrawTextEx(gs.ga.p->font, "Steam API init failed", {10.0f, 26.0f}, 13, 1, RED);
    }
}
