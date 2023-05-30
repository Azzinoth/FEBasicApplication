#pragma once

#include "FENetworkingBaseClasses.h"

namespace FocalEngine
{
    class FENetworkingManager;
    class FEClientSideNetworkConnection
    {
        friend FENetworkingManager;

        std::string IP;
        std::string Port;
        SOCKET* Socket;

        std::string SendDedicatedThreadID = "";
        std::string ReceiveDedicatedThreadID = "";

        std::function<void(std::string)> OnDataSentCallback = nullptr;
        std::function<void(char*, size_t)> OnDataReceivedCallback = nullptr;
        std::function<void(bool)> OnDisconnectCallback = nullptr;

        FEClientSideNetworkConnection();
        bool TryToConnect(std::string ServerIP, unsigned int ServerPort, std::function<void(std::string)> OnDataSentCallback, std::function<void(char*, size_t)> OnDataReceivedCallback);

        static void SendFunction(void* Input, void* Output);
        static void AfterSendOccurredFunction(void* OutputData);

        static void ReceiveFunction(void* Input, void* Output);
        static void AfterReceivingDataFunction(void* OutputData);

        void OnConnectionError(FE_NETWORK_ERROR Error);
        void Clear();
    public:
		~FEClientSideNetworkConnection();

        std::string Send(char* Data, size_t DataSize);

        void Disconnect(FE_NETWORK_ERROR Error = FE_NETWORK_ERROR::FE_NONE);
        void SetOnDisconnectCallback(std::function<void(bool)> OnDisconnectCallback);
    };
}