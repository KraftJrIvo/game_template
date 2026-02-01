#include "game.h"

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
    if (now - gs.net.lastSendTime < 0.01) return;
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