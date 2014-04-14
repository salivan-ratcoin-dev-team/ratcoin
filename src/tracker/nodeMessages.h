// Copyright (c) 2014 Ratcoin dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NODE_MESSAGES_H
#define NODE_MESSAGES_H

#include "uint256.h"
#include "core.h"

namespace tracker
{

struct CMainRequestType
{
	enum Enum
	{
		  ContinueReq
		, Transaction
		, TransactionStatusReq
		, MonitorInfoReq
		, TrackerInfoReq
		, RequestSatatusReq
	};
};

struct CTransactionMessage
{
	CTransaction m_transaction;
};

struct CTrackerStatsReq
{
	uint256 m_trackerPubKey;
};

struct CMonitorInfoReq
{
	uint256 m_monitorPubKey;
};

struct CTransactionInfoReq
{
	uint256 m_transactionPubKey;
};


}

#endif