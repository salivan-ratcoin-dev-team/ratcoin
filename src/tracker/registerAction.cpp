// Copyright (c) 2014-2015 Dims dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "registerAction.h"
#include "wallet.h"

#include <boost/statechart/state.hpp>
#include <boost/statechart/transition.hpp>
#include <boost/statechart/custom_reaction.hpp>

#include "common/commonRequests.h"
#include "common/setResponseVisitor.h"

#include "tracker/trackerRequests.h"
#include "tracker/selfWallet.h"
#include "tracker/getSelfBalanceAction.h"

extern CWallet* pwalletMain;

namespace tracker
{

/*
get information about registration status
get  conditions of registration

------>> 1 <<-------
-no other trackes
-start operating
-prepare transaction emit it
-send hash to monitor
- conclude
------>> 2 <<-------
- other trackers
- ask for balance ( send  proof of registration if refused )
- prepare  transaction and emit
- send hash to monitor
- conclude
*/



//milisec
unsigned int const WaitTime = 20000;
unsigned int const MoneyWaitTime = 20000;

struct CFreeRegistration;

struct CInitiateRegistration : boost::statechart::state< CInitiateRegistration, CRegisterAction >
{
	CInitiateRegistration( my_context ctx )
		: my_base( ctx )
	{
		LogPrintf("register action: %p initiate registration \n", &context< CRegisterAction >() );
		context< CRegisterAction >().dropRequests();
		context< CRegisterAction >().addRequest(
		 new common::CTimeEventRequest< common::CTrackerTypes >( WaitTime, new CMediumClassFilter( common::CMediumKinds::Time ) ) );

		context< CRegisterAction >().addRequest(
					new CAskForRegistrationRequest(
						context< CRegisterAction >().getActionKey()
						, new CSpecificMediumFilter( context< CRegisterAction >().getNodePtr() ) ) );
	}

	boost::statechart::result react( common::CMessageResult const & _messageResult )
	{
		common::CMessage orginalMessage;
		if ( !common::CommunicationProtocol::unwindMessage( _messageResult.m_message, orginalMessage, GetTime(), _messageResult.m_pubKey ) )
			assert( !"service it somehow" );

		common::CRegistrationTerms connectCondition;

		common::convertPayload( orginalMessage, connectCondition );

		if ( !connectCondition.m_price )
		{
			context< CRegisterAction >().dropRequests();

			context< CRegisterAction >().addRequest(
						new common::CAckRequest< common::CTrackerTypes >(
							  context< CRegisterAction >().getActionKey()
							, connectCondition.m_id
							, new CSpecificMediumFilter( context< CRegisterAction >().getNodePtr() ) ) );

			return transit< CFreeRegistration >();
		}
		else
			//do something later
			return discard_event();
	}

	boost::statechart::result react( common::CTimeEvent const & _timeEvent )
	{
		assert( !"no response" );
		context< CRegisterAction >().dropRequests();
		return discard_event();
	}

	boost::statechart::result react( common::CAckEvent const & _ackEvent )
	{
		return discard_event();
	}

	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CTimeEvent >,
	boost::statechart::custom_reaction< common::CMessageResult >,
	boost::statechart::custom_reaction< common::CAckEvent >
	> reactions;
};

struct CFreeRegistration : boost::statechart::state< CFreeRegistration, CRegisterAction >
{
	CFreeRegistration( my_context ctx )
		: my_base( ctx )
	{
		LogPrintf("register action: %p free registration \n", &context< CRegisterAction >() );

		context< CRegisterAction >().addRequest(
					new common::CTimeEventRequest< common::CTrackerTypes >(
						WaitTime
						, new CMediumClassFilter( common::CMediumKinds::Time ) ) );

		context< CRegisterAction >().addRequest(
					new CRegisterProofRequest(
						context< CRegisterAction >().getActionKey()
						, new CSpecificMediumFilter( context< CRegisterAction >().getNodePtr() ) ) );
	}

	boost::statechart::result react( common::CMessageResult const & _messageResult )
	{
		common::CMessage orginalMessage;
		if ( !common::CommunicationProtocol::unwindMessage( _messageResult.m_message, orginalMessage, GetTime(), _messageResult.m_pubKey ) )
			assert( !"service it somehow" );

		common::CResult result;

		common::convertPayload( orginalMessage, result );

		assert( result.m_result );// for debug only, do something here

		context< CRegisterAction >().addRequest(
					new common::CAckRequest< common::CTrackerTypes >(
						  context< CRegisterAction >().getActionKey()
						, result.m_id
						, new CSpecificMediumFilter( context< CRegisterAction >().getNodePtr() ) ) );
		return discard_event();
	}

