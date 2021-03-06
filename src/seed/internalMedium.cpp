// Copyright (c) 2014-2015 DiMS dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "internalMedium.h"
#include "common/requests.h"
#include "common/manageNetwork.h"

#include <algorithm>

namespace seed
{

CInternalMedium * CInternalMedium::ms_instance = NULL;

CInternalMedium*
CInternalMedium::getInstance()
{
	if ( !ms_instance )
	{
		ms_instance = new CInternalMedium();
	};
	return ms_instance;
}

CInternalMedium::CInternalMedium()
{
}

bool
CInternalMedium::serviced() const
{
	return true;
}

void
CInternalMedium::add( common::CConnectToNodeRequest const *_request )
{
// in general  it is to slow to be  handled  this  way, but  as usual we can live with that for a while
	common::CSelfNode* node = common::CManageNetwork::getInstance()->connectNode( _request->getServiceAddress(), _request->getAddress().empty()? 0 : _request->getAddress().c_str() );

	m_responses.insert( std::make_pair( (common::CRequest*)_request, common::CConnectedNode( node ) ) );
}


bool
CInternalMedium::getResponseAndClear( std::multimap< common::CRequest const*, common::DimsResponse > & _requestResponse )
{
	_requestResponse = m_responses;
	clearResponses();
	return true;
}

void
CInternalMedium::clearResponses()
{
	m_responses.clear();
}


}

