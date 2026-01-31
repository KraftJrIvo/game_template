#include "game.h"
#include "steam/isteamnetworkingmessages.h"

void GameState::NetCallbacks::OnLobbyCreated(LobbyCreated_t* res, bool ioFailure) {
    if (!gs) 
        return;
    gs->net.lobbyPending = false;
    if (ioFailure || res->m_eResult != k_EResultOK) 
        return;
    gs->net.lobbyId = CSteamID(res->m_ulSteamIDLobby);
    gs->net.inLobby = true;
}

void GameState::NetCallbacks::OnLobbyEnter(LobbyEnter_t* res, bool ioFailure) {
    if (!gs) 
        return;
    gs->net.lobbyPending = false;
    if (ioFailure || res->m_EChatRoomEnterResponse != k_EChatRoomEnterResponseSuccess)
        return;
    gs->net.lobbyId = CSteamID(res->m_ulSteamIDLobby);
    gs->net.inLobby = true;
}

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
    if (!id.IsValid())
        return false;
    out = id;
    return true;
}

void _netClearPeers(GameState& gs) {
    gs.net.peerCount = 0;
    gs.net.peers.fill(CSteamID());
    gs.net.peerPlayerIdx.fill(SIZE_MAX);
}

bool _netFindPeer(GameState& gs, const CSteamID& id, size_t& outIdx) {
    for (size_t i = 0; i < gs.net.peerCount; ++i) {
        if (gs.net.peers[i] == id) {
            outIdx = i;
            return true;
        }
    }
    return false;
}

size_t _netAddPeer(GameState& gs, const CSteamID& id) {
    if (gs.net.peerCount >= MAX_PLAYERS - 1) 
        return SIZE_MAX;
    size_t playerIdx = gs.players.acquire(
        Player{Vector2Zero(), Vector2Zero(), COLORS[(gs.net.peerCount + 1) % COLORS.size()]}
    );
    if (playerIdx == SIZE_MAX) 
        return SIZE_MAX;
    size_t idx = gs.net.peerCount++;
    gs.net.peers[idx] = id;
    gs.net.peerPlayerIdx[idx] = playerIdx;
    return idx;
}

void _netRemovePeer(GameState& gs, size_t idx) {
    if (idx >= gs.net.peerCount) 
        return;
    size_t playerIdx = gs.net.peerPlayerIdx[idx];
    if (playerIdx != SIZE_MAX)
        gs.players.remove(playerIdx);
    size_t last = gs.net.peerCount - 1;
    if (idx != last) {
        gs.net.peers[idx] = gs.net.peers[last];
        gs.net.peerPlayerIdx[idx] = gs.net.peerPlayerIdx[last];
    }
    gs.net.peers[last] = CSteamID();
    gs.net.peerPlayerIdx[last] = SIZE_MAX;
    gs.net.peerCount--;
}

void _netJoinLobbyFromClipboard(GameState& gs) {
    const char* clip = GetClipboardText();
    std::string clipStr = clip ? clip : "";
    CSteamID lobbyId;
    if (!_parseSteamIdText(clipStr.c_str(), lobbyId)) 
        return;
    auto* match = SteamMatchmaking();
    if (!match) 
        return;
    gs.net.lobbyPending = true;
    SteamAPICall_t call = match->JoinLobby(lobbyId);
    gs.net.lobbyEnter->Set(call, &gs.net.netCallbacks, &GameState::NetCallbacks::OnLobbyEnter);
}

void _netCreateLobby(GameState& gs) {
    auto* match = SteamMatchmaking();
    if (!match) 
        return;
    gs.net.lobbyPending = true;
    SteamAPICall_t call = match->CreateLobby(k_ELobbyTypeFriendsOnly, MAX_PLAYERS);
    gs.net.lobbyCreated->Set(call, &gs.net.netCallbacks, &GameState::NetCallbacks::OnLobbyCreated);
}