	boost::statechart::result react( common::CTimeEvent const & _timeEvent )
	{
		context< CRegisterAction >().dropRequests();
		return discard_event();
	}

	boost::statechart::result react( common::CAckEvent const & _ackEvent )
	{
		return discard_event();
	}

	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CTimeEvent >,
	boost::statechart::custom_reaction< common::CMessageResult >,
	boost::statechart::custom_reaction< common::CAckEvent >
	> reactions;
};

struct CNoTrackers : boost::statechart::state< CNoTrackers, CRegisterAction >
{
	// send  ready  to  monitor      ??????
	CNoTrackers( my_context ctx )
		: my_base( ctx )
	{

		std::vector< std::pair< CKeyID, int64_t > > outputs;

		outputs.push_back(
					std::pair< CKeyID, int64_t >(
						context< CRegisterAction >().getPublicKey().GetID()
						, context< CRegisterAction >().getRegisterPayment() ) );

			CWalletTx tx;
			std::string failReason;

			common::CTrackerStats tracker;
			tracker.m_price = 0; // this  will produce transaction with no tracker output
			if ( pwalletMain->CreateTransaction( outputs, std::vector< CSpendCoins >(), tracker, tx, failReason ) )
			{


			}
			else
			{
				// wait  till  money  available
				context< CRegisterAction >().addRequest(
							new common::CTimeEventRequest< common::CTrackerTypes >(
								MoneyWaitTime
								, new CMediumClassFilter( common::CMediumKinds::Time ) ) );
			}

		//	CTransactionRecordManager::getInstance()->addClientTransaction( _transactionMessage.m_transaction );
		context< CRegisterAction >().addRequest(
					new CRegisterProofRequest(
						context< CRegisterAction >().getActionKey()
						, new CSpecificMediumFilter( context< CRegisterAction >().getNodePtr() ) ) );
	}


	boost::statechart::result react( common::CMessageResult const & _messageResult )
	{
		common::CMessage orginalMessage;
		if ( !common::CommunicationProtocol::unwindMessage( _messageResult.m_message, orginalMessage, GetTime(), _messageResult.m_pubKey ) )
			assert( !"service it somehow" );

		common::CResult result;

		common::convertPayload( orginalMessage, result );

		assert( result.m_result );// for debug only, do something here

		context< CRegisterAction >().addRequest(
					new common::CAckRequest< common::CTrackerTypes >(
						  context< CRegisterAction >().getActionKey()
						, result.m_id
						, new CSpecificMediumFilter( context< CRegisterAction >().getNodePtr() ) ) );
		return discard_event();
	}

	boost::statechart::result react( common::CTimeEvent const & _timeEvent )
	{
		context< CRegisterAction >().dropRequests();
		return discard_event();
	}

	boost::statechart::result react( common::CAckEvent const & _ackEvent )
	{
		return discard_event();
	}

	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CTimeEvent >,
	boost::statechart::custom_reaction< common::CMessageResult >,
	boost::statechart::custom_reaction< common::CAckEvent >
	> reactions;
};


struct CNetworkAlive : boost::statechart::state< CNetworkAlive, CRegisterAction >
{
	// follow ------>> 2 <<-------
	CNetworkAlive( my_context ctx )
		: my_base( ctx )
	{
		context< CRegisterAction >().dropRequests();
		context< CRegisterAction >().addRequest(
					new common::CScheduleActionRequest< common::CTrackerTypes >(
						new CGetSelfBalanceAction()
						, new CMediumClassFilter( common::CMediumKinds::Trackers, 1 ) ) );

	}

	boost::statechart::result react( common::CMessageResult const & _messageResult )
	{
		return discard_event();
	}

	boost::statechart::result react( common::CTimeEvent const & _timeEvent )
	{
		return discard_event();
	}

	boost::statechart::result react( common::CAckEvent const & _ackEvent )
	{
		return discard_event();
	}

	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CTimeEvent >,
	boost::statechart::custom_reaction< common::CMessageResult >,
	boost::statechart::custom_reaction< common::CAckEvent >
	> reactions;
};

CRegisterAction::CRegisterAction( uintptr_t _nodePtr )
	: m_registerObject( getActionKey() )
	, m_nodePtr( _nodePtr )
{
	initiate();
}

CPubKey
CRegisterAction::getPublicKey() const
{
	CAddress address;
	if ( !CTrackerNodesManager::getInstance()->getAddress( m_nodePtr, address ) )
		return CPubKey();

	CPubKey pubKey;
	if ( !CTrackerNodesManager::getInstance()->getPublicKey( address, pubKey ) )
		return CPubKey();

	return pubKey;
}

void
CRegisterAction::accept( common::CSetResponseVisitor< common::CTrackerTypes > & _visitor )
{
	_visitor.visit( *this );
}

}
