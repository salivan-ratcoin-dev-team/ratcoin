// Copyright (c) 2014-2015 DiMS dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/statechart/simple_state.hpp>
#include <boost/statechart/state.hpp>
#include <boost/statechart/transition.hpp>
#include <boost/statechart/custom_reaction.hpp>

#include "wallet.h"
#include "main.h"

#include "common/setResponseVisitor.h"
#include "common/requests.h"
#include "common/events.h"
#include "common/segmentFileStorage.h"
#include "common/mediumKinds.h"
#include "common/supportTransactionsDatabase.h"
#include "common/events.h"
#include "common/actionHandler.h"
#include "common/analyseTransaction.h"
#include "common/authenticationProvider.h"

#include "tracker/transactionRecordManager.h"
#include "tracker/synchronizationAction.h"
#include "tracker/requests.h"
#include "tracker/filters.h"
#include "tracker/events.h"
#include "tracker/controller.h"
#include "tracker/controllerEvents.h"
#include "tracker/trackOriginAddressAction.h"

namespace tracker
{
// fix  synchronized
unsigned const SynchronisingGetInfoTime = 10000;//milisec

unsigned const SynchronisingWaitTime = 15000;

struct CSynchronizingGetInfo;
struct CSynchronizedGetInfo;
struct CSynchronizingRegistrationAsk;
struct CGetBitcoinHeader;

struct CUninitiated : boost::statechart::simple_state< CUninitiated, CSynchronizationAction >
{
	typedef boost::mpl::list<
	boost::statechart::transition< CSwitchToSynchronizing, CSynchronizingRegistrationAsk >,
	boost::statechart::transition< CSwitchToSynchronized, CSynchronizedGetInfo >
	> reactions;
};

struct CSynchronizingHeaders;

struct CSynchronizingRegistrationAsk : boost::statechart::state< CSynchronizingRegistrationAsk, CSynchronizationAction >
{
	CSynchronizingRegistrationAsk( my_context ctx ) : my_base( ctx )
	{
		LogPrintf("synchronize action: %p ask \n", &context< CSynchronizationAction >() );

		context< CSynchronizationAction >().forgetRequests();

		context< CSynchronizationAction >().addRequest(
				new common::CSendMessageRequest(
					common::CPayloadKind::SynchronizationAsk
					, common::CSynchronizationAsk()
					, context< CSynchronizationAction >().getActionKey()
					, new CByKeyMediumFilter( context< CSynchronizationAction >().getPartnerKey() ) ) );

		context< CSynchronizationAction >().addRequest(
					new common::CTimeEventRequest(
						 SynchronisingGetInfoTime
						, new CMediumClassFilter( common::CMediumKinds::Time ) ) );
	}

	boost::statechart::result react( common::CTimeEvent const & _timeEvent )
	{
		context< CSynchronizationAction >().forgetRequests();
		context< CSynchronizationAction >().setExit();
		return discard_event();
	}

	boost::statechart::result react( common::CAckEvent const & _promptAck )
	{
		return discard_event();
	}

