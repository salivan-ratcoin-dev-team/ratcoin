// Copyright (c) 2014 Dims dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "acceptNodeAction.h"
#include "common/setResponseVisitor.h"
#include "common/commonEvents.h"
#include "common/authenticationProvider.h"
#include "common/mediumRequests.h"

#include <boost/statechart/simple_state.hpp>
#include <boost/statechart/state.hpp>
#include <boost/statechart/transition.hpp>
#include <boost/statechart/custom_reaction.hpp>

#include "acceptNodeAction.h"
#include "seedNodesManager.h"

#include "seedDb.h"
#include "seedFilter.h"


namespace seed
{
extern CAddrDb db;

struct CUnconnected; struct CBothUnidentifiedConnected;


struct CUninitiated : boost::statechart::simple_state< CUninitiated, CAcceptNodeAction >
{
	typedef boost::mpl::list<
	boost::statechart::transition< common::CSwitchToConnectingEvent, CUnconnected >,
	boost::statechart::transition< common::CSwitchToConnectedEvent, CBothUnidentifiedConnected >
	> reactions;

};


struct CIdentified : boost::statechart::state< CIdentified, CAcceptNodeAction >
{
	CIdentified( my_context ctx ) : my_base( ctx )
	{
		context< CAcceptNodeAction >().setValid( true );
		context< CAcceptNodeAction >().setRequest( 0 );
	}
};


template < class Parent >
void
createIdentifyResponse( Parent & parent )
{
	uint256 hash = Hash( &parent.getPayload().front(), &parent.getPayload().back() );

	std::vector< unsigned char > signedHash;
	common::CAuthenticationProvider::getInstance()->sign( hash, signedHash );

	parent.setRequest( new common::CIdentifyResponse<SeedResponses>( new CSpecificMediumFilter( parent.getMediumKind() ), signedHash, common::CAuthenticationProvider::getInstance()->getMyKey(), parent.getPayload(), parent.getActionKey() ) );
}

struct ConnectedToTracker;
struct ConnectedToSeed;
struct ConnectedToMonitor;

struct CPairIdentifiedConnecting : boost::statechart::state< CPairIdentifiedConnecting, CAcceptNodeAction >
{
	CPairIdentifiedConnecting( my_context ctx ) : my_base( ctx )
	{
		common::CIntroduceEvent const* requestedEvent = dynamic_cast< common::CIntroduceEvent const* >( simple_state::triggering_event() );

		uint256 hash = Hash( &requestedEvent->m_payload.front(), &requestedEvent->m_payload.back() );

		if ( requestedEvent->m_key.Verify( hash, requestedEvent->m_signed ) )
		{
			createIdentifyResponse( context< CAcceptNodeAction >() );
		}
		else
		{
			context< CAcceptNodeAction >().setRequest( 0 );
		}

	}

	boost::statechart::result react( common::CContinueEvent const & _continueEvent )
	{
		context< CAcceptNodeAction >().setRequest( new common::CContinueReqest<SeedResponses>( _continueEvent.m_keyId, new CSpecificMediumFilter( context< CAcceptNodeAction >().getMediumKind() ) ) );
	}

	boost::statechart::result react( common::CRoleEvent const & _roleEvent )
	{
		context< CAcceptNodeAction >().setRequest( new common::CNetworkRoleRequest<SeedResponses>( context< CAcceptNodeAction >().getActionKey(), common::CRole::Seed, new CSpecificMediumFilter( context< CAcceptNodeAction >().getMediumKind() ) ) );

		switch ( _roleEvent.m_role )
		{
		case common::CRole::Tracker:
			return transit< ConnectedToTracker >();
		case common::CRole::Seed:
			return transit< ConnectedToSeed >();
		case common::CRole::Monitor:
			return transit< ConnectedToMonitor >();
		default:
			break;
		}
	}

	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CRoleEvent >,
	boost::statechart::custom_reaction< common::CContinueEvent >
	> reactions;

};

struct CPairIdentifiedConnected : boost::statechart::state< CPairIdentifiedConnected, CAcceptNodeAction >
{
	CPairIdentifiedConnected( my_context ctx ) : my_base( ctx )
	{
		common::CIntroduceEvent const* requestedEvent = dynamic_cast< common::CIntroduceEvent const* >( simple_state::triggering_event() );

		uint256 hash = Hash( &requestedEvent->m_payload.front(), &requestedEvent->m_payload.back() );

		if ( requestedEvent->m_key.Verify( hash, requestedEvent->m_signed ) )
		{
			m_address = requestedEvent->m_address;

			CSeedNodesManager::getInstance()->setPublicKey( m_address, requestedEvent->m_key );

			context< CAcceptNodeAction >().setRequest( new common::CNetworkRoleRequest<SeedResponses>( context< CAcceptNodeAction >().getActionKey(), common::CRole::Seed, new CSpecificMediumFilter( context< CAcceptNodeAction >().getMediumKind() ) ) );
		}
		else
		{
			context< CAcceptNodeAction >().setValid( false );
			context< CAcceptNodeAction >().setRequest( 0 );
		}
	}

