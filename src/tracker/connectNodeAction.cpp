// Copyright (c) 2014-2015 Dims dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "connectNodeAction.h"
#include "common/setResponseVisitor.h"
#include "common/commonEvents.h"
#include "common/authenticationProvider.h"
#include "common/mediumRequests.h"

#include "tracker/trackerNodesManager.h"
#include "tracker/trackerFilters.h"
#include "tracker/trackerController.h"
#include "tracker/trackerControllerEvents.h"
#include "tracker/trackerNodeMedium.h"
#include "tracker/trackerRequests.h"

#include <boost/statechart/simple_state.hpp>
#include <boost/statechart/state.hpp>
#include <boost/statechart/transition.hpp>
#include <boost/statechart/custom_reaction.hpp>

namespace tracker
{

// use  similar  algorithms like in client  to recognize network while connecting ???

unsigned int const TrackerLoopTime = 20;
unsigned int const SeedLoopTime = 25;
unsigned int const MonitorLoopTime = 25;

struct CUnconnected; struct CBothUnidentifiedConnected;


struct CConnectNodeActionUninitiated : boost::statechart::simple_state< CConnectNodeActionUninitiated, CConnectNodeAction >
{
	typedef boost::mpl::list<
	boost::statechart::transition< common::CSwitchToConnectingEvent, CUnconnected >,
	boost::statechart::transition< common::CSwitchToConnectedEvent, CBothUnidentifiedConnected >
	> reactions;

};

class ConnectedToSeed;

class ConnectedToMonitor;

class ConnectedToTracker;

template < class Parent >
void
createIdentifyResponse( Parent & parent )
{
	uint256 hash = Hash( &parent.getPayload().front(), &parent.getPayload().back() );

	std::vector< unsigned char > signedHash;
	common::CAuthenticationProvider::getInstance()->sign( hash, signedHash );

	parent.addRequests( new common::CIdentifyResponse<TrackerResponses>( new CSpecificMediumFilter( parent.getMediumPtr() ), signedHash, common::CAuthenticationProvider::getInstance()->getMyKey(), parent.getPayload(), parent.getActionKey() ) );
}

struct CDetermineRoleConnecting : boost::statechart::state< CDetermineRoleConnecting, CConnectNodeAction >
{
	CDetermineRoleConnecting( my_context ctx ) : my_base( ctx )
	{
		context< CConnectNodeAction >().dropRequests();
		context< CConnectNodeAction >().addRequests( new common::CNetworkRoleRequest<TrackerResponses>( context< CConnectNodeAction >().getActionKey(), common::CRole::Tracker, new CSpecificMediumFilter( context< CConnectNodeAction >().getMediumPtr() ) ) );
	}

	boost::statechart::result react( common::CRoleEvent const & _roleEvent )
	{
		context< CConnectNodeAction >().dropRequests();
		context< CConnectNodeAction >().addRequests( new common::CAckRequest< TrackerResponses >( context< CConnectNodeAction >().getActionKey(), new CSpecificMediumFilter( context< CConnectNodeAction >().getMediumPtr() ) ) );

		switch ( _roleEvent.m_role )
		{
		case common::CRole::Tracker:
			CTrackerController::getInstance()->process_event( CConnectedToTrackerEvent() );
			return transit< ConnectedToTracker >();
		case common::CRole::Seed:
			return transit< ConnectedToSeed >();
		case common::CRole::Monitor:
			return transit< ConnectedToMonitor >();
		default:
			break;
		}

		return discard_event();
	}

	boost::statechart::result react( common::CAckEvent const & _ackEvent )
	{
		return discard_event();
	}

	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CRoleEvent >,
	boost::statechart::custom_reaction< common::CAckEvent >
	> reactions;
};

struct CPairIdentifiedConnecting : boost::statechart::state< CPairIdentifiedConnecting, CConnectNodeAction >
{
	CPairIdentifiedConnecting( my_context ctx ) : my_base( ctx )
	{
		common::CIdentificationResult const* requestedEvent = dynamic_cast< common::CIdentificationResult const* >( simple_state::triggering_event() );

		uint256 hash = Hash( &requestedEvent->m_payload.front(), &requestedEvent->m_payload.back() );

		if ( requestedEvent->m_key.Verify( hash, requestedEvent->m_signed ) )
		{
			context< CConnectNodeAction >().setPublicKey( requestedEvent->m_key );

			CTrackerNodesManager::getInstance()->setPublicKey( context< CConnectNodeAction >().getServiceAddress(), requestedEvent->m_key );
			context< CConnectNodeAction >().dropRequests();
			context< CConnectNodeAction >().addRequests( new common::CAckRequest< TrackerResponses >( context< CConnectNodeAction >().getActionKey(), new CSpecificMediumFilter( context< CConnectNodeAction >().getMediumPtr() ) ) );
			createIdentifyResponse( context< CConnectNodeAction >() );
		}
		else
		{
		// something  is  wrong  with  pair react  somehow for  now put 0
			context< CConnectNodeAction >().dropRequests();
		}
	}

