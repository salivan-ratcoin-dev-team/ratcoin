// Copyright (c) 2014 Ratcoin dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef REQUEST_RESPOND_H
#define REQUEST_RESPOND_H

#include <boost/variant.hpp>

#include "uint256.h"
#include "common/transactionStatus.h"

namespace node
{

struct CTransactionStatus
{
	common::TransactionsStatus::Enum m_status;
	uint256 m_token;
};

struct CAccountBalance
{

};

struct CTrackerStats
{
	CTrackerStats( std::string _publicKey = "", unsigned int  _reputation = 0, float _price = 0.0, std::string _ip = "" )
		:m_publicKey( _publicKey ), m_reputation( _reputation ), m_price( _price ), m_ip( _ip ){};
	std::string m_publicKey;
	unsigned int  m_reputation;
	float m_price;
	std::string m_ip;
};

struct CMonitorInfo
{
	std::vector< std::string > m_info;
};

struct CPending
{
	uint256 m_token;
};

typedef boost::variant< CTransactionStatus, CAccountBalance, CTrackerStats, CMonitorInfo, CPending > RequestRespond;

}

#endif