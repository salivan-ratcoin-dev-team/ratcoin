#include "nodesManager.h"

namespace tracker
{

CNodesManager * CNodesManager::ms_instance = NULL;

CNodesManager*
CNodesManager::getInstance( )
{
	if ( !ms_instance )
	{
		ms_instance = new CNodesManager();
	};
	return ms_instance;
}


CNodesManager::CNodesManager()
{

}

void
CNodesManager::PropagateBundle( std::vector< CTransaction > _bundle )
{

}

void
CNodesManager::handleMessages()
{

}

bool
CNodesManager::getMessagesForNode( CNode * _node, std::vector< CMessage > & _messages )
{
	return true;
}

bool
CNodesManager::processMessagesFormNode( CNode * _node, std::vector< CMessage > const & _messages )
{
	return true;
}

void
CNodesManager::connectNodes()
{
}

void
CNodesManager::propagateMessage()
{

}

void
CNodesManager::analyseMessage()
{

}

}
