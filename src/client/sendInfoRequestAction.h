// Copyright (c) 2014-2015 DiMS dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SEND_INFO_REQUEST_ACTION_H
#define SEND_INFO_REQUEST_ACTION_H

#include "common/action.h"
#include "tracker/validationManager.h"
#include "common/requestHandler.h"

namespace client
{


struct NetworkInfo
{
	enum Enum
	{
		  Monitor
		, Tracker
	};
};

struct TrackerInfo
{
	enum Enum
	{
		  Ip
		, Price
		, Rating
        , PublicKey
		, MinPrice
		, MaxPrice
	};
};

extern std::vector< TrackerInfo::Enum > const TrackerDescription;

class CSendInfoRequestAction : public common::CAction
{
public:
	CSendInfoRequestAction( NetworkInfo::Enum const _networkInfo );

	virtual void accept( common::CSetResponseVisitor & _visitor );
};

}

#endif // SEND_INFO_REQUEST_ACTION_H
