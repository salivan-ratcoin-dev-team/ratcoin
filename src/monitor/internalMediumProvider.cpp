// Copyright (c) 2014-2015 Dims dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "internalMediumProvider.h"
#include "internalOperationsMedium.h"

#include "common/timeMedium.h"
#include "common/scheduledActionManager.h"

namespace monitor
{

CInternalMediumProvider * CInternalMediumProvider::ms_instance = NULL;

CInternalMediumProvider*
CInternalMediumProvider::getInstance( )
{
	if ( !ms_instance )
	{
		ms_instance = new CInternalMediumProvider();
	};
	return ms_instance;
}

CInternalMediumProvider::CInternalMediumProvider()
{
}

std::list< common::CMonitorBaseMedium *>
CInternalMediumProvider::provideConnection( common::CMonitorMediumFilter const & _mediumFilter )
{
	return _mediumFilter.getMediums( this );
}

void
CInternalMediumProvider::registerRemoveCallback( CNodeSignals& nodeSignals )
{
	nodeSignals.NotifyTrackerNode.connect( bind( &CInternalMediumProvider::removeNodeCallback, this, _1) );
}

void
CInternalMediumProvider::removeNodeCallback( CNode * node )
{
	boost::lock_guard<boost::mutex> lock( m_mutex );
	m_nodeToMedium.erase( node );

}


std::list< common::CMonitorBaseMedium *>
CInternalMediumProvider::getMediumByClass( common::CMediumKinds::Enum _mediumKind, unsigned int _mediumNumber )
{
	boost::lock_guard<boost::mutex> lock( m_mutex );
	std::list< common::CMonitorBaseMedium *> mediums;

	if ( common::CMediumKinds::Internal == _mediumKind )
	{
		mediums.push_back( CInternalOperationsMedium::getInstance() );
	}
	else if ( common::CMediumKinds::Time == _mediumKind )
	{
		mediums.push_back( common::CTimeMedium< common::CMonitorBaseMedium >::getInstance() );
	}
	else if ( common::CMediumKinds::Schedule == _mediumKind )
	{
		mediums.push_back( common::CScheduledActionManager< common::CMonitorTypes >::getInstance() );
	}
	else if ( common::CMediumKinds::BitcoinsNodes == _mediumKind )
	{

		std::map< CNode *, CBitcoinNodeMedium * >::const_iterator iterator =  m_nodeToMedium.begin();
		//simplified  approach
		for ( unsigned int i = 0; ( i < vNodes.size() ) && ( i < _mediumNumber ); )
		{

			if ( iterator != m_nodeToMedium.end() )
			{
				// validate that node  is  still working??
				/*		mediums.push_back( static_cast< common::CMonitorBaseMedium * >( iterator->second ) );
				iterator++;
				++i;*/
			}
			else
			{
				/*
				CNode * node = vNodes.at( i );
				m_nodeToMedium.insert( std::make_pair( node, new CBitcoinNodeMedium( node ) ) );
				//ugly
				iterator =  m_nodeToMedium.begin();
				std::advance( iterator, i );*/
			}

		}
	}
	return mediums;
}


}
