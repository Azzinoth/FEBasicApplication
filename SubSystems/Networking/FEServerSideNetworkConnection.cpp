#include "FEServerSideNetworkConnection.h"
using namespace FocalEngine;

FEServerSideNetworkConnection::FEServerSideNetworkConnection() {};
FEServerSideNetworkConnection::~FEServerSideNetworkConnection()
{
    Shutdown();
};

bool FEServerSideNetworkConnection::TryToBind(std::string IP, unsigned int Port, std::function<void(std::string, std::string)> OnDataSentCallback, std::function<void(std::string, char*, size_t)> OnDataReceivedCallback, std::function<void(FENetworkNewClientInfo*)> OnNewClientConnectionCallback)
{
    if (OnDataSentCallback == nullptr ||
        OnDataReceivedCallback == nullptr ||
        OnNewClientConnectionCallback == nullptr)
        return false;

    ListeningSocket = new SOCKET;
    *ListeningSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in ServerAddresses{};
    ServerAddresses.sin_family = AF_INET;
    ServerAddresses.sin_port = htons(Port);
    ServerAddresses.sin_addr.S_un.S_addr = INADDR_ANY;

    int BindResult = bind(*ListeningSocket, (SOCKADDR*)&ServerAddresses, sizeof(ServerAddresses));
    if (BindResult == SOCKET_ERROR)
    {
        closesocket(*ListeningSocket);
        LOG.Add("Binding to a IP: " + IP + " on port: " + std::to_string(Port) + " failed with error : " + std::to_string(WSAGetLastError()), "FE_NETWORKING");
        return false;
    }

    if (listen(*ListeningSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        int error_code = WSAGetLastError();
        LOG.Add("Listen failed with error : " + std::to_string(WSAGetLastError()), "FE_NETWORKING");
        closesocket(*ListeningSocket);
    }

    IP = IP;
    Port = Port;
    this->OnDataReceivedCallback = OnDataReceivedCallback;
    this->OnDataSentCallback = OnDataSentCallback;
    this->OnNewClientConnectionCallback = OnNewClientConnectionCallback;

    ListeningDedicatedThreadID = THREAD_POOL.CreateDedicatedThread();

    FENetworkServerListeningThreadJobInfo* ListeningJobInfo = new FENetworkServerListeningThreadJobInfo;
    ListeningJobInfo->ListeningSocket = ListeningSocket;
    ListeningJobInfo->ListeningDedicatedThreadID = ListeningDedicatedThreadID;
    ListeningJobInfo->OnNewClientConnectionCallback = OnNewClientConnectionCallback;
    ListeningJobInfo->Server = this;
    THREAD_POOL.Execute(ListeningDedicatedThreadID, ListeningFunction, (void*)ListeningJobInfo, (void*)ListeningJobInfo, AfterNewClientConnectedFunction);

    return true;
}

void FEServerSideNetworkConnection::ListeningFunction(void* Input, void* Output)
{
    if (Input == nullptr)
        return;

    FENetworkServerListeningThreadJobInfo* Info = (FENetworkServerListeningThreadJobInfo*)Input;
    if (Info->ListeningSocket == nullptr)
        return;

    struct sockaddr_in ClientAddress;
    int ClientAddressLength = sizeof(ClientAddress);

    Info->NewClientSocket = new SOCKET;
    *Info->NewClientSocket = accept(*Info->ListeningSocket, (struct sockaddr*)&ClientAddress, &ClientAddressLength);
    if (*Info->NewClientSocket == INVALID_SOCKET)
    {
        delete Info->NewClientSocket;
        Info->NewClientSocket = nullptr;
		LOG.Add("New client connection failed with error code : " + std::to_string(WSAGetLastError()), "FE_NETWORKING");
        return;
	}

    Info->ClientID = UNIQUE_ID.GetUniqueHexID();
    char IPString[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(ClientAddress.sin_addr), IPString, INET_ADDRSTRLEN);
    Info->ClientIP = IPString;
    Info->ClientPort = ntohs(ClientAddress.sin_port);
}

void FEServerSideNetworkConnection::AddClient(FENetworkNewClientInfo* ClientInfo, SOCKET* ClientSocket)
{
    if (ClientInfo == nullptr || ClientSocket == nullptr)
		return;

    if (Clients.find(ClientInfo->ClientID) != Clients.end())
    {
        LOG.Add("Trying to add client to a server that already have client with such ID.", "FE_NETWORKING");
    }

    Clients[ClientInfo->ClientID] = new FENetworkPerClientInfo;
    Clients[ClientInfo->ClientID]->ClientID = ClientInfo->ClientID;
    Clients[ClientInfo->ClientID]->ClientIP = ClientInfo->ClientIP;
    Clients[ClientInfo->ClientID]->ClientPort = ClientInfo->ClientPort;
    Clients[ClientInfo->ClientID]->ClientSocket = ClientSocket;

    Clients[ClientInfo->ClientID]->SendDedicatedThreadID = THREAD_POOL.CreateDedicatedThread();
    Clients[ClientInfo->ClientID]->ReceiveDedicatedThreadID = THREAD_POOL.CreateDedicatedThread();

    FENetworkReceiveFromClientThreadJobInfo* ReceiveJobInfo = new FENetworkReceiveFromClientThreadJobInfo;
    ReceiveJobInfo->CurrentSocket = Clients[ClientInfo->ClientID]->ClientSocket;
    ReceiveJobInfo->OnDataReceivedCallback = OnDataReceivedCallback;
    ReceiveJobInfo->ReceiveDedicatedThreadID = Clients[ClientInfo->ClientID]->ReceiveDedicatedThreadID;
    ReceiveJobInfo->ClientID = ClientInfo->ClientID;
    ReceiveJobInfo->Caller = (void*)this;
    THREAD_POOL.Execute(Clients[ClientInfo->ClientID]->ReceiveDedicatedThreadID, ReceiveFromClientFunction, (void*)ReceiveJobInfo, (void*)ReceiveJobInfo, AfterReceivingDataFromClientFunction);
}

void FEServerSideNetworkConnection::AfterNewClientConnectedFunction(void* OutputData)
{
    FENetworkServerListeningThreadJobInfo* Info = (FENetworkServerListeningThreadJobInfo*)OutputData;

    if (Info->NewClientSocket != nullptr)
    {
		FENetworkNewClientInfo* NewClientInfo = new FENetworkNewClientInfo;
		NewClientInfo->ClientID = Info->ClientID;
		NewClientInfo->ClientIP = Info->ClientIP;
		NewClientInfo->ClientPort = Info->ClientPort;

        Info->Server->AddClient(NewClientInfo, Info->NewClientSocket);
        
		if (Info->OnNewClientConnectionCallback != nullptr)
			Info->OnNewClientConnectionCallback(NewClientInfo);
	}

    Info->NewClientSocket = nullptr;
    Info->ClientID = "";
    Info->ClientIP = "";
    Info->ClientPort = 0;
        
    THREAD_POOL.Execute(Info->ListeningDedicatedThreadID, FEServerSideNetworkConnection::ListeningFunction, OutputData, OutputData, FEServerSideNetworkConnection::AfterNewClientConnectedFunction);
}

void FEServerSideNetworkConnection::SendToClientFunction(void* Input, void* Output)
{
    if (Input == nullptr)
        return;

    FENetworkSendToClientThreadJobInfo* Info = (FENetworkSendToClientThreadJobInfo*)Input;
    if (Info->DataSize == 0 || Info->Data == nullptr)
        return;

    size_t MessageSize = Info->DataSize + sizeof(size_t) + sizeof(int);
    int MessageType = 1;
    if (MessageType < 0)
        MessageType = 0;
    MessageType += 1; // To ensure that it can not be 0.

    // In order not to reallocate memory for each data transfer,
    // we will send header part before data
    // And since TCP will ensure that data will be received in the same order as sent, this aproach is safe.
    int Result = send(*Info->CurrentSocket, (char*)&MessageSize, sizeof(size_t), 0);
    if (Result == -1)
    {
        Info->ErrorCode = FE_ABRUPTLY_DISCONNECTED;
        return;
    }

    Result = send(*Info->CurrentSocket, (char*)&MessageType, sizeof(int), 0);
    if (Result == -1)
    {
        Info->ErrorCode = FE_ABRUPTLY_DISCONNECTED;
        return;
    }

    // This is the maximum size that send function can handle.
    const size_t MAX_CHUNK_SIZE = 2147483647;  

    for (size_t i = 0; i < MessageSize; i += MAX_CHUNK_SIZE)
    {
        size_t CurrentChunkSize = MAX_CHUNK_SIZE > MessageSize - i ? MessageSize - i : MAX_CHUNK_SIZE;
        Result = send(*Info->CurrentSocket, (char*)Info->Data + i, static_cast<int>(CurrentChunkSize), 0);
        if (Result == -1)
        {
            Info->ErrorCode = FE_ABRUPTLY_DISCONNECTED;
            return;
        }
    }
}

void FEServerSideNetworkConnection::AfterSendToClientOccurredFunction(void* OutputData)
{
    if (OutputData == nullptr)
        return;

    FENetworkSendToClientThreadJobInfo* Info = (FENetworkSendToClientThreadJobInfo*)OutputData;
    if (Info->ErrorCode != FE_NONE)
    {
        reinterpret_cast<FEServerSideNetworkConnection*>(Info->Caller)->OnConnectionError(Info->ClientID, Info->ErrorCode);
        return;
    }

    reinterpret_cast<FEServerSideNetworkConnection*>(Info->Caller)->OnDataSentCallback(Info->ClientID, Info->MessageID);
    delete Info;
}

std::string FEServerSideNetworkConnection::Send(std::string ClientID, char* Data, size_t DataSize)
{
    if (Data == nullptr || DataSize == 0)
        return "";

    if (Clients.find(ClientID) == Clients.end())
    {
        LOG.Add("Trying to send data to unknown client.", "FE_NETWORKING");
        return "";
    }

    FENetworkSendToClientThreadJobInfo* SendJobInfo = new FENetworkSendToClientThreadJobInfo;
    SendJobInfo->CurrentSocket = Clients[ClientID]->ClientSocket;
    SendJobInfo->Data = Data;
    SendJobInfo->DataSize = DataSize;
    SendJobInfo->OnDataSentCallback = OnDataSentCallback;
    SendJobInfo->ClientID = ClientID;
    SendJobInfo->Caller = (void*)this;
    SendJobInfo->MessageID = UNIQUE_ID.GetUniqueHexID();

    THREAD_POOL.Execute(Clients[ClientID]->SendDedicatedThreadID, FEServerSideNetworkConnection::SendToClientFunction, (void*)SendJobInfo, (void*)SendJobInfo, FEServerSideNetworkConnection::AfterSendToClientOccurredFunction);
    return SendJobInfo->MessageID;
}

std::vector<std::string> FEServerSideNetworkConnection::SendToAll(char* Data, size_t DataSize)
{
    std::vector<std::string> MessageIDs;
    if (Data == nullptr || DataSize == 0)
        return MessageIDs;

    auto Iterator = Clients.begin();
    while (Iterator != Clients.end())
    {
        if (Iterator->second->ClossingConnection)
            continue;

        FENetworkSendToClientThreadJobInfo* SendJobInfo = new FENetworkSendToClientThreadJobInfo;
        SendJobInfo->CurrentSocket = Clients[Iterator->first]->ClientSocket;
        SendJobInfo->Data = Data;
        SendJobInfo->DataSize = DataSize;
        SendJobInfo->OnDataSentCallback = OnDataSentCallback;
        SendJobInfo->ClientID = Iterator->first;
        SendJobInfo->Caller = (void*)this;
        SendJobInfo->MessageID = UNIQUE_ID.GetUniqueHexID();

        THREAD_POOL.Execute(Clients[Iterator->first]->SendDedicatedThreadID, FEServerSideNetworkConnection::SendToClientFunction, (void*)SendJobInfo, (void*)SendJobInfo, FEServerSideNetworkConnection::AfterSendToClientOccurredFunction);
        Iterator++;
        MessageIDs.push_back(SendJobInfo->MessageID);
    }

    return MessageIDs;
}

void FEServerSideNetworkConnection::ReceiveFromClientFunction(void* Input, void* Output)
{
    if (Input == nullptr)
        return;

    FENetworkReceiveFromClientThreadJobInfo* Info = (FENetworkReceiveFromClientThreadJobInfo*)Input;

    int SizeOfTempBuffer = 100000;
    char* Buffer = new char[SizeOfTempBuffer];
    int BytesReceived;

    while ((BytesReceived = recv(*Info->CurrentSocket, Buffer, SizeOfTempBuffer, 0)) > 0)
    {
        NetworkMessage::WorkOnMessage(Buffer, BytesReceived, &Info->CurrentMessage, &Info->Messages);

        if (Info->Messages.size() != 0)
        {
            delete[] Buffer;
            return;
        }
    }

    delete[] Buffer;
    if (BytesReceived == 0)
    {
        Info->ErrorCode = FE_GRACEFULLY_CLOSED;
	}
    else if (BytesReceived == SOCKET_ERROR)
    {
        Info->ErrorCode = FE_ABRUPTLY_DISCONNECTED;
	}
}

void FEServerSideNetworkConnection::AfterReceivingDataFromClientFunction(void* OutputData)
{
    FENetworkReceiveFromClientThreadJobInfo* Info = (FENetworkReceiveFromClientThreadJobInfo*)OutputData;

    if (Info->ErrorCode != FE_NONE)
    {
        reinterpret_cast<FEServerSideNetworkConnection*>(Info->Caller)->OnConnectionError(Info->ClientID, Info->ErrorCode);
        return;
    }

    while (!Info->Messages.empty())
    {
        auto CurrentMessage = Info->Messages.front();

        reinterpret_cast<FEServerSideNetworkConnection*>(Info->Caller)->OnDataReceivedCallback(Info->ClientID, CurrentMessage.ReceivedData.data(), CurrentMessage.ReceivedData.size());
        Info->Messages.pop();
    }

    std::string ThreadID = reinterpret_cast<FEServerSideNetworkConnection*>(Info->Caller)->Clients[Info->ClientID]->ReceiveDedicatedThreadID;
    THREAD_POOL.Execute(ThreadID, FEServerSideNetworkConnection::ReceiveFromClientFunction, OutputData, OutputData, FEServerSideNetworkConnection::AfterReceivingDataFromClientFunction);
}

void FEServerSideNetworkConnection::OnConnectionError(std::string ClientID, FE_NETWORK_ERROR Error)
{
    if (!Clients[ClientID]->ClossingConnection)
    {
        Clients[ClientID]->ClossingConnection = true;
        
        if (OnClientDisconnectCallback != nullptr)
        {
            OnClientDisconnectCallback(ClientID, Error == FE_GRACEFULLY_CLOSED);
        }

        RemoveClient(ClientID);
    }
}

void FEServerSideNetworkConnection::RemoveClient(std::string ClientID)
{
    THREAD_POOL.ShutdownDedicatedThread(Clients[ClientID]->ReceiveDedicatedThreadID);
    THREAD_POOL.ShutdownDedicatedThread(Clients[ClientID]->SendDedicatedThreadID);

    closesocket(*Clients[ClientID]->ClientSocket);

    auto TempPointer = Clients[ClientID];
    Clients.erase(ClientID);
    delete TempPointer;
}

void FEServerSideNetworkConnection::SetOnClientDisconnectCallback(std::function<void(std::string, bool)> OnClientDisconnectCallback)
{
    this->OnClientDisconnectCallback = OnClientDisconnectCallback;
}

FENetworkNewClientInfo FEServerSideNetworkConnection::GetClientInfo(std::string ClientID)
{
    FENetworkNewClientInfo Result;
    if (Clients.find(ClientID) == Clients.end())
        return Result;

    Result.ClientID = Clients[ClientID]->ClientID;
    Result.ClientIP = Clients[ClientID]->ClientIP;
    Result.ClientPort = Clients[ClientID]->ClientPort;

    return Result;
}

void FEServerSideNetworkConnection::DisconnectClient(std::string ClientID)
{
    if (Clients.find(ClientID) == Clients.end())
        return;

    shutdown(*Clients[ClientID]->ClientSocket, SD_SEND);
    if (OnClientDisconnectCallback != nullptr)
        OnClientDisconnectCallback(ClientID, true);
    RemoveClient(ClientID);
}

void FEServerSideNetworkConnection::DisconnectAll()
{
    auto Iterator = Clients.begin();
    while (Iterator != Clients.end())
    {
        shutdown(*Clients[Iterator->first]->ClientSocket, SD_SEND);
        if (OnClientDisconnectCallback != nullptr)
            OnClientDisconnectCallback(Iterator->first, true);
        THREAD_POOL.ShutdownDedicatedThread(Clients[Iterator->first]->ReceiveDedicatedThreadID);
        THREAD_POOL.ShutdownDedicatedThread(Clients[Iterator->first]->SendDedicatedThreadID);
        closesocket(*Clients[Iterator->first]->ClientSocket);
        delete Clients[Iterator->first];

        Iterator++;
    }
}

void FEServerSideNetworkConnection::Shutdown()
{
    THREAD_POOL.ShutdownDedicatedThread(ListeningDedicatedThreadID);

    shutdown(*ListeningSocket, SD_SEND);
    closesocket(*ListeningSocket);

    DisconnectAll();
    Clients.clear();
}