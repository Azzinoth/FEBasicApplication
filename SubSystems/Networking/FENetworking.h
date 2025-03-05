#pragma once

#include "FEClientSideNetworkConnection.h"
#include "FEServerSideNetworkConnection.h"

namespace FocalEngine
{
    class FENetworkingManager
    {
    public:
        SINGLETON_PUBLIC_PART(FENetworkingManager)

        void CreateClient(std::function<void(FEClientSideNetworkConnection*)> ResultReadyCallback, std::string ServerIP, unsigned int ServerPort, std::function<void(std::string)> OnDataSentCallback, std::function<void(char*, size_t)> OnDataReceivedCallback);
        void CreateServer(std::function<void(FEServerSideNetworkConnection*)> ResultReadyCallback, std::string IP, unsigned int Port, std::function<void(std::string, std::string)> OnDataSentCallback, std::function<void(std::string, char*, size_t)> OnDataReceivedCallback, std::function<void(FENetworkNewClientInfo*)> OnNewClientConnectionCallback);
    private:
        SINGLETON_PRIVATE_PART(FENetworkingManager)

        bool bStartedCorrectly = false;

        // CLIENT PART
        struct FENetworkCreateClientJobInfo
        {
            std::string ServerIP = "";
            unsigned int ServerPort = 0;
            
            FEClientSideNetworkConnection* Result = nullptr;
            std::function<void(std::string)> OnDataSentCallback = nullptr;
            std::function<void(char*, size_t)> OnDataReceivedCallback = nullptr;

            std::function<void(FEClientSideNetworkConnection*)> ResultReadyCallback = nullptr;
        };

        static void TryToCreateClient(void* Input, void* Output);
        static void AfterClientCreated(void* OutputData);

        std::vector<FEClientSideNetworkConnection*> ClientConnections;

        // SERVER PART
        struct FENetworkCreateServerJobInfo
        {
            std::string IP = "";
            unsigned int Port = 0;

            FEServerSideNetworkConnection* Result = nullptr;
            std::function<void(std::string, std::string)> OnDataSentCallback = nullptr;
            std::function<void(std::string, char*, size_t)> OnDataReceivedCallback = nullptr;
            std::function<void(FENetworkNewClientInfo*)> OnNewClientConnectionCallback = nullptr;

            std::function<void(FEServerSideNetworkConnection*)> ResultReadyCallback = nullptr;
        };

        static void TryToCreateServer(void* Input, void* Output);
        static void AfterServerCreated(void* OutputData);

        std::vector<FEServerSideNetworkConnection*> ServerConnections;
    };

#ifdef FEBASICAPPLICATION_SHARED
    extern "C" __declspec(dllexport) void* GetNetworkingManager();
    #define NETWORKING_MANAGER (*static_cast<FENetworkingManager*>(GetNetworkingManager()))
#else
    #define NETWORKING_MANAGER FENetworkingManager::GetInstance()
#endif
}