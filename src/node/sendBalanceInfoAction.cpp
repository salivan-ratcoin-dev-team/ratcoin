// Copyright (c) 2014 Dims dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/statechart/transition.hpp>
#include <boost/statechart/state.hpp>
#include <boost/statechart/custom_reaction.hpp>

#include "sendBalanceInfoAction.h"
#include "common/setResponseVisitor.h"
#include "common/commonEvents.h"
#include "sendInfoRequestAction.h"

#include "clientFilters.h"
#include "clientControl.h"
#include "clientEvents.h"

#include "serialize.h"
#include "base58.h"
#include "common/nodeMessages.h"

#include "wallet.h"

using namespace  common;

namespace client
{

struct CSwitchToGetBalanceEvent : boost::statechart::event< CSwitchToGetBalanceEvent >
{
};

struct CGetBalanceInfo;

struct CGetData : boost::statechart::state< CGetBalanceInfo, CSendBalanceInfoAction >
{
	CGetData( my_context ctx ) : my_base( ctx )
	{
		context< CSendBalanceInfoAction >().setAddresses( CClientControl::getInstance()->getAvailableAddresses() );
		context< CSendBalanceInfoAction >().process_event( CSwitchToGetBalanceEvent() );
	}

	typedef boost::mpl::list<
	boost::statechart::transition< CSwitchToGetBalanceEvent, CGetBalanceInfo >
	> reactions;
};

struct CGetBalanceInfo : boost::statechart::state< CGetBalanceInfo, CSendBalanceInfoAction >
{
	CGetBalanceInfo( my_context ctx ) : my_base( ctx )
	{
		m_addressIndex = 0;
		std::vector< std::string > const & m_addresses = context< CSendBalanceInfoAction >().getAddresses();

		if ( m_addressIndex < m_addresses.size() )
			context< CSendBalanceInfoAction >().setRequest( new CBalanceRequest( m_addresses.at( m_addressIndex++ ) ) );
		else
			context< CSendBalanceInfoAction >().setRequest( 0 );
	}

	boost::statechart::result react( CCoinsEvent const & _coinsEvent )
	{

		CBitcoinAddress address;
//		address.SetString( m_pubKey );

		CKeyID keyId;
		address.GetKeyID( keyId );
		typedef std::map< uint256, CCoins >::value_type  MapElement;
		std::vector< CAvailableCoin > availableCoins;
		BOOST_FOREACH( MapElement const & coins, _coinsEvent.m_coins )
		{
			std::vector< CAvailableCoin > tempCoins = context< CSendBalanceInfoAction >().getAvailableCoins( coins.second, keyId, coins.first );
			availableCoins.insert(availableCoins.end(), tempCoins.begin(), tempCoins.end());
		}
		CWallet::getInstance()->setAvailableCoins( keyId, availableCoins );

		std::vector< std::string > const & m_addresses = context< CSendBalanceInfoAction >().getAddresses();

		if ( m_addressIndex < m_addresses.size() )
			context< CSendBalanceInfoAction >().setRequest( new CBalanceRequest( m_addresses.at( m_addressIndex++ ) ) );
		else
			context< CSendBalanceInfoAction >().setRequest( 0 );
		return discard_event();
	}

	boost::statechart::result react( common::CContinueEvent const & _continueEvent )
	{
		context< CSendBalanceInfoAction >().setRequest( new common::CContinueReqest<NodeResponses>(uint256(), new CMediumClassFilter( common::RequestKind::Balance ) ) );
		return discard_event();
	}

	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CContinueEvent >,
	boost::statechart::custom_reaction< CCoinsEvent >
	> reactions;

	unsigned int m_addressIndex;
};

CSendBalanceInfoAction::CSendBalanceInfoAction( bool _autoDelete )
	: common::CAction< NodeResponses >( _autoDelete )
{
	initiate();
}

void
CSendBalanceInfoAction::accept( common::CSetResponseVisitor< NodeResponses > & _visitor )
{
	_visitor.visit( *this );
}

common::CRequest< NodeResponses >*
CSendBalanceInfoAction::execute()
{
    return 0;
}

void
CSendBalanceInfoAction::reset()
{
	initiate();
}

void
CSendBalanceInfoAction::setAddresses( std::vector< std::string > const & _addresses )
{
	m_addresses = _addresses;
}

std::vector< std::string > const &
CSendBalanceInfoAction::getAddresses() const
{
	return m_addresses;
}

std::vector< CAvailableCoin >
CSendBalanceInfoAction::getAvailableCoins( CCoins const & _coins, uint160 const & _pubId, uint256 const & _hash ) const
{
	std::vector< CAvailableCoin > availableCoins;
	unsigned int valueSum = 0, totalInputSum = 0;
	for (unsigned int i = 0; i < _coins.vout.size(); i++)
	{
		const CTxOut& txout = _coins.vout[i];

		opcodetype opcode;

		std::vector<unsigned char> data;

		CScript::const_iterator pc = txout.scriptPubKey.begin();
		//sanity check
		while( pc != txout.scriptPubKey.end() )
		{
			if (!txout.scriptPubKey.GetOp(pc, opcode, data))
				return std::vector< CAvailableCoin >();
		}
		txnouttype type;

		std::vector< std:: vector<unsigned char> > vSolutions;
		if (Solver(txout.scriptPubKey, type, vSolutions) &&
				(type == TX_PUBKEY || type == TX_PUBKEYHASH))
		{
			std::vector<std::vector<unsigned char> >::iterator it = vSolutions.begin();

			while( it != vSolutions.end() )
			{

				if (
						( ( type == TX_PUBKEY ) && ( _pubId == Hash160( *it ) ) )
					||	( ( type == TX_PUBKEYHASH ) && ( _pubId == uint160( *it ) ) )
					)
				{
					if ( !txout.IsNull() )
						availableCoins.push_back( CAvailableCoin( txout, i, _hash ) );
					break;
				}
				it++;
			}
		}
	}
	return availableCoins;
}

void
CSendBalanceInfoAction::setRequest( common::CRequest< NodeResponses > * _request )
{
	m_request = _request;
}

CBalanceRequest::CBalanceRequest( std::string _address )
	: common::CRequest< NodeResponses >( new CMediumClassFilter( RequestKind::Balance ) )
	, m_address( _address )
{
}

void
CBalanceRequest::accept( common::CMedium< NodeResponses > * _medium ) const
{
	_medium->add( this );
}

inline
common::CMediumFilter< NodeResponses > *
CBalanceRequest::getMediumFilter() const
{
	return common::CRequest< NodeResponses >::m_mediumFilter;
}

}
