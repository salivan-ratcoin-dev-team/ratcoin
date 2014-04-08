// Copyright (c) 2014 Ratcoin dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include "tracker/validationManager.h"

#include "connectionProvider.h"

#include "requestRespond.h"

namespace node
{

class CMedium;

class CRequestHandler
{
public:
	CRequestHandler( CMedium * _medium );

	RequestRespond getRespond( CRequest* _request ) const;

	bool isProcessed( CRequest* _request ) const;

	bool setRequest( CRequest* _request ) const;

	void runRequests();

	void readLoop();
private:
	std::vector<CRequest*> m_newRequest;
	std::map<CRequest*,uint256> m_pendingRequest;
	std::map<CRequest*,RequestRespond> m_processedRequests;

	CMedium * m_usedMedium;
};


/*
m_processedRequests.find( reqAction.first )
CClientMessageType::Enum
Transaction
, TrackerInfoReq
, MonitorInfoReq
, TransactionInfoReq

*/

}

#endif