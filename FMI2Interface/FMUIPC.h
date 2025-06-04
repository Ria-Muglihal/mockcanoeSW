/*
* FMUIPC.h
*
*  Created on: Mar 16, 2017
*      Author: idcsim
*/
#pragma once


#ifdef _WIN32
#include <Ws2tcpip.h>
#define SOCKET SOCKET
#elif __linux__
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>
#include <errno.h>
#define SOCKET int
#endif
#include "Defines.h"

#ifdef _WIN32
#define IPC_ERROR WSAGetLastError()
#elif __linux__
#define IPC_ERROR errno
#endif

enum vINC_WARNINGS_ERRORS
{
	vINC_FAIL = -1,
	vINC_SUCCESS,
	vINC_XML_PARSING_ERROR,
	vINC_SENDER_ERROR,
	vINC_RECEIVER_ERROR,
	vINC_NTWRK_ERROR,
	vINC_SOCKET_ERROR,
	vINC_THREAD_ERROR
};
#ifndef FMUIPC_H_
#define FMUIPC_H_
#include <memory>
#include <vector>
#include <map>
#include "BaseIPC.h"

class FMUTCP :  public IBaseIPC {
public:
	FMUTCP();
	virtual ~FMUTCP();
	virtual IPC_RETURN_TYPE InitCommunication(const std::string& strPortORFileName, const std::string& strIP_Address) override;
	virtual IPC_RETURN_TYPE OpenCommunication() override;
	virtual IPC_RETURN_TYPE WriteData(void* pMemData, int iDataSize) override;
	virtual IPC_RETURN_TYPE ReadData(void* pMemData, int iDataSize,int* readDataSize) override;
    IPC_RETURN_TYPE ReadStatus(int* recv_value ) override;
	virtual std::string getErrorDescription() override;
	virtual void CloseCommunication() override;
	 
	
protected:
	SOCKET         m_sockfd;
	std::string m_errorDescription;
	int receivefull(void *buffer, size_t len, int flags);
	static void GetErrorMsgDescription(std::string& errorMsg);
private:
    std::string m_portNum;
    std::string m_IP_Address;
	int m_canSend;    
	int  resolveAddress(std::string& errorMsg);
	int WriteToSocket(void* buffer, int iDataSize, std::string& errorMsg);

protected:	
};

class FMUUDP : public IBaseIPC {
public:
	FMUUDP();
	virtual ~FMUUDP();
	virtual IPC_RETURN_TYPE InitCommunication(const std::string& strPortORFileName, const std::string& strIP_Address) override;
	virtual IPC_RETURN_TYPE OpenCommunication() override;
	virtual IPC_RETURN_TYPE WriteData(void* pMemData, int iDataSize) override;
	virtual IPC_RETURN_TYPE ReadData(void* pMemData, int iDataSize, int* readDataSize) override = 0;
	IPC_RETURN_TYPE ReadStatus(int* recv_value ) override;
	virtual std::string getErrorDescription() override;
	virtual void CloseCommunication() override;

protected:
	SOCKET         m_sockfd;
	std::string m_errorDescription;
	static void GetErrorMsgDescription(std::string& errorMsg);
	inline sockaddr_in*  getSocketAddr()
	{
		return this->m_socketAddr;
	};

private:
	std::string m_portNum;
	std::string m_IP_Address;
	sockaddr_in * m_socketAddr;
	int m_canSend;
	int  resolveAddress(std::string& errorMsg);
	int WriteToSocket(void* buffer, int iDataSize, std::string& errorMsg);

protected:
};

#endif /* FMUIPC_H_ */
