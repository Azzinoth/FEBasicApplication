#include "FENetworkingBaseClasses.h"
using namespace FocalEngine;

void NetworkMessage::WorkOnMessage(char* ReceivedData, int BytesReceived, NetworkMessage* CurrentMessage, std::queue<NetworkMessage>* Messages)
{
    BufferReader Reader(ReceivedData, BytesReceived);

    while (Reader.BytesLeft > 0)
    {
        if ((CurrentMessage)->PayloadSize == 0)
        {
            int BytesToRead = sizeof(size_t) > BytesReceived ? BytesReceived : sizeof(size_t);
            if (Reader.BytesLeft != 0 && BytesToRead > Reader.BytesLeft)
                BytesToRead = Reader.BytesLeft;

            int BytesNeededToFill = sizeof(size_t) - static_cast<size_t>(CurrentMessage->PartialPayloadSize.size());
            BytesToRead = BytesToRead > BytesNeededToFill ? BytesNeededToFill : BytesToRead;

            Reader.GetBytes(CurrentMessage->PartialPayloadSize, BytesToRead);

            if (CurrentMessage->PartialPayloadSize.size() == sizeof(size_t))
            {
                CurrentMessage->PayloadSize = *reinterpret_cast<size_t*>(CurrentMessage->PartialPayloadSize.data()) - HEADER_SIZE;
                CurrentMessage->PartialPayloadSize.clear();
            }

            continue;
        }

        if (CurrentMessage->MessageType == -1)
        {
            int BytesToRead = sizeof(int) > Reader.BytesLeft ? Reader.BytesLeft : sizeof(int);
            int BytesNeededToFill = sizeof(int) - static_cast<int>(CurrentMessage->PartialMessageType.size());
            BytesToRead = BytesToRead > BytesNeededToFill ? BytesNeededToFill : BytesToRead;

            Reader.GetBytes(CurrentMessage->PartialMessageType, BytesToRead);

            if (CurrentMessage->PartialMessageType.size() == sizeof(int))
            {
                CurrentMessage->MessageType = *reinterpret_cast<int*>(CurrentMessage->PartialMessageType.data()) - 1;
                CurrentMessage->PartialMessageType.clear();
            }

            continue;
        }

        if (CurrentMessage->PayloadSize > CurrentMessage->ReceivedData.size())
        {
            size_t BytesToRead = CurrentMessage->PayloadSize - CurrentMessage->ReceivedData.size();
            if (Reader.BytesLeft >= BytesToRead)
            {
                Reader.GetBytes(CurrentMessage->ReceivedData, BytesToRead);
                Messages->push(*CurrentMessage);
                *CurrentMessage = NetworkMessage();
            }
            else
            {
                Reader.GetBytes(CurrentMessage->ReceivedData, Reader.BytesLeft);
            }

            continue;
        }
    }
}

BufferReader::BufferReader(char* ReceivedData, size_t BytesReceived)
{
    this->ReceivedData = ReceivedData;
    BytesTotal = BytesReceived;
    BytesLeft = BytesTotal;
}

void BufferReader::GetBytes(std::vector<char>& Out, size_t Size)
{
    if (Size > BytesLeft)
        return;

    size_t StartPosition = Out.size();
    Out.resize(Out.size() + Size);
    memcpy(Out.data() + StartPosition, ReceivedData + CurrentShift, Size);
    CurrentShift += Size;
    BytesRead += Size;
    BytesLeft -= Size;
}