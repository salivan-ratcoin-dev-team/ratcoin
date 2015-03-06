// Copyright (c) 2014-2015 Dims dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "trackerControllerStates.h"
#include "boost/foreach.hpp"
#include "protocol.h"

#include "common/manageNetwork.h"
#include "common/actionHandler.h"
#include "connectNodeAction.h"
#include "synchronizationAction.h"
#include "trackerEvents.h"
#include "trackerController.h"
#include "trackOriginAddressAction.h"

namespace tracker
{

CInitialSynchronization::CInitialSynchronization()
{
	common::CActionHandler< tracker::TrackerResponses >::getInstance()->executeAction( new tracker::CTrackOriginAddressAction );
}

CStandAlone::CStandAlone( my_context ctx ) : my_base( ctx )
{
	// search for  seeder  action
	std::vector<CAddress> vAdd;

	common::CManageNetwork::getInstance()->getIpsFromSeed( vAdd );

	if ( !vAdd.empty() )
	{
		BOOST_FOREACH( CAddress address, vAdd )
		{
			common::CActionHandler< CTrackerTypes >::getInstance()->executeAction( new CConnectNodeAction( address ) );
		}
	}
	else
	{
		common::CManageNetwork::getInstance()->getSeedIps( vAdd );

		// let know seed about our existence
		BOOST_FOREACH( CAddress address, vAdd )
		{
			common::CActionHandler< CTrackerTypes >::getInstance()->executeAction( new CConnectNodeAction( address ) );
		}
	}
}

CSynchronizing::CSynchronizing( my_context ctx ) : my_base( ctx )
{
	CSynchronizationAction * synchronizationAction = new CSynchronizationAction();

	common::CActionHandler< CTrackerTypes >::getInstance()->executeAction( synchronizationAction );
	synchronizationAction->process_event( CSwitchToSynchronizing() );

}


CConnected::CConnected( my_context ctx ) : my_base( ctx )
{
	CTrackerController::getInstance()->setConnected( true );
}


}
