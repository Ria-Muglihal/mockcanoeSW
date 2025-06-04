/*
* FMUIPC.cpp
*
*  Created on: Mar 16, 2017
*      Author: idcsim
*/


#include "FMUIPC.h"
#include <string.h>
#include <iostream>
#include <vector>
#include <algorithm>

#ifdef __linux__
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

// #define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)
#ifndef __linux__ 
#pragma warning(disable:4996)
#endif

FMUTCP::FMUTCP()
    :m_sockfd(0)
	, m_portNum(VINC_SENDER_PORT)
    , m_IP_Address(LOCAL_IP_ADDRESS)
    , m_canSend(vINC_SUCCESS)	
{    

}

FMUTCP::~FMUTCP()
{
	CloseCommunication();
}

FMUUDP::FMUUDP()
	:m_sockfd(0)
	, m_portNum(VINC_SENDER_PORT)
	, m_IP_Address(LOCAL_IP_ADDRESS)
	, m_canSend(vINC_SUCCESS)
{
	m_socketAddr = new sockaddr_in();
	memset((char*)m_socketAddr, 0, sizeof(*m_socketAddr));
	m_socketAddr->sin_family = AF_INET;
	m_socketAddr->sin_port = htons(std::stoi(m_portNum.c_str()));
	m_socketAddr->sin_addr.s_addr = inet_addr(m_IP_Address.c_str());
}

FMUUDP::~FMUUDP()
{
	CloseCommunication();
	if (m_socketAddr != nullptr)
	{
		delete m_socketAddr;
	}
}

int  FMUTCP::resolveAddress(std::string& errorMsg)
{
    int bRet = 1;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;
	#ifdef  _WIN32
	ZeroMemory(&hints, sizeof(hints));
	#elif __linux__
		memset(&hints, 0,sizeof(hints));
	#endif
	hints.ai_family = PF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	int iResult = getaddrinfo(m_IP_Address.c_str(), m_portNum.c_str(), &hints, &result);
	if (iResult != 0) {
		//printf("getaddrinfo failed with error: %d\n", iResult);
		//errorMsg = "getaddrinfo failed with error.";
		GetErrorMsgDescription(errorMsg);
#ifdef  _WIN32
		WSACleanup();
#elif __linux__
  close(m_sockfd);
#endif
		return 1;
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL;ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		m_sockfd = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (m_sockfd == INVALID_SOCKET) {
			//printf("socket failed with error: %ld\n", WSAGetLastError());
			//errorMsg = "socket failed with error.";
			GetErrorMsgDescription(errorMsg);
			freeaddrinfo(result);
#ifdef  _WIN32
			WSACleanup();
#elif __linux__
            close(m_sockfd);
#endif
			return 1;
		}
        
        iResult = connect(m_sockfd, ptr->ai_addr, (int)ptr->ai_addrlen);
        if ( iResult == -1 ) {
#ifdef  _WIN32
            closesocket( m_sockfd );
#else
            close( m_sockfd );
#endif
            m_sockfd = -1;
            continue;
        }
        bRet = 0;
        break;
	}	
    freeaddrinfo(result);
	return bRet;
}

int  FMUUDP::resolveAddress(std::string& errorMsg)
{
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;
#ifdef  _WIN32
	ZeroMemory(&hints, sizeof(hints));
#elif __linux__
	memset(&hints, 0, sizeof(hints));
#endif
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;

	// Resolve the server address and port
	int iResult = getaddrinfo(m_IP_Address.c_str(), m_portNum.c_str(), &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		//errorMsg = "getaddrinfo failed with error.";
		GetErrorMsgDescription(errorMsg);
#ifdef  _WIN32
		WSACleanup();
#elif __linux__
		close(m_sockfd);
#endif
		return 1;
	}


	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		m_sockfd = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (m_sockfd == INVALID_SOCKET) {
			printf("socket failed with error: %d\n", IPC_ERROR);
			//errorMsg = "socket failed with error.";
			GetErrorMsgDescription(errorMsg);
			freeaddrinfo(result);
#ifdef  _WIN32
			WSACleanup();
#elif __linux__
			close(m_sockfd);
#endif
			return 1;
		}
//		struct timeval tv;
//		tv.tv_sec = 20; /* seconds */
//		tv.tv_usec = 0;
//
//		if (setsockopt(m_sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0)
//			std::cout << "Cannot Set SO_SNDTIMEO for socket" << std::endl;
//
//		if (setsockopt(m_sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0)
//			std::cout << "Cannot Set SO_RCVTIMEO for socket\n" << std::endl;
//
//BOOL bNewBehavior = FALSE;
//DWORD dwBytesReturned = 0;
//WSAIoctl(m_sockfd, SIO_UDP_CONNRESET, &bNewBehavior, sizeof bNewBehavior, NULL, 0, &dwBytesReturned, NULL, NULL);

		int bindResult = 0;
		do
		{
			bindResult = connect(m_sockfd, ptr->ai_addr, (int)ptr->ai_addrlen);
			printf("socket connect result : %d\n", bindResult);
#ifdef  _WIN32
			Sleep(1000);
#elif __linux__
			sleep(1);
#endif
		} while (bindResult <= SOCKET_ERROR);
		break;
	}
	freeaddrinfo(result);
	return 0;
}

