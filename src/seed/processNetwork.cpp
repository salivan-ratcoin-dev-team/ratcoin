// Copyright (c) 2014-2015 DiMS dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "common/actionHandler.h"
#include "common/communicationProtocol.h"
#include "common/events.h"
#include "common/networkActionRegister.h"

#include "seed/processNetwork.h"
#include "seed/seedNodeMedium.h"
#include "seed/seedNodesManager.h"
#include "seed/acceptNodeAction.h"
#include "seed/pingAction.h"

namespace seed
{
	CServiceResult service;//ugly
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
	if ( !CSeedNodesManager::getInstance()->getMediumForNode( pfrom ) )
	{
		CSeedNodesManager::getInstance()->addNode( new CSeedNodeMedium( pfrom ) );
	}

	BOOST_FOREACH( common::CMessage const & message, messages )
	{
		if ( message.m_header.m_payloadKind == common::CPayloadKind::IntroductionReq )
		{
			common::CIdentifyMessage identifyMessage;
			convertPayload( message, identifyMessage );

			common::CNodeMedium * nodeMedium = CSeedNodesManager::getInstance()->getMediumForNode( pfrom );

			if ( common::CNetworkActionRegister::getInstance()->isServicedByAction( message.m_header.m_actionKey ) )
			{
				nodeMedium->addActionResponse(
							message.m_header.m_actionKey
							, common::CIdentificationResult(
								  identifyMessage.m_payload
								, identifyMessage.m_signed
								, identifyMessage.m_key
								, pfrom->addr
								, message.m_header.m_id ) );
			}
			else
			{
				CAcceptNodeAction * connectNodeAction = new CAcceptNodeAction(
							message.m_header.m_actionKey, convertToInt( nodeMedium->getNode() ), service );

				connectNodeAction->process_event( common::CSwitchToConnectedEvent() );
				connectNodeAction->process_event( common::CIdentificationResult( identifyMessage.m_payload, identifyMessage.m_signed, identifyMessage.m_key, pfrom->addr, message.m_header.m_id ) );
				common::CActionHandler::getInstance()->executeAction( connectNodeAction );
			}

		}
		else if (  message.m_header.m_payloadKind == common::CPayloadKind::Ack )
		{
			common::CAck ack;

			common::convertPayload( message, ack );

			common::CNodeMedium * nodeMedium = CSeedNodesManager::getInstance()->getMediumForNode( pfrom );

			if ( common::CNetworkActionRegister::getInstance()->isServicedByAction( message.m_header.m_actionKey ) )
			{
				nodeMedium->setResponse( message.m_header.m_id, common::CAckResult( convertToInt( nodeMedium->getNode() ) ) );
			}
		}
		else
		{

			common::CNodeMedium * nodeMedium = CSeedNodesManager::getInstance()->getMediumForNode( pfrom );

			CPubKey pubKey;

			if ( !CSeedNodesManager::getInstance()->getNodePublicKey( convertToInt( pfrom ), pubKey ) )
				return true;

			if ( common::CNetworkActionRegister::getInstance()->isServicedByAction( message.m_header.m_actionKey ) )
			{
				if ( message.m_header.m_payloadKind == common::CPayloadKind::InfoReq )
					nodeMedium->addActionResponse( message.m_header.m_actionKey, common::CMessageResult( message, pubKey ) );
				else
					nodeMedium->setResponse( message.m_header.m_id, common::CMessageResult( message, pubKey ) );
			}
			else if ( message.m_header.m_payloadKind == common::CPayloadKind::Ping )
			{
				CPingAction * pingAction = new CPingAction( message.m_header.m_actionKey );

				pingAction->process_event( common::CMessageResult( message, pubKey ) );

				common::CActionHandler::getInstance()->executeAction( pingAction );
			}
		}

	}
	return true;
}

bool
CProcessNetwork::sendMessages(common::CSelfNode* pto, bool fSendTrickle)
{
	pto->sendMessages();

	return true;
}

}


namespace common
{
void
CSelfNode::clearManager()
{
	common::CNodesManager::getInstance()->eraseMedium( convertToInt( this ) );
}
}
