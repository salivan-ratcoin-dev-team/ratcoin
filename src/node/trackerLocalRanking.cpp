// Copyright (c) 2014-2015 Dims dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "trackerLocalRanking.h"
#include "networkClient.h"

#include "base58.h"

namespace client
{

CTrackerLocalRanking * CTrackerLocalRanking::ms_instance = NULL;


CTrackerLocalRanking::CTrackerLocalRanking()
{
}

CTrackerLocalRanking::~CTrackerLocalRanking()
{
	if ( ms_instance )
		delete ms_instance;
	ms_instance = 0;
}

CTrackerLocalRanking*
CTrackerLocalRanking::getInstance( )
{
	if ( !ms_instance )
	{
		ms_instance = new CTrackerLocalRanking();
	};
	return ms_instance;
}

void
CTrackerLocalRanking::addTracker( common::CTrackerStats const & _trackerStats )
{
	m_balancedRanking.insert( _trackerStats ); 
	m_reputationRanking.insert( _trackerStats );
}

void
CTrackerLocalRanking::addUnidentifiedNode( std::string const & _ip, common::CUnidentifiedNodeInfo const & _unidentifiedNode )
{
	m_unidentifiedNodes.insert( std::make_pair( _ip, _unidentifiedNode ) );
}


bool
CTrackerLocalRanking::isInUnidentified( std::string const & _ip ) const
{
	return m_unidentifiedNodes.find( _ip ) != m_unidentifiedNodes.end();
}

void
CTrackerLocalRanking::removeUnidentifiedNode( std::string const & _ip )
{
	m_unidentifiedNodes.erase( _ip );
}

void
CTrackerLocalRanking::addUndeterminedTracker( common::CNodeInfo const & _undeterminedTracker )
{
	CPubKey pubKey;

	if ( !getNodeKey( _undeterminedTracker.m_ip, pubKey ) )
	{
		setIpAndKey( _undeterminedTracker.m_ip, _undeterminedTracker.m_key );
		pubKey = _undeterminedTracker.m_key;
	}
	m_undeterminedTrackers.insert( std::make_pair( pubKey, _undeterminedTracker ) );
}

bool
CTrackerLocalRanking::getUndeterminedTracker( std::string const & _ip, common::CNodeInfo & _undeterminedTracker )
{
	CPubKey pubKey;
	getNodeKey( _ip, pubKey );

	std::map< CPubKey, common::CNodeInfo >::const_iterator iterator = m_undeterminedTrackers.find( pubKey );

	if ( iterator == m_undeterminedTrackers.end() )
		return false;

	_undeterminedTracker = iterator->second;

	return true;
}

void
CTrackerLocalRanking::removeUndeterminedTracker( std::string const & _ip )
{
	CPubKey pubKey;

	if ( getNodeKey( _ip, pubKey ) )
	{
		m_undeterminedTrackers.erase( pubKey );
	}
}

void
CTrackerLocalRanking::addMonitor( common::CNodeInfo const & _monitor )
{
	CPubKey pubKey;

	if ( !getNodeKey( _monitor.m_ip, pubKey ) )
	{
		setIpAndKey( _monitor.m_ip, _monitor.m_key );
		pubKey = _monitor.m_key;
	}
	m_monitors.insert( std::make_pair( pubKey, _monitor ) );
}

void
CTrackerLocalRanking::resetMonitors()
{
	m_monitors.clear();
}

void
CTrackerLocalRanking::resetTrackers()
{
	m_undeterminedTrackers.clear();
	m_reputationRanking.clear();
	m_balancedRanking.clear();
}

void
CTrackerLocalRanking::removeMonitor( std::string const & _ip )
{
	CPubKey pubKey;

	if ( getNodeKey( _ip, pubKey ) )
	{
		m_monitors.erase( pubKey );
	}
}

std::list< common::CMedium< NodeResponses > *>
CTrackerLocalRanking::provideConnection( common::CMediumFilter< NodeResponses > const & _mediumFilter )
{
	return _mediumFilter.getMediums( this );
}

std::list< common::CMedium< NodeResponses > *>
CTrackerLocalRanking::getMediumByClass( common::RequestKind::Enum _requestKind, unsigned int _mediumNumber )
{
	std::list< common::CMedium< NodeResponses > *> mediums;

	switch ( _requestKind )
	{
	case common::RequestKind::Unknown:
		if ( m_unidentifiedNodes.begin() != m_unidentifiedNodes.end() )
		{
			BOOST_FOREACH( Unidentified const & stats, m_unidentifiedNodes )
			{
				mediums.push_back( getNetworkConnection( stats.second ) );
			}
		}
		break;
	case common::RequestKind::UndeterminedTrackers:
		if ( m_undeterminedTrackers.begin() != m_undeterminedTrackers.end() )
		{
			BOOST_FOREACH( PAIRTYPE( CPubKey, common::CNodeInfo ) const & stats, m_undeterminedTrackers )
			{
				mediums.push_back( getNetworkConnection( stats.second ) );
			}
		}
	break;
	case common::RequestKind::Monitors:
		if ( m_monitors.begin() != m_monitors.end() )
		{
			BOOST_FOREACH( PAIRTYPE( CPubKey, common::CNodeInfo ) const & stats, m_monitors )
			{
				mediums.push_back( getNetworkConnection( stats.second ) );
			}
		}
	break;
	case common::RequestKind::Transaction:
		if ( m_balancedRanking.begin() != m_balancedRanking.end() )
		{
			BOOST_FOREACH( common::CTrackerStats const & stats, m_balancedRanking )
			{
				mediums.push_back( getNetworkConnection( stats) );
			}
		}
		break;
	case common::RequestKind::TransactionStatus:
	case common::RequestKind::Balance:
		if ( m_reputationRanking.begin() != m_reputationRanking.end() )
		{
			BOOST_FOREACH( common::CTrackerStats const  & stats, m_balancedRanking )
			{
				mediums.push_back( getNetworkConnection( stats ) );
			}
		}
		break;
		break;
	default:
		;
	}
	// there will be  not many  mediums  I belive
	if ( _mediumNumber != -1 && mediums.size() > _mediumNumber )
		mediums.resize( _mediumNumber );
	return mediums;
}

common::CMedium< NodeResponses > *
CTrackerLocalRanking::getSpecificTracker( uintptr_t _trackerPtr ) const
{
	std::map< uintptr_t, common::CMedium< NodeResponses > * >::const_iterator iterator = m_mediumRegister.find( _trackerPtr );

	return iterator != m_mediumRegister.end() ? iterator->second : 0;

}

bool
CTrackerLocalRanking::isValidTrackerKnown( CKeyID const & _trackerId )
{
	BOOST_FOREACH( common::CTrackerStats const & trackerStats, m_reputationRanking )
	{
		if ( trackerStats.m_key.GetID() == _trackerId )
			return true;
	}

	return false;
}

bool
CTrackerLocalRanking::getTrackerStats( CKeyID const & _trackerId, common::CTrackerStats & _trackerStats )
{
	if ( !isValidTrackerKnown( _trackerId ) )
		return false;

	BOOST_FOREACH( common::CTrackerStats const & trackerStats, m_reputationRanking )
	{
		if ( trackerStats.m_key.GetID() == _trackerId )
		{
			_trackerStats = trackerStats;
			return true;
		}
	}
	assert( !"can't be here" );
	return false;
}

bool
CTrackerLocalRanking::getNodeKey( std::string const & _ip, CPubKey & _pubKey ) const
{
	std::map< std::string, CPubKey >::const_iterator iterator = m_ipToKey.find( _ip );
	if ( iterator != m_ipToKey.end() )
	{
		_pubKey = iterator->second;
		return true;
	}
	else
	{
		return false;
	}
}

bool
CTrackerLocalRanking::getNodeInfo( CPubKey const & _key, common::CNodeInfo & _nodeInfo ) const
{
	/*
	std::map< CPubKey, common::CNodeInfo >::const_iterator iterator = m_monitors.find( _ip );

	if ( iterator != m_ipToKey.end() )

	std::map< CPubKey, common::CNodeInfo > m_monitors;

	std::map< CPubKey, common::CNodeInfo > m_undeterminedTrackers;*/
}

void
CTrackerLocalRanking::setIpAndKey( std::string const & _ip, CPubKey const _pubKey )
{
	m_ipToKey.insert( std::make_pair( _ip, _pubKey ) );
}

bool
CTrackerLocalRanking::getSpecificTrackerMedium( CKeyID const & _trackerId, common::CMedium< NodeResponses > *& _medium )
{
	if ( !isValidTrackerKnown( _trackerId ) )
		return false;

	BOOST_FOREACH( common::CTrackerStats const & trackerStats, m_reputationRanking )
	{
		if ( trackerStats.m_key.GetID() == _trackerId )
		{
			assert( m_createdMediums.find( trackerStats.m_ip ) != m_createdMediums.end() );

			_medium = m_createdMediums.find( trackerStats.m_ip )->second;
			return true;
		}
	}

	assert( !"can't be here" );
	return false;
}

bool
CTrackerLocalRanking::isValidMonitorKnown( CKeyID const & _monitorId )
{
	BOOST_FOREACH( PAIRTYPE( CPubKey, common::CNodeInfo ) const & monitor, m_monitors )
	{
		if ( monitor.second.m_key.GetID() == _monitorId )
			return true;
	}

	return false;
}

float
CTrackerLocalRanking::getPrice()
{
	return 0.0;
}

}