	boost::statechart::result react( const common::CContinueEvent & _continueEvent )
	{
		context< CAcceptNodeAction >().setRequest( new common::CContinueReqest<SeedResponses>( _continueEvent.m_keyId, new CSpecificMediumFilter( context< CAcceptNodeAction >().getMediumKind() ) ) );
	}

	boost::statechart::result react( common::CRoleEvent const & _roleEvent )
	{

		switch ( _roleEvent.m_role )
		{
		case common::CRole::Tracker:
			context< CAcceptNodeAction >().setRequest( new common::CAckRequest<SeedResponses>( context< CAcceptNodeAction >().getActionKey(), new CSpecificMediumFilter( context< CAcceptNodeAction >().getMediumKind() ) ) );
			db.Add(m_address);
			return transit< ConnectedToTracker >();
		case common::CRole::Seed:
			return transit< ConnectedToSeed >();
		case common::CRole::Monitor:
			db.Add(m_address);
			return transit< ConnectedToMonitor >();
		default:
			break;
		}
	}

	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CRoleEvent >,
	boost::statechart::custom_reaction< common::CContinueEvent >
	> reactions;

	CAddress m_address;

};


struct CBothUnidentifiedConnecting : boost::statechart::state< CBothUnidentifiedConnecting, CAcceptNodeAction >
{
	CBothUnidentifiedConnecting( my_context ctx ) : my_base( ctx )
	{

		common::CNodeConnectedEvent const* connectedEvent = dynamic_cast< common::CNodeConnectedEvent const* >( simple_state::triggering_event() );
		context< CAcceptNodeAction >().setMediumKind( convertToInt( connectedEvent->m_node ) );
		// looks funny that  I set it in this  state, but let  it  be
		CSeedNodesManager::getInstance()->addNode( connectedEvent->m_node );
		context< CAcceptNodeAction >().setRequest( new common::CIdentifyRequest<SeedResponses>( new CSpecificMediumFilter( convertToInt( connectedEvent->m_node ) ), context< CAcceptNodeAction >().getPayload(), context< CAcceptNodeAction >().getActionKey() ) );

	}

	boost::statechart::result react( const common::CContinueEvent & _continueEvent )
	{
		context< CAcceptNodeAction >().setRequest( new common::CContinueReqest<SeedResponses>( _continueEvent.m_keyId, new CSpecificMediumFilter( context< CAcceptNodeAction >().getMediumKind() ) ) );
	}

	typedef boost::mpl::list<
	boost::statechart::transition< common::CIntroduceEvent, CPairIdentifiedConnecting >,
	boost::statechart::custom_reaction< common::CContinueEvent >
	> reactions;
};

struct CBothUnidentifiedConnected : boost::statechart::state< CBothUnidentifiedConnected, CAcceptNodeAction >
{
	CBothUnidentifiedConnected( my_context ctx ) : my_base( ctx )
	{
		createIdentifyResponse( context< CAcceptNodeAction >() );

	}

	boost::statechart::result react( const common::CContinueEvent & _continueEvent )
	{
		context< CAcceptNodeAction >().setRequest( new common::CContinueReqest<SeedResponses>( _continueEvent.m_keyId, new CSpecificMediumFilter( context< CAcceptNodeAction >().getMediumKind() ) ) );
	}

	typedef boost::mpl::list<
	boost::statechart::transition< common::CIntroduceEvent, CPairIdentifiedConnected >,
	boost::statechart::custom_reaction< common::CContinueEvent >
	> reactions;
};

struct CCantReachNode : boost::statechart::state< CCantReachNode, CAcceptNodeAction >
{
	CCantReachNode( my_context ctx ) : my_base( ctx )
	{
		context< CAcceptNodeAction >().setValid( false );
		context< CAcceptNodeAction >().setRequest( 0 );
	}
};

struct CUnconnected : boost::statechart::state< CUnconnected, CAcceptNodeAction >
{
	CUnconnected( my_context ctx ) : my_base( ctx )
	{
		context< CAcceptNodeAction >().setRequest(
				  new common::CConnectToNodeRequest< SeedResponses >( std::string(""), context< CAcceptNodeAction >().getAddress(), 0 ) );

	}