	boost::statechart::result react( common::CMessageResult const & _messageResult )
		{
			common::CMessage orginalMessage;
			if ( !common::CommunicationProtocol::unwindMessage( _messageResult.m_message, orginalMessage, GetTime(), _messageResult.m_pubKey) )
				assert( !"service it somehow" );

			if ( orginalMessage.m_header.m_payloadKind == common::CPayloadKind::Result )
			{
				common::CResult result;

				common::convertPayload( orginalMessage, result );

				context< CSynchronizationAction >().forgetRequests();

				context< CSynchronizationAction >().addRequest(
							new common::CAckRequest(
								  context< CSynchronizationAction >().getActionKey()
								, _messageResult.m_message.m_header.m_id
								, new CByKeyMediumFilter( context< CSynchronizationAction >().getPartnerKey() ) ) );

				if ( result.m_result )
				{
					CTransactionRecordManager::getInstance()->clearCoinViewDB();
					CTransactionRecordManager::getInstance()->clearAddressToCoinsDatabase();
					CTransactionRecordManager::getInstance()->clearSupportTransactionsDatabase();

					CWallet::getInstance()->resetDatabase();

					CWallet::getInstance()->AddKeyPubKey(
								common::CAuthenticationProvider::getInstance()->getMyPrivKey()
								, common::CAuthenticationProvider::getInstance()->getMyKey());

					common::CSegmentFileStorage::getInstance()->setSynchronizationInProgress();

					return transit< CGetBitcoinHeader >();
				}
				context< CSynchronizationAction >().setResult( common::CSynchronizationResult( 0 ) );
				context< CSynchronizationAction >().setExit();
			}
			return discard_event();
	}

	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CTimeEvent >,
	boost::statechart::custom_reaction< common::CMessageResult >,
	boost::statechart::custom_reaction< common::CAckEvent >
	> reactions;
};

struct CGetBitcoinHeader: boost::statechart::state< CGetBitcoinHeader, CSynchronizationAction >
{
	CGetBitcoinHeader( my_context ctx ) : my_base( ctx )
	{
		LogPrintf("synchronize action: %p get bitcoin  header \n", &context< CSynchronizationAction >() );
		context< CSynchronizationAction >().forgetRequests();

		context< CSynchronizationAction >().addRequest(
				new common::CSendMessageRequest(
					common::CPayloadKind::InfoReq
					, common::CInfoRequestData( (int)common::CInfoKind::BitcoinHeaderAsk, std::vector<unsigned char>() )
					, context< CSynchronizationAction >().getActionKey()
					, new CByKeyMediumFilter( context< CSynchronizationAction >().getPartnerKey() ) ) );

		context< CSynchronizationAction >().addRequest(
					new common::CTimeEventRequest(
						 SynchronisingGetInfoTime
						, new CMediumClassFilter( common::CMediumKinds::Time ) ) );
	}


	boost::statechart::result react( common::CMessageResult const & _messageResult )
	{
		common::CMessage orginalMessage;
		if ( !common::CommunicationProtocol::unwindMessage( _messageResult.m_message, orginalMessage, GetTime(), _messageResult.m_pubKey) )
			assert( !"service it somehow" );

		if ( orginalMessage.m_header.m_payloadKind == common::CPayloadKind::SynchronizationBitcoinHeader )
		{
			common::CBitcoinHeader bitcoinHeader;

			common::convertPayload( orginalMessage, bitcoinHeader );

			CAutoFile file( OpenHeadFile(false), SER_DISK, CLIENT_VERSION );

			file << bitcoinHeader.m_bitcoinHeader;
			fflush(file);
			FileCommit(file);

			resetChains();
			context< CSynchronizationAction >().addRequest(
						new common::CAckRequest(
							  context< CSynchronizationAction >().getActionKey()
							, _messageResult.m_message.m_header.m_id
							, new CByKeyMediumFilter( context< CSynchronizationAction >().getPartnerKey() ) ) );

			common::CActionHandler::getInstance()->executeAction( tracker::CTrackOriginAddressAction::createInstance() );

		}
		return transit< CSynchronizingGetInfo >();
	}

	boost::statechart::result react( common::CTimeEvent const & _timeEvent )
	{
		context< CSynchronizationAction >().forgetRequests();
		context< CSynchronizationAction >().setResult( common::CSynchronizationResult( 0 ) );
		context< CSynchronizationAction >().setExit();
		return discard_event();
	}

	boost::statechart::result react( common::CAckEvent const & _promptAck )
	{
		return discard_event();
	}

	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CTimeEvent >,
	boost::statechart::custom_reaction< common::CMessageResult >,
	boost::statechart::custom_reaction< common::CAckEvent >
	> reactions;
};

struct CSynchronizingGetInfo : boost::statechart::state< CSynchronizingGetInfo, CSynchronizationAction >
{
	CSynchronizingGetInfo( my_context ctx ) : my_base( ctx )
	{
		LogPrintf("synchronize action: %p get info \n", &context< CSynchronizationAction >() );

		context< CSynchronizationAction >().forgetRequests();

		context< CSynchronizationAction >().addRequest(
				new common::CSendMessageRequest(
					common::CPayloadKind::InfoReq
					, common::CInfoRequestData( (int)common::CInfoKind::StorageInfoAsk, 0 )
					, context< CSynchronizationAction >().getActionKey()
					, new CByKeyMediumFilter( context< CSynchronizationAction >().getPartnerKey() ) ) );
	}

