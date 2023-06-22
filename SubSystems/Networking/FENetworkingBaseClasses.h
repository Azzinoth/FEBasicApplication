#pragma once

#include "../FEThreadPool.h"

#include <ws2tcpip.h>
#include <queue>
#include <mutex>

#pragma comment(lib,"ws2_32.lib")

namespace FocalEngine
{
    enum FE_NETWORK_ERROR
    {
        FE_NONE = 0,
        FE_GRACEFULLY_CLOSED = 1,
        FE_ABRUPTLY_DISCONNECTED = 2,
    };

    struct NetworkMessage
    {
        size_t PayloadSize = 0;
        std::vector<char> PartialPayloadSize;
        int MessageType = -1;
        std::vector<char> PartialMessageType;

        std::vector<char> ReceivedData;

        static void WorkOnMessage(char* ReceivedData, int BytesReceived, NetworkMessage* CurrentMessage, std::queue<NetworkMessage>* Messages);
    };

    const int HEADER_SIZE = sizeof(int) * 2;

    struct BufferReader
    {
        char* ReceivedData = nullptr;
        size_t CurrentShift = 0;
        size_t BytesTotal = 0;
        size_t BytesRead = 0;
        size_t BytesLeft = 0;

        BufferReader(char* ReceivedData, size_t BytesReceived);
        void GetBytes(std::vector<char>& Out, size_t Size);
    };

    struct FENetworkSendThreadJobInfo
    {
        char* Data = nullptr;
        size_t DataSize = 0;
        SOCKET* CurrentSocket = nullptr;
        std::function<void(std::string)> OnDataSentCallback = nullptr;

        void* Caller = nullptr;
        FE_NETWORK_ERROR ErrorCode = FE_NONE;
        std::string MessageID = "";
    };

    struct FENetworkReceiveThreadJobInfo
    {
        std::string ReceiveDedicatedThreadID = "";
        NetworkMessage CurrentMessage;
        std::queue<NetworkMessage> Messages;
        SOCKET* CurrentSocket;
        std::function<void(char*, size_t)> OnDataReceivedCallback = nullptr;

        void* Caller = nullptr;
        FE_NETWORK_ERROR ErrorCode = FE_NONE;
    };
}