
#ifdef __APPLE__
static void CopyWideToWire(uint16_t* dest, const WideChar* src, size_t maxLen) {
    for(size_t i=0; i<maxLen; ++i) {
        dest[i] = (uint16_t)src[i];
        if(src[i] == 0) break;
    }
}
static void CopyWireToWide(WideChar* dest, const uint16_t* src, size_t maxLen) {
    for(size_t i=0; i<maxLen; ++i) {
        dest[i] = (WideChar)src[i];
        if(src[i] == 0) break;
    }
}

void ConvertWireToLANMessage(const LANMessageWire* wire, LANMessage* msg) {
    memset(msg, 0, sizeof(LANMessage));
    msg->messageType = (LANMessage::Type)wire->messageType;
    CopyWireToWide(msg->name, wire->name, ARRAY_SIZE(wire->name));
    memcpy(msg->userName, wire->userName, sizeof(wire->userName));
    memcpy(msg->hostName, wire->hostName, sizeof(wire->hostName));
    
    switch(wire->messageType) {
        case LANMessage::MSG_GAME_START_TIMER:
            msg->StartTimer.seconds = wire->StartTimer.seconds;
            break;
        case LANMessage::MSG_REQUEST_GAME_LEAVE:
            CopyWireToWide(msg->GameToLeave.gameName, wire->GameToLeave.gameName, ARRAY_SIZE(wire->GameToLeave.gameName));
            break;
        case LANMessage::MSG_GAME_ANNOUNCE:
            CopyWireToWide(msg->GameInfo.gameName, wire->GameInfo.gameName, ARRAY_SIZE(wire->GameInfo.gameName));
            msg->GameInfo.inProgress = wire->GameInfo.inProgress;
            memcpy(msg->GameInfo.options, wire->GameInfo.options, sizeof(wire->GameInfo.options));
            msg->GameInfo.isDirectConnect = wire->GameInfo.isDirectConnect;
            break;
        case LANMessage::MSG_REQUEST_GAME_INFO:
            msg->PlayerInfo.ip = wire->PlayerInfo.ip;
            CopyWireToWide(msg->PlayerInfo.playerName, wire->PlayerInfo.playerName, ARRAY_SIZE(wire->PlayerInfo.playerName));
            break;
        case LANMessage::MSG_REQUEST_JOIN:
            msg->GameToJoin.gameIP = wire->GameToJoin.gameIP;
            msg->GameToJoin.exeCRC = wire->GameToJoin.exeCRC;
            msg->GameToJoin.iniCRC = wire->GameToJoin.iniCRC;
            memcpy(msg->GameToJoin.serial, wire->GameToJoin.serial, sizeof(wire->GameToJoin.serial));
            break;
        case LANMessage::MSG_JOIN_ACCEPT:
            CopyWireToWide(msg->GameJoined.gameName, wire->GameJoined.gameName, ARRAY_SIZE(wire->GameJoined.gameName));
            msg->GameJoined.gameIP = wire->GameJoined.gameIP;
            msg->GameJoined.playerIP = wire->GameJoined.playerIP;
            msg->GameJoined.slotPosition = wire->GameJoined.slotPosition;
            break;
        case LANMessage::MSG_JOIN_DENY:
            CopyWireToWide(msg->GameNotJoined.gameName, wire->GameNotJoined.gameName, ARRAY_SIZE(wire->GameNotJoined.gameName));
            msg->GameNotJoined.gameIP = wire->GameNotJoined.gameIP;
            msg->GameNotJoined.playerIP = wire->GameNotJoined.playerIP;
            msg->GameNotJoined.reason = wire->GameNotJoined.reason;
            break;
        case LANMessage::MSG_SET_ACCEPT:
            CopyWireToWide(msg->Accept.gameName, wire->Accept.gameName, ARRAY_SIZE(wire->Accept.gameName));
            msg->Accept.isAccepted = wire->Accept.isAccepted;
            break;
        case LANMessage::MSG_MAP_AVAILABILITY:
            CopyWireToWide(msg->MapStatus.gameName, wire->MapStatus.gameName, ARRAY_SIZE(wire->MapStatus.gameName));
            msg->MapStatus.mapCRC = wire->MapStatus.mapCRC;
            msg->MapStatus.hasMap = wire->MapStatus.hasMap;
            break;
        case LANMessage::MSG_CHAT:
            CopyWireToWide(msg->Chat.gameName, wire->Chat.gameName, ARRAY_SIZE(wire->Chat.gameName));
            msg->Chat.chatType = wire->Chat.chatType;
            CopyWireToWide(msg->Chat.message, wire->Chat.message, ARRAY_SIZE(wire->Chat.message));
            break;
        case LANMessage::MSG_GAME_OPTIONS:
            memcpy(msg->GameOptions.options, wire->GameOptions.options, sizeof(wire->GameOptions.options));
            break;
    }
}