	boost::statechart::result react( common::CMessageResult const & _messageResult )
	{
		common::CMessage orginalMessage;
		if ( !common::CommunicationProtocol::unwindMessage( _messageResult.m_message, orginalMessage, GetTime(), _messageResult.m_pubKey) )
			assert( !"service it somehow" );

		if ( orginalMessage.m_header.m_payloadKind == common::CPayloadKind::SynchronizationInfo )
		{
			common::CSynchronizationInfo synchronizationInfo;

			common::convertPayload( orginalMessage, synchronizationInfo );

			context< CSynchronizationAction >().setStorageSize( synchronizationInfo.m_strageSize );
			context< CSynchronizationAction >().setHeaderSize( synchronizationInfo.m_headerSize );

			context< CSynchronizationAction >().addRequest(
						new common::CAckRequest(
							  context< CSynchronizationAction >().getActionKey()
							, _messageResult.m_message.m_header.m_id
							, new CByKeyMediumFilter( context< CSynchronizationAction >().getPartnerKey() ) ) );

		}
		return transit< CSynchronizingHeaders >();
	}

	boost::statechart::result react( common::CAckEvent const & _promptAck )
	{
		return discard_event();
	}

	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CMessageResult >,
	boost::statechart::custom_reaction< common::CAckEvent >
	> reactions;
};

struct CSynchronizingBlocks : boost::statechart::state< CSynchronizingBlocks, CSynchronizationAction >
{
	CSynchronizingBlocks( my_context ctx ) : my_base( ctx ), m_currentBlock( 0 )
	{
		LogPrintf("synchronize action: %p blocks \n", &context< CSynchronizationAction >() );

		context< CSynchronizationAction >().forgetRequests();

		common::CSynchronizationGet synchronizationGet(
			(int)common::CBlockKind::Segment
			, m_currentBlock );

		context< CSynchronizationAction >().addRequest(
				new common::CSendMessageRequest(
					common::CPayloadKind::SynchronizationGet
					, synchronizationGet
					, context< CSynchronizationAction >().getActionKey()
					, new CByKeyMediumFilter( context< CSynchronizationAction >().getPartnerKey() ) ) );

		context< CSynchronizationAction >().addRequest(
					new common::CTimeEventRequest(
						  SynchronisingWaitTime
						, new CMediumClassFilter( common::CMediumKinds::Time ) ) );
	}

	boost::statechart::result react( common::CMessageResult const & _messageResult )
	{
		common::CMessage orginalMessage;
		if ( !common::CommunicationProtocol::unwindMessage( _messageResult.m_message, orginalMessage, GetTime(), _messageResult.m_pubKey) )
			assert( !"service it somehow" );

		if ( orginalMessage.m_header.m_payloadKind == common::CPayloadKind::SynchronizationBlock )
		{
			common::CSynchronizationBlock synchronizationBlock( new common::CDiskBlock(), -1 );

			common::convertPayload( orginalMessage, synchronizationBlock );
			context< CSynchronizationAction >().forgetRequests();

			context< CSynchronizationAction >().addRequest(
						new common::CAckRequest(
							  context< CSynchronizationAction >().getActionKey()
							, _messageResult.m_message.m_header.m_id
							, new CByKeyMediumFilter( context< CSynchronizationAction >().getPartnerKey() ) ) );

			std::vector< CTransaction > transactions;

			assert( synchronizationBlock.m_blockIndex == m_currentBlock );

			common::CSegmentFileStorage::getInstance()->setDiscBlock( *synchronizationBlock.m_diskBlock, synchronizationBlock.m_blockIndex, transactions );

			BOOST_FOREACH( CTransaction const & transaction, transactions )
			{
				common::findSelfCoinsAndAddToWallet( transaction );
				common::CSupportTransactionsDatabase::getInstance()->setTransactionLocation( transaction.GetHash(), transaction.m_location );
			}

			common::CSupportTransactionsDatabase::getInstance()->flush();

			CTransactionRecordManager::getInstance()->addRetrivedTransactionBundle( transactions );

			if ( context< CSynchronizationAction >().getStorageSize() > ++m_currentBlock )
			{
				common::CSynchronizationGet synchronizationGet(
					(int)common::CBlockKind::Segment
					, m_currentBlock );

				context< CSynchronizationAction >().addRequest(
						new common::CSendMessageRequest(
							common::CPayloadKind::SynchronizationGet
							, synchronizationGet
							, context< CSynchronizationAction >().getActionKey()
							, new CByKeyMediumFilter( context< CSynchronizationAction >().getPartnerKey() ) ) );

			}
			else
			{
				CController::getInstance()->process_event( CSynchronizedWithNetworkEvent() );
				common::CSegmentFileStorage::getInstance()->resetState();
				common::CSegmentFileStorage::getInstance()->retriveState();

				context< CSynchronizationAction >().setResult( common::CSynchronizationResult(1) );
				context< CSynchronizationAction >().setExit();
			}
		}
		return discard_event();
	}

