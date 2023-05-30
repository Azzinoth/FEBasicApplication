#include "FENetworking.h"

using namespace FocalEngine;
FENetworkingManager* FENetworkingManager::Instance = nullptr;
FENetworkingManager::FENetworkingManager()
{
    WSADATA WinSockData;
    int StartupResult = WSAStartup(MAKEWORD(2, 2), &WinSockData);
    if (StartupResult != 0)
        bStartedCorrectly = false;

    bStartedCorrectly = true;
};

FENetworkingManager::~FENetworkingManager()
{
    WSACleanup();
};

void FENetworkingManager::CreateClient(std::function<void(FEClientSideNetworkConnection*)> ResultReadyCallback, 
                                                 std::string ServerIP, unsigned int ServerPort,
                                                 std::function<void(std::string)> OnDataSentCallback,
                                                 std::function<void(char*, size_t)> OnDataReceivedCallback)
{
    FENetworkCreateClientJobInfo* JobInfo = new FENetworkCreateClientJobInfo;
    JobInfo->ServerIP = ServerIP;
    JobInfo->ServerPort = ServerPort;
    JobInfo->OnDataSentCallback = OnDataSentCallback;
    JobInfo->OnDataReceivedCallback = OnDataReceivedCallback;
    JobInfo->ResultReadyCallback = ResultReadyCallback;

    THREAD_POOL.Execute(TryToCreateClient, (void*)JobInfo, (void*)JobInfo, AfterClientCreated);
}

void FENetworkingManager::TryToCreateClient(void* Input, void* Output)
{
    FENetworkCreateClientJobInfo* JobInfo = (FENetworkCreateClientJobInfo*)Input;
    JobInfo->Result = new FEClientSideNetworkConnection;
    if (!JobInfo->Result->TryToConnect(JobInfo->ServerIP, JobInfo->ServerPort, JobInfo->OnDataSentCallback, JobInfo->OnDataReceivedCallback))
    {
        delete JobInfo->Result;
        JobInfo->Result = nullptr;
    }
}

void FENetworkingManager::AfterClientCreated(void* OutputData)
{
    FENetworkCreateClientJobInfo* JobInfo = (FENetworkCreateClientJobInfo*)OutputData;

    if (JobInfo->ResultReadyCallback)
		JobInfo->ResultReadyCallback(JobInfo->Result);

    if (JobInfo->Result != nullptr)
        NETWORKING_MANAGER.ClientConnections.push_back(JobInfo->Result);

    delete JobInfo;
}

void FENetworkingManager::CreateServer(std::function<void(FEServerSideNetworkConnection*)> ResultReadyCallback,
                                       std::string IP, unsigned int Port,
                                       std::function<void(std::string, std::string)> OnDataSentCallback,
                                       std::function<void(std::string, char*, size_t)> OnDataReceivedCallback,
                                       std::function<void(FENetworkNewClientInfo*)> OnNewClientConnectionCallback)
{
    FENetworkCreateServerJobInfo* JobInfo = new FENetworkCreateServerJobInfo;
    JobInfo->IP = IP;
    JobInfo->Port = Port;
    JobInfo->OnDataSentCallback = OnDataSentCallback;
    JobInfo->OnDataReceivedCallback = OnDataReceivedCallback;
    JobInfo->OnNewClientConnectionCallback = OnNewClientConnectionCallback;
    JobInfo->ResultReadyCallback = ResultReadyCallback;

    THREAD_POOL.Execute(TryToCreateServer, (void*)JobInfo, (void*)JobInfo, AfterServerCreated);

    /*FEServerSideNetworkConnection* Server = new FEServerSideNetworkConnection;
    if (!Server->TryToBind(IP, Port, OnDataSentCallback, OnDataReceivedCallback, OnNewClientConnectionCallback))
    {
        delete Server;
        return nullptr;
    }

    ServerConnections.push_back(Server);
    return Server;*/
}

void FENetworkingManager::TryToCreateServer(void* Input, void* Output)
{
    FENetworkCreateServerJobInfo* JobInfo = (FENetworkCreateServerJobInfo*)Input;
    JobInfo->Result = new FEServerSideNetworkConnection;
    if (!JobInfo->Result->TryToBind(JobInfo->IP, JobInfo->Port, JobInfo->OnDataSentCallback, JobInfo->OnDataReceivedCallback, JobInfo->OnNewClientConnectionCallback))
    {
        delete JobInfo->Result;
        JobInfo->Result = nullptr;
    }
}

void FENetworkingManager::AfterServerCreated(void* OutputData)
{
    FENetworkCreateServerJobInfo* JobInfo = (FENetworkCreateServerJobInfo*)OutputData;

    if (JobInfo->ResultReadyCallback)
        JobInfo->ResultReadyCallback(JobInfo->Result);

    if (JobInfo->Result != nullptr)
        NETWORKING_MANAGER.ServerConnections.push_back(JobInfo->Result);

    delete JobInfo;
}