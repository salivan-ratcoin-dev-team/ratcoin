// Copyright (c) 2014-2015 DiMS dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "common/setResponseVisitor.h"
#include "common/responseVisitorInternal.h"
#include "common/events.h"

#include "monitor/connectNodeAction.h"
#include "monitor/updateNetworkDataAction.h"
#include "monitor/admitTrackerAction.h"
#include "monitor/admitTransactionsBundle.h"
#include "monitor/pingAction.h"
#include "monitor/recognizeNetworkAction.h"
#include "monitor/trackOriginAddressAction.h"
#include "monitor/provideInfoAction.h"
#include "monitor/synchronizationAction.h"
#include "monitor/enterNetworkAction.h"
#include "monitor/passTransactionAction.h"
#include "monitor/reputationControlAction.h"
#include "monitor/updateNetworkDataAction.h"
#include "monitor/activityControllerAction.h"

namespace common
{

void
CSetResponseVisitor::visit( monitor::CConnectNodeAction & _action )
{
	boost::apply_visitor( CSetResult<monitor::CConnectNodeAction>( &_action ), m_responses );
}

void
CSetResponseVisitor::visit( monitor::CAdmitTrackerAction & _action )
{
	boost::apply_visitor( CSetResult<monitor::CAdmitTrackerAction>( &_action ), m_responses );
}

void
CSetResponseVisitor::visit( monitor::CAdmitTransactionBundle & _action )
{
	boost::apply_visitor( CSetResult<monitor::CAdmitTransactionBundle>( &_action ), m_responses );
}

void
CSetResponseVisitor::visit( monitor::CPingAction & _action )
{
	boost::apply_visitor( CSetResult<monitor::CPingAction>( &_action ), m_responses );
}

void
CSetResponseVisitor::visit( monitor::CRecognizeNetworkAction & _action )
{
	boost::apply_visitor( CSetResult<monitor::CRecognizeNetworkAction>( &_action ), m_responses );
}

void
CSetResponseVisitor::visit( monitor::CTrackOriginAddressAction & _action )
{
	boost::apply_visitor( CSetResult<monitor::CTrackOriginAddressAction>( &_action ), m_responses );
}

void
CSetResponseVisitor::visit( monitor::CProvideInfoAction & _action )
{
	boost::apply_visitor( CSetResult<monitor::CProvideInfoAction>( &_action ), m_responses );
}

void
CSetResponseVisitor::visit( monitor::CCopyTransactionStorageAction & _action )
{}

void
CSetResponseVisitor::visit( monitor::CSynchronizationAction & _action )
{
	boost::apply_visitor( CSetResult<monitor::CSynchronizationAction>( &_action ), m_responses );
}

void
CSetResponseVisitor::visit( monitor::CEnterNetworkAction & _action )
{
	boost::apply_visitor( CSetResult<monitor::CEnterNetworkAction>( &_action ), m_responses );
}

void
CSetResponseVisitor::visit( monitor::CPassTransactionAction & _action )
{
	boost::apply_visitor( CSetResult<monitor::CPassTransactionAction>( &_action ), m_responses );
}

void
CSetResponseVisitor::visit( monitor::CReputationControlAction & _action )
{
	boost::apply_visitor( CSetResult<monitor::CReputationControlAction>( &_action ), m_responses );
}

void
CSetResponseVisitor::visit( monitor::CUpdateNetworkDataAction & _action )
{
	boost::apply_visitor( CSetResult<monitor::CUpdateNetworkDataAction>( &_action ), m_responses );
}

void
CSetResponseVisitor::visit( monitor::CActivityControllerAction & _action )
{
	boost::apply_visitor( CSetResult<monitor::CActivityControllerAction>( &_action ), m_responses );
}

}