IPC_RETURN_TYPE  FMUTCP::InitCommunication(const std::string& strPortORFileName, const std::string& strIP_Address)
{
	// Nothing here for TCP/IP communication
	m_portNum = strPortORFileName;
	m_IP_Address = strIP_Address;
	return IPC_RETURN_SUCCESS;
}

IPC_RETURN_TYPE  FMUUDP::InitCommunication(const std::string& strPortORFileName, const std::string& strIP_Address)
{
	// Nothing here for TCP/IP communication
	m_portNum = strPortORFileName;
	m_IP_Address = strIP_Address;

	if (m_socketAddr != nullptr)
	{
		delete m_socketAddr;
	}

	m_socketAddr = new sockaddr_in();
	memset((char*)m_socketAddr, 0, sizeof(*m_socketAddr));
	m_socketAddr->sin_family = AF_INET;
	m_socketAddr->sin_port = htons(std::stoi(m_portNum.c_str()));
	m_socketAddr->sin_addr.s_addr = inet_addr(m_IP_Address.c_str());
	return IPC_RETURN_SUCCESS;
}

IPC_RETURN_TYPE FMUTCP::OpenCommunication()
{
#ifdef  _WIN32
	WSADATA w;
	m_canSend = WSAStartup(0x0101, &w);

	if (m_canSend != 0) {
		m_errorDescription = "WSAStartup failed with error.";
		return IPC_RETURN_ERROR;
	}
#endif
	#ifdef __linux__
	  sleep(1);
	#endif
    
	while (1 == resolveAddress(m_errorDescription))
	{
#ifdef __linux__
        sleep(1);
#endif
	}
	if (m_sockfd == INVALID_SOCKET) {
		//printf("Unable to connect to server!\n");
		//errorMsg = "WSAStartup failed with error.";
#ifdef  _WIN32
		WSACleanup();
#elif __linux__
  close(m_sockfd);
#endif
		return IPC_RETURN_ERROR;
	}

	m_errorDescription.clear();
	return IPC_RETURN_SUCCESS;	
}

IPC_RETURN_TYPE FMUUDP::OpenCommunication()
{
#ifdef  _WIN32
	WSADATA w;
	m_canSend = WSAStartup(0x0101, &w);

	if (m_canSend != 0) {
		m_errorDescription = "WSAStartup failed with error.";
		return IPC_RETURN_ERROR;
	}
#endif
#ifdef __linux__
	sleep(1);
#endif

	if (1 == resolveAddress(m_errorDescription))
	{
		return IPC_RETURN_ERROR;
	}
	if (m_sockfd == INVALID_SOCKET) {
		//printf("Unable to connect to server!\n");
		//errorMsg = "WSAStartup failed with error.";
#ifdef  _WIN32
		WSACleanup();
#elif __linux__
		close(m_sockfd);
#endif
		return IPC_RETURN_ERROR;
	}

	m_errorDescription.clear();
	return IPC_RETURN_SUCCESS;
}

IPC_RETURN_TYPE FMUTCP::WriteData(void* pMemData, int iDataSize)
{
	if (SOCKET_ERROR == WriteToSocket(pMemData, iDataSize, m_errorDescription))
	{
		return IPC_RETURN_ERROR;
	}

	return IPC_RETURN_SUCCESS;
}
IPC_RETURN_TYPE FMUUDP::WriteData(void* pMemData, int iDataSize)
{
	if (SOCKET_ERROR == WriteToSocket(pMemData, iDataSize, m_errorDescription))
	{
		return IPC_RETURN_ERROR;
	}

	return IPC_RETURN_SUCCESS;
}

IPC_RETURN_TYPE FMUTCP::ReadStatus( int* recv_value )
{
    IPC_RETURN_TYPE retValue = IPC_RETURN_ERROR;
    if ( m_sockfd == INVALID_SOCKET ) {
        printf( "FMU socket invalid\n" );
        return retValue;
    }

    int buffer;
    int bytecount = 0;

    // Peek into the socket and get the packet size
    if ( ( bytecount = recv( m_sockfd, (char*)&buffer, 4, 0 ) ) == -1 )
    {
        fprintf( stderr, "Error: Receiving data %d\n", errno );
        return retValue;
    }
    else if ( bytecount == 0 )
        printf( "Connection closed\n" );
	*recv_value = buffer;
    return IPC_RETURN_SUCCESS;
}

IPC_RETURN_TYPE FMUUDP::ReadStatus(int* recv_value)
{
	IPC_RETURN_TYPE retValue = IPC_RETURN_ERROR;
	if (m_sockfd == INVALID_SOCKET) {
		printf("FMU socket invalid\n");
		return retValue;
	}

	int buffer;
	int bytecount = 0;

	int addr_length = sizeof(sockaddr_in);
	// Peek into the socket and get the packet size
	if ((bytecount = recvfrom(m_sockfd, (char*)&buffer, 4, 0, (struct sockaddr*)m_socketAddr, (socklen_t*)&addr_length)) == -1)
	{
		fprintf(stderr, "Error: Receiving data %d\n", errno);
		return retValue;
	}
	else if (bytecount == 0)
		printf("Connection closed\n");
	*recv_value = buffer;
	return IPC_RETURN_SUCCESS;
}