	boost::statechart::result react( common::CTimeEvent const & _timeEvent )
	{
		context< CSynchronizationAction >().forgetRequests();

		common::CSynchronizationGet synchronizationGet(
			(int)common::CBlockKind::Segment
			, m_currentBlock );

		context< CSynchronizationAction >().addRequest(
				new common::CSendMessageRequest(
					common::CPayloadKind::SynchronizationGet
					, synchronizationGet
					, context< CSynchronizationAction >().getActionKey()
					, new CByKeyMediumFilter( context< CSynchronizationAction >().getPartnerKey() ) ) );

		context< CSynchronizationAction >().addRequest(
					new common::CTimeEventRequest(
						  SynchronisingWaitTime
						, new CMediumClassFilter( common::CMediumKinds::Time ) ) );

		return discard_event();
	}

	boost::statechart::result react( common::CAckEvent const & _promptAck )
	{
		return discard_event();
	}

	~CSynchronizingBlocks()
	{
		common::CSegmentFileStorage::getInstance()->releaseSynchronizationInProgress();
	}

	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CTimeEvent >,
	boost::statechart::custom_reaction< common::CAckEvent >,
	boost::statechart::custom_reaction< common::CMessageResult >
	> reactions;

	unsigned int m_currentBlock;
};


struct CSynchronizingHeaders : boost::statechart::state< CSynchronizingHeaders, CSynchronizationAction >
{
	CSynchronizingHeaders( my_context ctx ) : my_base( ctx ), m_currentBlock( 0 )
	{
		LogPrintf("synchronize action: %p headers \n", &context< CSynchronizationAction >() );

		context< CSynchronizationAction >().forgetRequests();

		common::CSynchronizationGet synchronizationGet(
			(int)common::CBlockKind::Header
			, m_currentBlock );

		context< CSynchronizationAction >().addRequest(
				new common::CSendMessageRequest(
					common::CPayloadKind::SynchronizationGet
					, synchronizationGet
					, context< CSynchronizationAction >().getActionKey()
					, new CByKeyMediumFilter( context< CSynchronizationAction >().getPartnerKey() ) ) );

		context< CSynchronizationAction >().addRequest(
					new common::CTimeEventRequest(
						  SynchronisingWaitTime
						, new CMediumClassFilter( common::CMediumKinds::Time ) ) );
	}

	boost::statechart::result react( common::CMessageResult const & _messageResult )
	{
		common::CMessage orginalMessage;
		if ( !common::CommunicationProtocol::unwindMessage( _messageResult.m_message, orginalMessage, GetTime(), _messageResult.m_pubKey) )
			assert( !"service it somehow" );

		if ( orginalMessage.m_header.m_payloadKind == common::CPayloadKind::SynchronizationHeader )
		{
			common::CSynchronizationSegmentHeader synchronizationHeader( new common::CSegmentHeader(), -1 );

			common::convertPayload( orginalMessage, synchronizationHeader );
			context< CSynchronizationAction >().forgetRequests();

			common::CSegmentFileStorage::getInstance()->setDiscBlock( *synchronizationHeader.m_segmentHeader, synchronizationHeader.m_blockIndex );

			context< CSynchronizationAction >().addRequest(
						new common::CAckRequest(
							  context< CSynchronizationAction >().getActionKey()
							, _messageResult.m_message.m_header.m_id
							, new CByKeyMediumFilter( context< CSynchronizationAction >().getPartnerKey() ) ) );

			if ( context< CSynchronizationAction >().getHeaderSize() > ++m_currentBlock )
			{
				common::CSynchronizationGet synchronizationGet(
					(int)common::CBlockKind::Header
					, m_currentBlock );

				context< CSynchronizationAction >().addRequest(
						new common::CSendMessageRequest(
							common::CPayloadKind::SynchronizationGet
							, synchronizationGet
							, context< CSynchronizationAction >().getActionKey()
							, new CByKeyMediumFilter( context< CSynchronizationAction >().getPartnerKey() ) ) );
			}
			else
			{
				return transit< CSynchronizingBlocks >();
			}

			assert( synchronizationHeader.m_blockIndex == m_currentBlock - 1 );
		}
		return discard_event();
	}

