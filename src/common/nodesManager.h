// Copyright (c) 2014 Dims dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NODES_MANAGER_H
#define NODES_MANAGER_H

/*
very probable that this  should be in common
*/
#include "connectionProvider.h"
#include "selfNode.h"
#include "nodeMedium.h"

#include "manageNetwork.h"

#include "mediumKinds.h"

namespace common
{

template < class RequestType > class CNodeMedium;

// class common::CConnectionProvider is problematic, may providers make  things confusing, sometimes it is not  clear which provider is responsible for  mediums of specific
template < class RequestType >
class CNodesManager : public common::CConnectionProvider< RequestType >
{
public:
	bool getMessagesForNode( common::CSelfNode * _node, std::vector< common::CMessage > & _messages );

	bool processMessagesFromNode( common::CSelfNode * _node, std::vector< common::CMessage > const & _messages );

	void connectNodes();

	CNodeMedium< RequestType > * addNode( common::CSelfNode * _node );

	std::list< common::CMedium< RequestType > *> provideConnection( CMediumFilter< RequestType > const & _mediumFilter );

	CNodeMedium< RequestType >* getMediumForNode( common::CSelfNode * _node ) const;

	common::CMedium< RequestType > * findNodeMedium( uintptr_t _ptr ) const;

	std::list< common::CMedium< RequestType > *> getNodesByClass( CMediumKinds::Enum _nodesClass ) const;

	static CNodesManager * getInstance();
protected:
	CNodesManager();

	void handleMessages();

	void propagateMessage();

	void analyseMessage();

	mutable boost::mutex m_nodesLock;

	//std::list< CSelfNode * > m_unidentified;

	std::map< unsigned int, CNodeMedium< RequestType >* > m_ptrToNodes;
protected:
	static CNodesManager< RequestType > * ms_instance;
// replace  this by trackers, mediums, seeds ???????????????
	std::list< common::CMedium< RequestType > *> m_nodeMediums;

};

template < class RequestType >
CNodesManager< RequestType >*
CNodesManager< RequestType >::getInstance( )
{
	if ( !ms_instance )
	{
		ms_instance = new CNodesManager< RequestType >();
	};
	return ms_instance;
}

template < class RequestType >
CNodesManager< RequestType >::CNodesManager()
{
}

template < class RequestType >
CNodeMedium< RequestType > *
CNodesManager< RequestType >::addNode( common::CSelfNode * _node )
{
	boost::lock_guard<boost::mutex> lock( m_nodesLock );

	CNodeMedium< RequestType > * medium = new CNodeMedium< RequestType >( _node );
	m_ptrToNodes.insert( std::make_pair( convertToInt( _node ), medium ) );

	return medium;
}

template < class RequestType >
void
CNodesManager< RequestType >::handleMessages()
{

}

template < class RequestType >
void
CNodesManager< RequestType >::connectNodes()
{
}

template < class RequestType >
void
CNodesManager< RequestType >::propagateMessage()
{

}

template < class RequestType >
void
CNodesManager< RequestType >::analyseMessage()
{

}

template < class RequestType >
CNodeMedium< RequestType >*
CNodesManager< RequestType >::getMediumForNode( common::CSelfNode * _node ) const
{
	typename std::map< unsigned int, CNodeMedium< RequestType >* >::const_iterator iterator = m_ptrToNodes.find( convertToInt( _node ) );
	if ( iterator != m_ptrToNodes.end() )
	{
		return iterator->second;
	}

	return 0;
}

template < class ResponseType >
std::list< common::CMedium< ResponseType > *>
CNodesManager< ResponseType >::provideConnection( CMediumFilter< ResponseType > const & _mediumFilter )
{
	return _mediumFilter.getMediums( this );
}

template < class ResponseType >
common::CMedium< ResponseType > *
CNodesManager< ResponseType >::findNodeMedium( uintptr_t _ptr ) const
{
	typename std::map< unsigned int, CNodeMedium< ResponseType >* >::const_iterator iterator = m_ptrToNodes.find( _ptr );

	return iterator != m_ptrToNodes.end() ? iterator->second : 0;
}

template < class ResponseType >
std::list< common::CMedium< ResponseType > *>
CNodesManager< ResponseType >::getNodesByClass( CMediumKinds::Enum _nodesClass ) const
{
	if ( CMediumKinds::DimsNodes == _nodesClass )
	{
		return m_nodeMediums;
	}

}


}

#endif // NODES_MANAGER_H