void ConvertLANMessageToWire(const LANMessage* msg, LANMessageWire* wire) {
    memset(wire, 0, sizeof(LANMessageWire));
    wire->messageType = msg->messageType;
    CopyWideToWire(wire->name, msg->name, ARRAY_SIZE(msg->name));
    memcpy(wire->userName, msg->userName, sizeof(msg->userName));
    memcpy(wire->hostName, msg->hostName, sizeof(msg->hostName));
    
    switch(msg->messageType) {
        case LANMessage::MSG_GAME_START_TIMER:
            wire->StartTimer.seconds = msg->StartTimer.seconds;
            break;
        case LANMessage::MSG_REQUEST_GAME_LEAVE:
            CopyWideToWire(wire->GameToLeave.gameName, msg->GameToLeave.gameName, ARRAY_SIZE(msg->GameToLeave.gameName));
            break;
        case LANMessage::MSG_GAME_ANNOUNCE:
            CopyWideToWire(wire->GameInfo.gameName, msg->GameInfo.gameName, ARRAY_SIZE(msg->GameInfo.gameName));
            wire->GameInfo.inProgress = msg->GameInfo.inProgress;
            memcpy(wire->GameInfo.options, msg->GameInfo.options, sizeof(msg->GameInfo.options));
            wire->GameInfo.isDirectConnect = msg->GameInfo.isDirectConnect;
            break;
        case LANMessage::MSG_REQUEST_GAME_INFO:
            wire->PlayerInfo.ip = msg->PlayerInfo.ip;
            CopyWideToWire(wire->PlayerInfo.playerName, msg->PlayerInfo.playerName, ARRAY_SIZE(msg->PlayerInfo.playerName));
            break;
        case LANMessage::MSG_REQUEST_JOIN:
            wire->GameToJoin.gameIP = msg->GameToJoin.gameIP;
            wire->GameToJoin.exeCRC = msg->GameToJoin.exeCRC;
            wire->GameToJoin.iniCRC = msg->GameToJoin.iniCRC;
            memcpy(wire->GameToJoin.serial, msg->GameToJoin.serial, sizeof(msg->GameToJoin.serial));
            break;
        case LANMessage::MSG_JOIN_ACCEPT:
            CopyWideToWire(wire->GameJoined.gameName, msg->GameJoined.gameName, ARRAY_SIZE(msg->GameJoined.gameName));
            wire->GameJoined.gameIP = msg->GameJoined.gameIP;
            wire->GameJoined.playerIP = msg->GameJoined.playerIP;
            wire->GameJoined.slotPosition = msg->GameJoined.slotPosition;
            break;
        case LANMessage::MSG_JOIN_DENY:
            CopyWideToWire(wire->GameNotJoined.gameName, msg->GameNotJoined.gameName, ARRAY_SIZE(msg->GameNotJoined.gameName));
            wire->GameNotJoined.gameIP = msg->GameNotJoined.gameIP;
            wire->GameNotJoined.playerIP = msg->GameNotJoined.playerIP;
            wire->GameNotJoined.reason = msg->GameNotJoined.reason;
            break;
        case LANMessage::MSG_SET_ACCEPT:
            CopyWideToWire(wire->Accept.gameName, msg->Accept.gameName, ARRAY_SIZE(msg->Accept.gameName));
            wire->Accept.isAccepted = msg->Accept.isAccepted;
            break;
        case LANMessage::MSG_MAP_AVAILABILITY:
            CopyWideToWire(wire->MapStatus.gameName, msg->MapStatus.gameName, ARRAY_SIZE(msg->MapStatus.gameName));
            wire->MapStatus.mapCRC = msg->MapStatus.mapCRC;
            wire->MapStatus.hasMap = msg->MapStatus.hasMap;
            break;
        case LANMessage::MSG_CHAT:
            CopyWideToWire(wire->Chat.gameName, msg->Chat.gameName, ARRAY_SIZE(msg->Chat.gameName));
            wire->Chat.chatType = msg->Chat.chatType;
            CopyWideToWire(wire->Chat.message, msg->Chat.message, ARRAY_SIZE(msg->Chat.message));
            break;
        case LANMessage::MSG_GAME_OPTIONS:
            memcpy(wire->GameOptions.options, msg->GameOptions.options, sizeof(msg->GameOptions.options));
            break;
    }
}
#endif