	typedef boost::mpl::list<
	boost::statechart::transition< common::CNodeConnectedEvent, CBothUnidentifiedConnecting >,
	boost::statechart::transition< common::CCantReachNode, CCantReachNode >
	> reactions;

};

struct ConnectedToTracker : boost::statechart::state< ConnectedToTracker, CAcceptNodeAction >
{
	ConnectedToTracker( my_context ctx ) : my_base( ctx )
	{
		context< CAcceptNodeAction >().setValid( true );
	}

	boost::statechart::result react( common::CNetworkInfoEvent const & _networkInfo )
	{
		context< CAcceptNodeAction >().setRequest( 0 );

		BOOST_FOREACH( common::CValidNodeInfo validNodeInfo, _networkInfo.m_networkInfo )
		{
			if ( validNodeInfo.m_role == common::CRole::Tracker || validNodeInfo.m_role == common::CRole::Monitor )
			{
				db.Add( validNodeInfo.m_address );
			}
		}
	}

	boost::statechart::result react( const common::CAckEvent & _ackEvent )
	{
		context< CAcceptNodeAction >().setRequest( new common::CAckRequest< SeedResponses >( context< CAcceptNodeAction >().getActionKey(), new CSpecificMediumFilter( context< CAcceptNodeAction >().getMediumKind() ) ) );
	}


	boost::statechart::result react( const common::CContinueEvent & _continueEvent )
	{
		context< CAcceptNodeAction >().setRequest( new common::CContinueReqest< SeedResponses >( _continueEvent.m_keyId, new CSpecificMediumFilter( context< CAcceptNodeAction >().getMediumKind() ) ) );
	}

	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CNetworkInfoEvent >,
	boost::statechart::custom_reaction< common::CAckEvent >,
	boost::statechart::custom_reaction< common::CContinueEvent >
	> reactions;
};

struct ConnectedToSeed : boost::statechart::state< ConnectedToSeed, CAcceptNodeAction >
{
	ConnectedToSeed( my_context ctx ) : my_base( ctx )
	{
		context< CAcceptNodeAction >().setRequest( 0 );
	}
};

struct ConnectedToMonitor : boost::statechart::state< ConnectedToMonitor, CAcceptNodeAction >
{
	ConnectedToMonitor( my_context ctx ) : my_base( ctx )
	{
		context< CAcceptNodeAction >().setRequest( 0 );

	}
};



struct CSynchronizing : boost::statechart::simple_state< CSynchronizing, CAcceptNodeAction >
{

};

CAcceptNodeAction::CAcceptNodeAction( uint256 const & _actionKey, std::vector< unsigned char > const & _payload, unsigned int _mediumKind )
: common::CCommunicationAction( _actionKey )
, m_payload( _payload )
, m_request( 0 )
, m_passive( true )
, m_mediumKind( _mediumKind )
, m_valid( false )
{
	initiate();
	process_event( common::CSwitchToConnectedEvent() );
}

CAcceptNodeAction::CAcceptNodeAction( CAddress const & _nodeAddress )
	: common::CAction< SeedResponses >( false )
	, m_nodeAddress( _nodeAddress )
	, m_request( 0 )
	, m_passive( false )
	, m_valid( false )
{
	for ( unsigned int i = 0; i < ms_randomPayloadLenght; i++ )
	{
		m_payload.push_back( insecure_rand() % 256 );
	}
	initiate();
	process_event( common::CSwitchToConnectingEvent() );
}

common::CRequest< SeedResponses >*
CAcceptNodeAction::execute()
{
	return m_request;
}

void
CAcceptNodeAction::accept( common::CSetResponseVisitor< SeedResponses > & _visitor )
{
	_visitor.visit( *this );
}

void
CAcceptNodeAction::setRequest( common::CRequest< SeedResponses >* _request )
{
	m_request = _request;
}

CAddress
CAcceptNodeAction::getAddress() const
{
	return m_nodeAddress;
}

std::vector< unsigned char > const &
CAcceptNodeAction::getPayload() const
{
	return m_payload;
}

unsigned int
CAcceptNodeAction::getMediumKind() const
{
	return m_mediumKind;
}

void
CAcceptNodeAction::setMediumKind( unsigned int _mediumKind )
{
	m_mediumKind = _mediumKind;
}



}