	typedef boost::mpl::list<
	boost::statechart::transition< common::CAckEvent, CDetermineRoleConnecting >
	> reactions;

};

struct CDetermineRoleConnected : boost::statechart::state< CDetermineRoleConnected, CConnectNodeAction >
{
	CDetermineRoleConnected( my_context ctx ) : my_base( ctx )
	{
		context< CConnectNodeAction >().dropRequests();
	}

	boost::statechart::result react( common::CRoleEvent const & _roleEvent )
	{
		m_role = _roleEvent.m_role;
		context< CConnectNodeAction >().dropRequests();
		context< CConnectNodeAction >().addRequests( new common::CAckRequest< TrackerResponses >( context< CConnectNodeAction >().getActionKey(), new CSpecificMediumFilter( context< CConnectNodeAction >().getMediumPtr() ) ) );
		context< CConnectNodeAction >().addRequests( new common::CNetworkRoleRequest<TrackerResponses>( context< CConnectNodeAction >().getActionKey(), common::CRole::Tracker, new CSpecificMediumFilter( context< CConnectNodeAction >().getMediumPtr() ) ) );

		switch ( m_role )
		{
		case common::CRole::Tracker:
			CTrackerController::getInstance()->process_event( CTrackerConnectingEvent() );
			return transit< ConnectedToTracker >();
		case common::CRole::Seed:
			return transit< ConnectedToSeed >();
		case common::CRole::Monitor:
			return transit< ConnectedToMonitor >();
		default:
			break;
		}
		return discard_event();
	}

	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CRoleEvent >
	> reactions;

	int m_role;
};

struct CPairIdentifiedConnected : boost::statechart::state< CPairIdentifiedConnected, CConnectNodeAction >
{
	CPairIdentifiedConnected( my_context ctx ) : my_base( ctx )
	{
		context< CConnectNodeAction >().dropRequests();
	}

	boost::statechart::result react( common::CIdentificationResult const & _identificationResult )
	{
		uint256 hash = Hash( &_identificationResult.m_payload.front(), &_identificationResult.m_payload.back() );

		if ( _identificationResult.m_key.Verify( hash, _identificationResult.m_signed ) )
		{
			CTrackerNodesManager::getInstance()->setPublicKey( _identificationResult.m_address, _identificationResult.m_key );
			context< CConnectNodeAction >().dropRequests();
			context< CConnectNodeAction >().addRequests( new common::CAckRequest< TrackerResponses >( context< CConnectNodeAction >().getActionKey(), new CSpecificMediumFilter( context< CConnectNodeAction >().getMediumPtr() ) ) );
		}
		else
		{
			context< CConnectNodeAction >().dropRequests();
		}

		return discard_event();
	}
	//boost::statechart::transition< common::CAckPromptResult, CDetermineRoleConnected >
	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CIdentificationResult >
	> reactions;
};


struct CBothUnidentifiedConnecting : boost::statechart::state< CBothUnidentifiedConnecting, CConnectNodeAction >
{
	CBothUnidentifiedConnecting( my_context ctx ) : my_base( ctx )
	{

		common::CNodeConnectedEvent const* connectedEvent = dynamic_cast< common::CNodeConnectedEvent const* >( simple_state::triggering_event() );
		context< CConnectNodeAction >().setMediumPtr( convertToInt( connectedEvent->m_node ) );
		// looks funny that  I set it in this  state, but let  it  be
		CTrackerNodesManager::getInstance()->addNode( new CTrackerNodeMedium( connectedEvent->m_node ) );
		context< CConnectNodeAction >().setServiceAddress( connectedEvent->m_node->addr );

		context< CConnectNodeAction >().dropRequests();
		context< CConnectNodeAction >().addRequests( new common::CIdentifyRequest<TrackerResponses>( new CSpecificMediumFilter( convertToInt( connectedEvent->m_node ) ), context< CConnectNodeAction >().getPayload(), context< CConnectNodeAction >().getActionKey() ) );
	}

	boost::statechart::result react( common::CAckEvent const & _ackEvent )
	{
		return discard_event();
	}

