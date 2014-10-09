// Copyright (c) 2014 Ratcoin dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "processNetwork.h"
#include "common/nodesManager.h"
#include "common/communicationProtocol.h"
#include "common/actionHandler.h"

#include "common/nodeMedium.h"

#include "configureMonitorActionHandler.h"

namespace common
{

template<> common::CNodesManager< monitor::MonitorResponses > * common::CNodesManager< monitor::MonitorResponses >::ms_instance = 0;

}

namespace monitor
{

CProcessNetwork * CProcessNetwork::ms_instance = NULL;

CProcessNetwork*
CProcessNetwork::getInstance()
{
	if ( !ms_instance )
	{
		ms_instance = new CProcessNetwork();
	};
	return ms_instance;
}

bool
CProcessNetwork::processMessage(common::CSelfNode* pfrom, CDataStream& vRecv)
{
	std::vector< common::CMessage > messages;
	vRecv >> messages;

// it is  stupid  to call this over and over again
	if ( !common::CNodesManager<MonitorResponses>::getInstance()->getMediumForNode( pfrom ) )
	{
		common::CNodesManager<MonitorResponses>::getInstance()->addNode( new common::CNodeMedium< MonitorResponses >( pfrom ) );
	}

	BOOST_FOREACH( common::CMessage const & message, messages )
	{


		if ( message.m_header.m_payloadKind == common::CPayloadKind::RoleInfo )
		{
			common::CNodeMedium< MonitorResponses > * nodeMedium = common::CNodesManager< MonitorResponses >::getInstance()->getMediumForNode( pfrom );

			if ( common::CNetworkActionRegister::getInstance()->isServicedByAction( message.m_header.m_actionKey ) )
			{
				nodeMedium->setResponse( message.m_header.m_actionKey, common::CMessageResult( message, convertToInt( nodeMedium->getNode() ) ) );
			}
			else
			{
			}
		}
		else if ( message.m_header.m_payloadKind == common::CPayloadKind::InfoReq )
		{
			//
		}
		else if ( message.m_header.m_payloadKind == common::CPayloadKind::IntroductionReq )
		{
			common::CIdentifyMessage identifyMessage;
			convertPayload( message, identifyMessage );

			common::CNodeMedium< MonitorResponses > * nodeMedium = common::CNodesManager< MonitorResponses >::getInstance()->getMediumForNode( pfrom );

			if ( common::CNetworkActionRegister::getInstance()->isServicedByAction( message.m_header.m_actionKey ) )
			{
				nodeMedium->setResponse( message.m_header.m_actionKey, common::CIdentificationResult( identifyMessage.m_payload, identifyMessage.m_signed, identifyMessage.m_key, pfrom->addr  ) );
			}
			else
			{
			/*	CConnectNodeAction * connectTrackerAction= new CConnectNodeAction(
							  message.m_header.m_actionKey
							, identifyMessage.m_payload
							, convertToInt( nodeMedium->getNode() ) );

				common::CActionHandler< TrackerResponses >::getInstance()->executeAction( connectTrackerAction );*/
			}

		}
		else if (  message.m_header.m_payloadKind == common::CPayloadKind::Ack )
		{
			common::CAck ack;

			common::convertPayload( message, ack );

			common::CNodeMedium< MonitorResponses > * nodeMedium = common::CNodesManager< MonitorResponses >::getInstance()->getMediumForNode( pfrom );

			if ( common::CNetworkActionRegister::getInstance()->isServicedByAction( message.m_header.m_actionKey ) )
			{
				nodeMedium->setResponse( message.m_header.m_actionKey, common::CAckResult( convertToInt( nodeMedium->getNode() ) ) );
			}
		}
		else if ( message.m_header.m_payloadKind == common::CPayloadKind::Uninitiated )
		{
			//
		}
	}
	/*


*/
	return true;
}


bool
CProcessNetwork::sendMessages(common::CSelfNode* pto, bool fSendTrickle)
{
	pto->sendMessages();

	return true;
}

}

