#pragma once

#include "FENetworkingBaseClasses.h"

namespace FocalEngine
{
    struct FENetworkNewClientInfo
    {
        std::string ClientID = "";
        std::string ClientIP = "";
        int ClientPort = -1;
    };

    class FENetworkingManager;
    class FEServerSideNetworkConnection
    {
        struct FENetworkReceiveFromClientThreadJobInfo : public FENetworkReceiveThreadJobInfo
        {
            std::function<void(std::string, char*, size_t)> OnDataReceivedCallback = nullptr;
            std::string ClientID = "";
        };

        struct FENetworkSendToClientThreadJobInfo : public FENetworkSendThreadJobInfo
        {
            std::function<void(std::string, std::string)> OnDataSentCallback = nullptr;
            std::string ClientID = "";
        };

        struct FENetworkPerClientInfo
        {
            std::string SendDedicatedThreadID = "";
            std::string ReceiveDedicatedThreadID = "";

            SOCKET* ClientSocket = nullptr;
            std::string ClientID = "";
            std::string ClientIP = "";
            int ClientPort = -1;

            bool bIsConnectionTerminating = false;
        };

        struct FENetworkServerListeningThreadJobInfo
        {
            std::string ListeningDedicatedThreadID = "";
            SOCKET* ListeningSocket = nullptr;
            SOCKET* NewClientSocket = nullptr;
            std::string ClientID = "";
            std::string ClientIP = "";
            int ClientPort = -1;
            FEServerSideNetworkConnection* Server = nullptr;

            std::function<void(FENetworkNewClientInfo*)> OnNewClientConnectionCallback = nullptr;
        };

        friend FENetworkingManager;

        std::string IP;
        std::string Port;
        SOCKET* ListeningSocket;
        std::unordered_map<std::string, FENetworkPerClientInfo*> Clients;
        void AddClient(FENetworkNewClientInfo* ClientInfo, SOCKET* ClientSocket);
        void RemoveClient(std::string ClientID);

        std::string ListeningDedicatedThreadID = "";

        std::function<void(std::string, std::string)> OnDataSentCallback = nullptr;
        std::function<void(std::string, char*, size_t)> OnDataReceivedCallback = nullptr;
        std::function<void(FENetworkNewClientInfo*)> OnNewClientConnectionCallback = nullptr;
        std::function<void(std::string, bool)> OnClientDisconnectCallback = nullptr;

        FEServerSideNetworkConnection();
        bool TryToBind(std::string IP, unsigned int Port, std::function<void(std::string, std::string)> OnDataSentCallback, std::function<void(std::string, char*, size_t)> OnDataReceivedCallback, std::function<void(FENetworkNewClientInfo*)> OnNewClientConnectionCallback);

        static void ListeningFunction(void* Input, void* Output);
        static void AfterNewClientConnectedFunction(void* OutputData);

        static void SendToClientFunction(void* Input, void* Output);
        static void AfterSendToClientOccurredFunction(void* OutputData);

        static void ReceiveFromClientFunction(void* Input, void* Output);
        static void AfterReceivingDataFromClientFunction(void* OutputData);

        void OnConnectionError(std::string ClientID, FE_NETWORK_ERROR Error);
    public:
		~FEServerSideNetworkConnection();

        void SetOnClientDisconnectCallback(std::function<void(std::string, bool)> OnClientDisconnectCallback);
        FENetworkNewClientInfo GetClientInfo(std::string ClientID);

        std::string Send(std::string ClientID, char* Data, size_t DataSize);
        std::vector<std::string> SendToAll(char* Data, size_t DataSize);

        void DisconnectClient(std::string ClientID);
        void DisconnectAll();
        void Shutdown();
    };
}