	typedef boost::mpl::list<
	boost::statechart::transition< common::CIdentificationResult, CPairIdentifiedConnecting >,
	boost::statechart::custom_reaction< common::CAckEvent >
	> reactions;
};

struct CBothUnidentifiedConnected : boost::statechart::state< CBothUnidentifiedConnected, CConnectNodeAction >
{
	CBothUnidentifiedConnected( my_context ctx ) : my_base( ctx )
	{
		context< CConnectNodeAction >().dropRequests();
		context< CConnectNodeAction >().addRequests( new common::CAckRequest< TrackerResponses >( context< CConnectNodeAction >().getActionKey(), new CSpecificMediumFilter( context< CConnectNodeAction >().getMediumPtr() ) ) );
	}

	boost::statechart::result react( common::CAckPromptResult const & _ackPrompt )
	{
		createIdentifyResponse( context< CConnectNodeAction >() );
		return discard_event();
	}

	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CAckPromptResult >,
	boost::statechart::transition< common::CAckEvent, CPairIdentifiedConnected >
	> reactions;
};

// it is  wrong but  for  now...
struct CCantReachNode : boost::statechart::state< CCantReachNode, CConnectNodeAction >
{
	CCantReachNode( my_context ctx ) : my_base( ctx )
	{
				context< CConnectNodeAction >().dropRequests();
	}
};

struct CUnconnected : boost::statechart::state< CUnconnected, CConnectNodeAction >
{
	CUnconnected( my_context ctx ) : my_base( ctx )
	{
		context< CConnectNodeAction >().dropRequests();
		context< CConnectNodeAction >().addRequests(
				  new CConnectToTrackerRequest( context< CConnectNodeAction >().getAddress(), context< CConnectNodeAction >().getServiceAddress() ) );
	}

	typedef boost::mpl::list<
	boost::statechart::transition< common::CNodeConnectedEvent, CBothUnidentifiedConnecting >,
	boost::statechart::transition< common::CCantReachNode, CCantReachNode >
	> reactions;

};

struct ConnectedToTracker : boost::statechart::state< ConnectedToTracker, CConnectNodeAction >
{
	ConnectedToTracker( my_context ctx ) : my_base( ctx )
	{
		CTrackerNodesManager::getInstance()->setNodeInfo(
					common::CValidNodeInfo( context< CConnectNodeAction >().getPublicKey(), context< CConnectNodeAction >().getServiceAddress() ), common::CRole::Seed );

		CTrackerNodesManager::getInstance()->setKeyToNode(
					  context< CConnectNodeAction >().getPublicKey()
					, context< CConnectNodeAction >().getMediumPtr()
					);

		context< CConnectNodeAction >().dropRequests();

		common::CAuthenticationProvider::getInstance()->addPubKey( context< CConnectNodeAction >().getPublicKey() );

		m_enterStateTime = GetTime();
	}

	boost::statechart::result react( common::CAckPromptResult const & _promptAck )
	{
		return discard_event();
	}

	int64_t m_enterStateTime;

	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CAckPromptResult >
	> reactions;
};

struct CStop;

struct ConnectedToSeed : boost::statechart::state< ConnectedToSeed, CConnectNodeAction >
{
	ConnectedToSeed( my_context ctx ) : my_base( ctx )
	{
		CTrackerNodesManager::getInstance()->setNodeInfo(
					common::CValidNodeInfo( context< CConnectNodeAction >().getPublicKey(), context< CConnectNodeAction >().getServiceAddress() ), common::CRole::Seed );

		CTrackerNodesManager::getInstance()->setKeyToNode(
					  context< CConnectNodeAction >().getPublicKey()
					, context< CConnectNodeAction >().getMediumPtr()
					);

		m_enterStateTime = GetTime();
		context< CConnectNodeAction >().dropRequests();
	}

	boost::statechart::result react( common::CNetworkInfoEvent const & _networkInfo )
	{
		common::CKnownNetworkInfo knownNetworkInfo;
		knownNetworkInfo.m_trackersInfo = CTrackerNodesManager::getInstance()->getNodesInfo( common::CRole::Tracker );
		knownNetworkInfo.m_monitorsInfo = CTrackerNodesManager::getInstance()->getNodesInfo( common::CRole::Monitor );

		context< CConnectNodeAction >().dropRequests();
		context< CConnectNodeAction >().addRequests( new common::CKnownNetworkInfoRequest< TrackerResponses >( context< CConnectNodeAction >().getActionKey(), knownNetworkInfo, new CSpecificMediumFilter( context< CConnectNodeAction >().getMediumPtr() ) ) );
		return discard_event();
	}

	boost::statechart::result react( common::CAckEvent const & _promptAck )
	{
		return transit< CStop >();
	}

