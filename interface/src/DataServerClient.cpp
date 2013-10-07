//
//  DataServerClient.cpp
//  hifi
//
//  Created by Stephen Birarda on 10/7/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <NodeList.h>
#include <PacketHeaders.h>
#include <UDPSocket.h>
#include <UUID.h>

#include "DataServerClient.h"

QUuid DataServerClient::_clientUUID;
std::vector<unsigned char*> DataServerClient::_unconfirmedPackets;

const char DATA_SERVER_HOSTNAME[] = "127.0.0.1";
const unsigned short DATA_SERVER_PORT = 3282;
const sockaddr_in DATA_SERVER_SOCKET = socketForHostnameAndHostOrderPort(DATA_SERVER_HOSTNAME, DATA_SERVER_PORT);

void DataServerClient::putValueForKey(const char* key, const char* value) {
    if (!_clientUUID.isNull()) {
        unsigned char* putPacket = new unsigned char[MAX_PACKET_SIZE];
        
        // setup the header for this packet
        int numPacketBytes = populateTypeAndVersion(putPacket, PACKET_TYPE_DATA_SERVER_PUT);
        
        // pack the client UUID
        QString uuidString = uuidStringWithoutCurlyBraces(_clientUUID);
        memcpy(putPacket + numPacketBytes, uuidString.toLocal8Bit().constData(), uuidString.toLocal8Bit().size());
        numPacketBytes += uuidString.toLocal8Bit().size();
        
        // pack the key, null terminated
        strcpy((char*) putPacket + numPacketBytes, key);
        numPacketBytes += strlen(key);
        putPacket[numPacketBytes++] = '\0';
        
        // pack the value, null terminated
        strcpy((char*) putPacket + numPacketBytes, value);
        numPacketBytes += strlen(value);
        putPacket[numPacketBytes++] = '\0';
        
        // add the putPacket to our vector of unconfirmed packets, will be deleted once put is confirmed
        _unconfirmedPackets.push_back(putPacket);
        
        // send this put request to the data server
        NodeList::getInstance()->getNodeSocket()->send((sockaddr*) &DATA_SERVER_SOCKET, putPacket, numPacketBytes);
    }
}

void DataServerClient::getValueForKeyAndUUID(const char* key, QUuid &uuid) {
    if (!uuid.isNull()) {
        unsigned char getPacket[MAX_PACKET_SIZE];
        
        // setup the header for this packet
        int numPacketBytes = populateTypeAndVersion(getPacket, PACKET_TYPE_DATA_SERVER_GET);
        
        // pack the UUID we're asking for data for
        QString uuidString = uuidStringWithoutCurlyBraces(uuid);
        memcpy(getPacket + numPacketBytes, uuidString.toLocal8Bit().constData(), uuidString.toLocal8Bit().size());
        numPacketBytes += uuidString.toLocal8Bit().size();
        
        // pack the key, null terminated
        strcpy((char*) getPacket + numPacketBytes, key);
        numPacketBytes += strlen(key);
        getPacket[numPacketBytes++] = '\0';
        
        // send the get to the data server
        NodeList::getInstance()->getNodeSocket()->send((sockaddr*) &DATA_SERVER_SOCKET, getPacket, numPacketBytes);
    }
}

void DataServerClient::processConfirmFromDataServer(unsigned char* packetData, int numPacketBytes) {
    
    for  (std::vector<unsigned char*>::iterator unconfirmedPacket = _unconfirmedPackets.begin();
          unconfirmedPacket != _unconfirmedPackets.end();
          ++unconfirmedPacket) {
        if (memcmp(*unconfirmedPacket + sizeof(PACKET_TYPE),
                   packetData + sizeof(PACKET_TYPE),
                   numPacketBytes - sizeof(PACKET_TYPE)) == 0) {
            // this is a match - remove the confirmed packet from the vector so it isn't sent back out
            _unconfirmedPackets.erase(unconfirmedPacket);
            
            // we've matched the packet - bail out
            break;
        } else {
            // no match, just push the iterator
            unconfirmedPacket++;
        }
    }
}

void DataServerClient::processGetFromDataServer(unsigned char* packetData, int numPacketBytes) {
    
}