	boost::statechart::result react( common::CTimeEvent const & _timeEvent )
	{
		context< CSynchronizationAction >().forgetRequests();

		common::CSynchronizationGet synchronizationGet(
			(int)common::CBlockKind::Header
			, m_currentBlock );

		context< CSynchronizationAction >().addRequest(
				new common::CSendMessageRequest(
					common::CPayloadKind::SynchronizationGet
					, synchronizationGet
					, context< CSynchronizationAction >().getActionKey()
					, new CByKeyMediumFilter( context< CSynchronizationAction >().getPartnerKey() ) ) );

		context< CSynchronizationAction >().addRequest(
					new common::CTimeEventRequest(
						  SynchronisingWaitTime
						, new CMediumClassFilter( common::CMediumKinds::Time ) ) );

		return discard_event();
	}

	boost::statechart::result react( common::CAckEvent const & _promptAck )
	{
		return discard_event();
	}

	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CTimeEvent >,
	boost::statechart::custom_reaction< common::CAckEvent >,
	boost::statechart::custom_reaction< common::CMessageResult >
	> reactions;

	unsigned int m_currentBlock;
};

struct CSynchronized;

struct CSynchronizedGetInfo : boost::statechart::state< CSynchronizedGetInfo, CSynchronizationAction >
{
	CSynchronizedGetInfo( my_context ctx ) : my_base( ctx )
	{
		LogPrintf("synchronize action: %p get info \n", &context< CSynchronizationAction >() );

		context< CSynchronizationAction >().forgetRequests();

		context< CSynchronizationAction >().addRequest(
				new common::CSendMessageRequest(
					common::CPayloadKind::InfoReq
					, common::CInfoRequestData( (int)common::CInfoKind::StorageInfoAsk, common::CSegmentFileStorage::getInstance()->getTimeStampOfLastFlush() )
					, context< CSynchronizationAction >().getActionKey()
					, new CByKeyMediumFilter( context< CSynchronizationAction >().getPartnerKey() ) ) );

		context< CSynchronizationAction >().addRequest(
					new common::CTimeEventRequest(
						SynchronisingWaitTime * 2
						, new CMediumClassFilter( common::CMediumKinds::Time ) ) );
	}

	boost::statechart::result react( common::CTimeEvent const & _timeEvent )
	{
		context< CSynchronizationAction >().forgetRequests();
		return discard_event();
	}

	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CTimeEvent >
	> reactions;

	// best synchronising option
	uint64_t m_bestTimeStamp;
};

struct CSynchronized : boost::statechart::state< CSynchronized, CSynchronizationAction >
{
	CSynchronized( my_context ctx ) : my_base( ctx ),m_currentBlock( 0 ),m_currentHeader( 0 )
	{

	}

	boost::statechart::result react( common::CAckEvent const & )
	{
		context< CSynchronizationAction >().forgetRequests();

		return discard_event();
	}

	~CSynchronized()
	{
		delete m_diskBlock;
		delete m_segmentHeader;

		common::CSegmentFileStorage::getInstance()->releaseSynchronizationInProgress();
	}

	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CAckEvent >
	> reactions;

	unsigned int m_storedBlocks;
	unsigned int m_currentBlock;

	unsigned int m_storedHeaders;
	unsigned int m_currentHeader;

	common::CDiskBlock * m_diskBlock;
	common::CSegmentHeader * m_segmentHeader;
};

CSynchronizationAction::CSynchronizationAction( CPubKey const & _partnerKey )
	: m_partnerKey( _partnerKey )
{
	LogPrintf("synchronize action: %p synchronizing \n", this );

	initiate();
	process_event( CSwitchToSynchronizing() );
}

CSynchronizationAction::CSynchronizationAction( uint256 const & _actionKey, CPubKey const & _partnerKey, uint64_t _timeStamp )
	: m_timeStamp( _timeStamp )
	, m_partnerKey( _partnerKey )
{
	LogPrintf("synchronize action: %p synchronized \n", this );

	initiate();
	process_event( CSwitchToSynchronized() );
}

void
CSynchronizationAction::accept( common::CSetResponseVisitor & _visitor )
{
	_visitor.visit( *this );
}

void
CSynchronizationAction::clear()
{
}

bool
CSynchronizationAction::isRequestInitialized() const
{
	return !m_requests.empty();
}

}
