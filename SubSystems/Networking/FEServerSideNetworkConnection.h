#pragma once

#include "FENetworkingBaseClasses.h"

namespace FocalEngine
{
    struct FENetworkNewClientInfo
    {
        FEUUID ClientID;
        std::string ClientIP = "";
        int ClientPort = -1;
    };

    class FENetworkingManager;
    class FEServerSideNetworkConnection
    {
        struct FENetworkReceiveFromClientThreadJobInfo : public FENetworkReceiveThreadJobInfo
        {
            std::function<void(FEUUID, char*, size_t)> OnDataReceivedCallback = nullptr;
            FEUUID ClientID;
        };

        struct FENetworkSendToClientThreadJobInfo : public FENetworkSendThreadJobInfo
        {
            std::function<void(FEUUID, FEUUID)> OnDataSentCallback = nullptr;
            FEUUID ClientID;
        };

        struct FENetworkPerClientInfo
        {
            FEUUID SendDedicatedThreadID;
            FEUUID ReceiveDedicatedThreadID;

            SOCKET* ClientSocket = nullptr;
            FEUUID ClientID;
            std::string ClientIP = "";
            int ClientPort = -1;

            bool bIsConnectionTerminating = false;
        };

        struct FENetworkServerListeningThreadJobInfo
        {
            FEUUID ListeningDedicatedThreadID;
            SOCKET* ListeningSocket = nullptr;
            SOCKET* NewClientSocket = nullptr;
            FEUUID ClientID;
            std::string ClientIP = "";
            int ClientPort = -1;
            FEServerSideNetworkConnection* Server = nullptr;

            std::function<void(FENetworkNewClientInfo*)> OnNewClientConnectionCallback = nullptr;
        };

        friend FENetworkingManager;

        std::string IP;
        std::string Port;
        SOCKET* ListeningSocket;
        std::unordered_map<FEUUID, FENetworkPerClientInfo*> Clients;
        void AddClient(FENetworkNewClientInfo* ClientInfo, SOCKET* ClientSocket);
        void RemoveClient(FEUUID ClientID);

        FEUUID ListeningDedicatedThreadID;

        std::function<void(FEUUID, FEUUID)> OnDataSentCallback = nullptr;
        std::function<void(FEUUID, char*, size_t)> OnDataReceivedCallback = nullptr;
        std::function<void(FENetworkNewClientInfo*)> OnNewClientConnectionCallback = nullptr;
        std::function<void(FEUUID, bool)> OnClientDisconnectCallback = nullptr;

        FEServerSideNetworkConnection();
        bool TryToBind(std::string IP, unsigned int Port, std::function<void(FEUUID, FEUUID)> OnDataSentCallback, std::function<void(FEUUID, char*, size_t)> OnDataReceivedCallback, std::function<void(FENetworkNewClientInfo*)> OnNewClientConnectionCallback);

        static void ListeningFunction(void* Input, void* Output);
        static void AfterNewClientConnectedFunction(void* OutputData);

        static void SendToClientFunction(void* Input, void* Output);
        static void AfterSendToClientOccurredFunction(void* OutputData);

        static void ReceiveFromClientFunction(void* Input, void* Output);
        static void AfterReceivingDataFromClientFunction(void* OutputData);

        void OnConnectionError(FEUUID ClientID, FE_NETWORK_ERROR Error);
    public:
		~FEServerSideNetworkConnection();

        void SetOnClientDisconnectCallback(std::function<void(FEUUID, bool)> OnClientDisconnectCallback);
        FENetworkNewClientInfo GetClientInfo(FEUUID ClientID);

        FEUUID Send(FEUUID ClientID, char* Data, size_t DataSize);
        std::vector<FEUUID> SendToAll(char* Data, size_t DataSize);

        void DisconnectClient(FEUUID ClientID);
        void DisconnectAll();
        void Shutdown();
    };
}