void _netCopyLobbyIdToClipboard(GameState& gs) {
    if (!gs.net.inLobby) 
        return;
    std::string lobbyText = std::to_string(gs.net.lobbyId.ConvertToUint64());
    SetClipboardText(lobbyText.c_str());
}

void _netSyncLobbyMembers(GameState& gs) {
    if (!gs.net.inLobby) 
        return;
    auto* match = SteamMatchmaking();
    if (!match) 
        return;
    int count = match->GetNumLobbyMembers(gs.net.lobbyId);
    for (int i = 0; i < count; ++i) {
        CSteamID member = match->GetLobbyMemberByIndex(gs.net.lobbyId, i);
        if (SteamUser() && member == SteamUser()->GetSteamID()) 
            continue;
        size_t idx;
        if (!_netFindPeer(gs, member, idx))
            _netAddPeer(gs, member);
    }
    for (size_t i = 0; i < gs.net.peerCount; ) {
        bool stillInLobby = false;
        for (int m = 0; m < count; ++m) {
            CSteamID member = match->GetLobbyMemberByIndex(gs.net.lobbyId, m);
            if (gs.net.peers[i] == member) {
                stillInLobby = true;
                break;
            }
        }
        if (!stillInLobby)
            _netRemovePeer(gs, i);
        else
            ++i;
    }
}

void _netInit(GameState& gs) {
    gs.net.steamOk = SteamAPI_Init();
    if (gs.net.steamOk) {
        gs.net.netCallbacks.gs = &gs;
        gs.net.inLobby = false;
        gs.net.lobbyPending = false;
        gs.net.lobbyId = CSteamID();
        if (!gs.net.lobbyCreated) 
            gs.net.lobbyCreated = new CCallResult<GameState::NetCallbacks, LobbyCreated_t>;
        if (!gs.net.lobbyEnter) 
            gs.net.lobbyEnter = new CCallResult<GameState::NetCallbacks, LobbyEnter_t>;
        _netClearPeers(gs);
    }
}

void _netReceive(GameState& gs) {
    auto* msgs = SteamNetworkingMessages();
    if (!msgs || !gs.net.inLobby) 
        return;
    SteamNetworkingMessage_t* incoming[8];
    int count = msgs->ReceiveMessagesOnChannel(0, incoming, 8);
    if (count <= 0) 
        return;

    for (int i = 0; i < count; ++i) {
        auto* msg = incoming[i];
        if (msg->m_cbSize == sizeof(Player)) {
            CSteamID from = msg->m_identityPeer.GetSteamID();
            size_t idx;
            if (!_netFindPeer(gs, from, idx))
                idx = _netAddPeer(gs, from);
            if (idx != SIZE_MAX) {
                auto* state = (const Player*)msg->m_pData;
                auto playerIdx = gs.net.peerPlayerIdx[idx];
                if (playerIdx != SIZE_MAX)
                    gs.players.at(playerIdx) = *state;
            }
        }
        msg->Release();
    }
}

void _netSend(GameState& gs) {
    auto* msgs = SteamNetworkingMessages();
    if (!msgs || !gs.net.inLobby) 
        return;
    auto now = GetTime();
    if (now - gs.net.lastSendTime < 0.01) 
        return;
    gs.net.lastSendTime = now;

    if (gs.playerIdx >= gs.players.count())
        return;
    auto& player = gs.players.at(gs.playerIdx);
    for (size_t i = 0; i < gs.net.peerCount; ++i) {
        SteamNetworkingIdentity peerIdentity;
        peerIdentity.SetSteamID(gs.net.peers[i]);
        msgs->SendMessageToUser(peerIdentity, &player, sizeof(player),
            k_nSteamNetworkingSend_UnreliableNoDelay, 0);
    }
}

void _netUpdate(GameState& gs) {
    SteamAPI_RunCallbacks();
    _netSyncLobbyMembers(gs);
    _netReceive(gs);
    _netSend(gs);
}