// Copyright (c) 2014-2015 Dims dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PASS_TRANSACTION_ACTION_H
#define PASS_TRANSACTION_ACTION_H

#include "common/scheduleAbleAction.h"

#include "configureTrackerActionHandler.h"

namespace tracker
{

struct CInitial;

class CPassTransactionAction : public common::CScheduleAbleAction< common::CTrackerTypes >, public  boost::statechart::state_machine< CPassTransactionAction, CInitial >
{
public:
	CPassTransactionAction( CKeyID const & _keyId, int64_t _amount );

	virtual void accept( common::CSetResponseVisitor< common::CTrackerTypes > & _visitor );

	CKeyID getKeyId() const{ return m_keyId; }

	int64_t getAmount() const{ return m_amount; }
private:
	CKeyID const m_keyId;

	int64_t m_amount;
};

}

#endif // PASS_TRANSACTION_ACTION_H