	int64_t m_enterStateTime;

	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CNetworkInfoEvent >,
	boost::statechart::custom_reaction< common::CAckEvent >
	> reactions;
};

struct ConnectedToMonitor : boost::statechart::state< ConnectedToMonitor, CConnectNodeAction >
{
	ConnectedToMonitor( my_context ctx ) : my_base( ctx )
	{
		CTrackerNodesManager::getInstance()->setNodeInfo(
					common::CValidNodeInfo( context< CConnectNodeAction >().getPublicKey(), context< CConnectNodeAction >().getServiceAddress() ), common::CRole::Monitor );

		CTrackerNodesManager::getInstance()->setKeyToNode(
					  context< CConnectNodeAction >().getPublicKey()
					, context< CConnectNodeAction >().getMediumPtr()
					);

		m_enterStateTime = GetTime();
		context< CConnectNodeAction >().dropRequests();
	}


	boost::statechart::result react( common::CMessageResult const & _connectCondition )
	{
		common::CMessage orginalMessage;
		if ( !common::CommunicationProtocol::unwindMessage( _connectCondition.m_message, orginalMessage, GetTime(), context< CConnectNodeAction >().getPublicKey() ) )
			assert( !"service it somehow" );

		common::CConnectCondition connectCondition;

		common::convertPayload( orginalMessage, connectCondition );

		unsigned int result = 0;

		CMonitorData & acquireMonitorData = CTrackerController::getInstance()->acquireMonitorData();

		if ( !acquireMonitorData.m_isAdmitted )
		{
			if ( acquireMonitorData.m_allowAdmission )
			{
				if ( acquireMonitorData.m_accepableRatio >= ( double )connectCondition.m_price / ( double )connectCondition.m_period.GetLow64() )
				{
					result = 1;
				}
			}

		}
		context< CConnectNodeAction >().dropRequests();
		context< CConnectNodeAction >().addRequests( new common::CResultRequest< TrackerResponses >( context< CConnectNodeAction >().getActionKey(), result, new CSpecificMediumFilter( context< CConnectNodeAction >().getMediumPtr() ) ) );

		if ( !result )
			 transit< CStop >();

		return discard_event();
	}

	boost::statechart::result react( common::CAckEvent const & _ackEvent )
	{
		return transit< CStop >();
	}

	int64_t m_enterStateTime;

	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CMessageResult >,
	boost::statechart::custom_reaction< common::CAckEvent >
	> reactions;
};

struct CStop : boost::statechart::state< CStop, CConnectNodeAction >
{
	CStop( my_context ctx ) : my_base( ctx )
	{
			context< CConnectNodeAction >().dropRequests();
	}
};

CConnectNodeAction::CConnectNodeAction( uint256 const & _actionKey, std::vector< unsigned char > const & _payload, uintptr_t _mediumPtr )
: CCommunicationAction( _actionKey )
, m_payload( _payload )
, m_passive( true )
, m_mediumPtr( _mediumPtr )
{
	initiate();
	process_event( common::CSwitchToConnectedEvent() );
}

CConnectNodeAction::CConnectNodeAction( CAddress const & _addrConnect )
	: m_passive( false )
	, m_addrConnect( _addrConnect )
{
	for ( unsigned int i = 0; i < ms_randomPayloadLenght; i++ )
	{
		m_payload.push_back( insecure_rand() % 256 );
	}
	initiate();
	process_event( common::CSwitchToConnectingEvent() );
}


CConnectNodeAction::CConnectNodeAction( std::string const & _nodeAddress )
	: m_nodeAddress( _nodeAddress )
	, m_passive( false )
{
	for ( unsigned int i = 0; i < ms_randomPayloadLenght; i++ )
	{
		m_payload.push_back( insecure_rand() % 256 );
	}
	initiate();
	process_event( common::CSwitchToConnectingEvent() );
}

void
CConnectNodeAction::accept( common::CSetResponseVisitor< TrackerResponses > & _visitor )
{
	_visitor.visit( *this );
}

std::string
CConnectNodeAction::getAddress() const
{
	return m_nodeAddress;
}

CAddress
CConnectNodeAction::getServiceAddress() const
{
	return m_addrConnect;
}

void
CConnectNodeAction::setServiceAddress( CAddress const & _addrConnect )
{
	m_addrConnect = _addrConnect;
}

std::vector< unsigned char >  const &
CConnectNodeAction::getPayload() const
{
	return m_payload;
}

uintptr_t
CConnectNodeAction::getMediumPtr() const
{
	return m_mediumPtr;
}

CPubKey
CConnectNodeAction::getPublicKey() const
{
	return m_key;
}

void
CConnectNodeAction::setPublicKey( CPubKey const & _pubKey )
{
	m_key = _pubKey;
}


void
CConnectNodeAction::setMediumPtr( uintptr_t _mediumPtr )
{
	m_mediumPtr = _mediumPtr;
}

}
