// Copyright (c) 2014-2015 DiMS dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CLIENT_REQUESTS_H
#define CLIENT_REQUESTS_H

#include "common/request.h"
#include "common/responses.h"

#include "appLib/messageType.h"
namespace client
{

struct CErrorForAppPaymentProcessing : public common::CRequest
{
public:
	CErrorForAppPaymentProcessing( dims::CAppError::Enum _error, common::CMediumFilter * _mediumFilter );
	void accept( common::CMedium * _medium ) const;
	int m_error;
};

struct CProofTransactionAndStatusRequest : public common::CRequest
{
	CProofTransactionAndStatusRequest(
			CTransaction const & _trasaction
			, std::vector<unsigned char> const & _transactionStatusSignature
			, CPubKey const & _servicingTracker
			, common::CMonitorData const & _monitorData
			, CPubKey const & _servicingMonitor
			, common::CMediumFilter * _mediumFilter );

public:
	void accept( common::CMedium * _medium ) const;

	common::CPayApplicationData m_payApplicationData;
};

struct CCreateTransactionRequest : public common::CRequest
{
	CCreateTransactionRequest(
			std::vector< std::pair< CKeyID, int64_t > > const & _outputs
			, std::vector< CSpendCoins > const & _sendCoins
			, common::CMediumFilter * _mediumFilter );

public:
	void accept( common::CMedium * _medium ) const;

	std::vector< std::pair< CKeyID, int64_t > > m_outputs;

	std::vector< CSpendCoins > m_sendCoins;
};

}

#endif // CLIENT_REQUESTS_H
