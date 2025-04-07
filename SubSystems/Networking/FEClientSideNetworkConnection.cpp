#include "FEClientSideNetworkConnection.h"
using namespace FocalEngine;

FEClientSideNetworkConnection::FEClientSideNetworkConnection() {};
FEClientSideNetworkConnection::~FEClientSideNetworkConnection()
{
    Clear();
};

bool FEClientSideNetworkConnection::TryToConnect(std::string ServerIP, unsigned int ServerPort, std::function<void(std::string)> OnDataSentCallback, std::function<void(char*, size_t)> OnDataReceivedCallback)
{
    Socket = new SOCKET;
    *Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in ClientService{};
    ClientService.sin_family = AF_INET;
    ClientService.sin_port = htons(ServerPort);
    inet_pton(AF_INET, ServerIP.c_str(), &ClientService.sin_addr.S_un.S_addr);

    int ConnectResult = connect(*Socket, (SOCKADDR*)&ClientService, sizeof(ClientService));
    if (ConnectResult == SOCKET_ERROR)
    {
        closesocket(*Socket);
        LOG.Add("Connection to a IP: " + ServerIP + " on port: " + std::to_string(ServerPort) + " failed with error : " + std::to_string(WSAGetLastError()), "FE_NETWORKING");
        return false;
    }

    IP = ServerIP;
    Port = ServerPort;
    this->OnDataReceivedCallback = OnDataReceivedCallback;
    this->OnDataSentCallback = OnDataSentCallback;

    // As multiple threads cannot simultaneously read from or write to the same socket,
    // a dedicated thread is created for each socket.
    SendDedicatedThreadID = THREAD_POOL.CreateDedicatedThread();
    ReceiveDedicatedThreadID = THREAD_POOL.CreateDedicatedThread();

    FENetworkReceiveThreadJobInfo* ReceiveJobInfo = new FENetworkReceiveThreadJobInfo;
    ReceiveJobInfo->CurrentSocket = Socket;
    ReceiveJobInfo->OnDataReceivedCallback = OnDataReceivedCallback;
    ReceiveJobInfo->ReceiveDedicatedThreadID = ReceiveDedicatedThreadID;
    ReceiveJobInfo->Caller = (void*)this;
    THREAD_POOL.Execute(ReceiveDedicatedThreadID, ReceiveFunction, (void*)ReceiveJobInfo, (void*)ReceiveJobInfo, AfterReceivingDataFunction);

    return true;
}

void FEClientSideNetworkConnection::SendFunction(void* Input, void* Output)
{
    if (Input == nullptr)
        return;

    FENetworkSendThreadJobInfo* Info = (FENetworkSendThreadJobInfo*)Input;
    if (Info->DataSize == 0 || Info->Data == nullptr)
        return;

    int MessageSize = static_cast<int>(Info->DataSize) + sizeof(size_t) + sizeof(int);
    int MessageType = 1;
    if (MessageType < 0)
        MessageType = 0;
    MessageType += 1; // To ensure that it can not be 0.

    // In order not to reallocate memory for each data transfer,
    // we will send header part before data
    // And since TCP will ensure that data will be received in the same order as sent, this approach is safe.
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

void FEClientSideNetworkConnection::AfterSendOccurredFunction(void* OutputData)
{
    if (OutputData == nullptr)
        return;

    FENetworkSendThreadJobInfo* Info = (FENetworkSendThreadJobInfo*)OutputData;
    if (Info->ErrorCode != FE_NONE)
    {
        reinterpret_cast<FEClientSideNetworkConnection*>(Info->Caller)->OnConnectionError(Info->ErrorCode);
        return;
    }

    if (Info->OnDataSentCallback != nullptr)
        Info->OnDataSentCallback(Info->MessageID);

    delete Info;
}

std::string FEClientSideNetworkConnection::Send(char* Data, size_t DataSize)
{
    FENetworkSendThreadJobInfo* SendJobInfo = new FENetworkSendThreadJobInfo;
    SendJobInfo->CurrentSocket = Socket;
    SendJobInfo->Data = Data;
    SendJobInfo->DataSize = DataSize;
    SendJobInfo->OnDataSentCallback = OnDataSentCallback;
    SendJobInfo->Caller = (void*)this;
    SendJobInfo->MessageID = UNIQUE_ID.GetUniqueHexID();

    THREAD_POOL.Execute(SendDedicatedThreadID, FEClientSideNetworkConnection::SendFunction, (void*)SendJobInfo, (void*)SendJobInfo, FEClientSideNetworkConnection::AfterSendOccurredFunction);
    return SendJobInfo->MessageID;
}

void FEClientSideNetworkConnection::ReceiveFunction(void* Input, void* Output)
{
    if (Input == nullptr)
        return;

    FENetworkReceiveThreadJobInfo* Info = (FENetworkReceiveThreadJobInfo*)Input;

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

void FEClientSideNetworkConnection::AfterReceivingDataFunction(void* OutputData)
{
    FENetworkReceiveThreadJobInfo* Info = (FENetworkReceiveThreadJobInfo*)OutputData;
    if (Info->ErrorCode != FE_NONE)
    {
        reinterpret_cast<FEClientSideNetworkConnection*>(Info->Caller)->OnConnectionError(Info->ErrorCode);
        return;
    }

    while (!Info->Messages.empty())
    {
        auto CurrentMessage = Info->Messages.front();

        if (Info->OnDataReceivedCallback != nullptr)
            Info->OnDataReceivedCallback(CurrentMessage.ReceivedData.data(), CurrentMessage.ReceivedData.size());

        Info->Messages.pop();
    }

    THREAD_POOL.Execute(Info->ReceiveDedicatedThreadID, FEClientSideNetworkConnection::ReceiveFunction, OutputData, OutputData, FEClientSideNetworkConnection::AfterReceivingDataFunction);
}

void FEClientSideNetworkConnection::Clear()
{
    THREAD_POOL.ShutdownDedicatedThread(ReceiveDedicatedThreadID);
    THREAD_POOL.ShutdownDedicatedThread(SendDedicatedThreadID);

    closesocket(*Socket);
}

void FEClientSideNetworkConnection::OnConnectionError(FE_NETWORK_ERROR Error)
{
    Disconnect(Error);
}

void FEClientSideNetworkConnection::SetOnDisconnectCallback(std::function<void(bool)> OnDisconnectCallback)
{
    this->OnDisconnectCallback = OnDisconnectCallback;
}

void FEClientSideNetworkConnection::Disconnect(FE_NETWORK_ERROR Error)
{
    if (Socket != nullptr)
    {
        shutdown(*Socket, SD_SEND);
        if (OnDisconnectCallback != nullptr)
            OnDisconnectCallback(Error == FE_NETWORK_ERROR::FE_GRACEFULLY_CLOSED);
    }

    Clear();
}