IPC_RETURN_TYPE FMUTCP::ReadData(void* pMemData, int iDataSize,int* readDataSize)
{
	int iResult = 0;
	m_errorDescription.clear();
	do {
		iResult = receivefull((char*)pMemData, iDataSize, 0);
		if (iResult > 0)
		{
            *readDataSize = iResult;
			return IPC_RETURN_SUCCESS;
		}
		//printf("Bytes received: %d\n", iResult);
		else if (iResult == 0)
			printf("Connection closed\n");
		else
		{
#ifdef  _WIN32
			printf("recv failed with error: %d\n", WSAGetLastError());
#elif __linux__
            printf("recv failed with error: %d\n", errno);
#endif
			GetErrorMsgDescription(m_errorDescription);
			printf("recv failed with error: %s\n", m_errorDescription.c_str());
		}

	} while (iResult > 0);
	return IPC_RETURN_ERROR;
}

std::string FMUTCP::getErrorDescription()
{
	return m_errorDescription;
}
std::string FMUUDP::getErrorDescription()
{
	return m_errorDescription;
}
void FMUTCP::GetErrorMsgDescription(std::string& errorMsg)
{
#ifdef  _WIN32
    DWORD errorMessageID = GetLastError();
    LPSTR messageBuffer = nullptr;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

    errorMsg = errorMsg.append("Socket Error: ").append(messageBuffer);

    //Free the buffer.
    LocalFree(messageBuffer);
#elif __linux__
errorMsg = errorMsg.append("Socket Error: ").append(strerror(errno));
#endif
}
void FMUUDP::GetErrorMsgDescription(std::string& errorMsg)
{
#ifdef  _WIN32
	DWORD errorMessageID = GetLastError();
	LPSTR messageBuffer = nullptr;
	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

	errorMsg = errorMsg.append("Socket Error: ").append(messageBuffer);

	//Free the buffer.
	LocalFree(messageBuffer);
#elif __linux__
	errorMsg = errorMsg.append("Socket Error: ").append(strerror(errno));
#endif
}

int FMUTCP::WriteToSocket(void* buffer, int iDataSize, std::string& errorMsg)
{
	if (m_canSend)
		return SOCKET_ERROR;
	
	// Check whether the connection available.
	int iResult = send(m_sockfd, (const char*)buffer, iDataSize /*(int)strlen(sendbuf)*/, 0);
	if (iResult == SOCKET_ERROR) {
		//printf("send failed with error: %d\n", WSAGetLastError());
		GetErrorMsgDescription(errorMsg);
#ifdef _WIN32
		closesocket(m_sockfd);
		WSACleanup();
#elif __linux__
        close(m_sockfd);
#endif
        return SOCKET_ERROR;
	}

	return iResult;
}

int FMUUDP::WriteToSocket(void* buffer, int iDataSize, std::string& errorMsg)
{
	if (m_canSend)
		return SOCKET_ERROR;

	
	int addr_length = sizeof(sockaddr_in);

	// Check whether the connection available.
	int iResult = sendto(m_sockfd, (const char*)buffer, iDataSize, 0, (struct sockaddr*)m_socketAddr, addr_length);
	if (iResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", IPC_ERROR);
		GetErrorMsgDescription(errorMsg);
		printf("send failed with error: %s\n", errorMsg.c_str());
#ifdef _WIN32
		closesocket(m_sockfd);
		WSACleanup();
#elif __linux__
		close(m_sockfd);
#endif
		return SOCKET_ERROR;
	}

	return iResult;
}

int FMUTCP::receivefull(void *buffer, size_t len, int flags)
{
	size_t yetToRead = len;
	char  *bufPointer = (char*)buffer;

	while (yetToRead > 0)
	{
		int received = recv(m_sockfd, bufPointer, static_cast<int>(yetToRead), flags);
		if (received <= 0)
			return received;     // We have error or the caller closed the connection

		yetToRead -= received;   // Read remaining
		bufPointer += received;  // Pointing to next buffer poistion
	}

	return static_cast<int>(len);
}

void FMUTCP::CloseCommunication()
{
	if (m_sockfd != INVALID_SOCKET)
	{
#ifdef _WIN32
		closesocket(m_sockfd);
		WSACleanup();
		Sleep(2000);
#elif __linux__
		int how = SHUT_RDWR;
		shutdown(m_sockfd,how);
		close(m_sockfd);
#endif
		m_sockfd = INVALID_SOCKET;

	}

}

void FMUUDP::CloseCommunication()
{
	if (m_sockfd != INVALID_SOCKET)
	{
#ifdef _WIN32
		closesocket(m_sockfd);
		WSACleanup();
		Sleep(2000);
#elif __linux__
		close(m_sockfd);
#endif
		m_sockfd = INVALID_SOCKET;

	}

}
