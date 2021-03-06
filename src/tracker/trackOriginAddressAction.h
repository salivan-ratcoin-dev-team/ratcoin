// Copyright (c) 2014-2015 DiMS dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TRACK_ORIGIN_ADDRESS_ACTION_H
#define TRACK_ORIGIN_ADDRESS_ACTION_H

#include "common/action.h"
#include <boost/statechart/state_machine.hpp>
#include "main.h"
#define MAX_TIME_MODIFIER 2.5
// please add excluding of bad nodes( not responding )

namespace common
{
	class CMedium;
}

namespace tracker
{
struct CUninitiatedTrackAction;

class CTrackOriginAddressAction : public common::CAction, public  boost::statechart::state_machine< CTrackOriginAddressAction, CUninitiatedTrackAction >
{
public:
	static CTrackOriginAddressAction * createInstance();

	static CTrackOriginAddressAction * getInstance();

	virtual void accept( common::CSetResponseVisitor & _visitor );

	void requestFiltered();

	void analyseOutput( common::CMedium * _key, std::map< uint256 ,std::vector< CTransaction > > const & _newTransactions, std::vector< CMerkleBlock > const & _newInput );

	void clear();

	void clearAccepted( unsigned int const _number );

	double getTimeModifier() const{ return m_timeModifier; }

	void increaseModifier()
	{
		if ( MAX_TIME_MODIFIER > m_timeModifier )
			m_timeModifier += 0.5;
	}

	void adjustTracking();

	uint256 getCurrentHash() const { return m_currentHash; }
private:
	CTrackOriginAddressAction();

	void validPart( common::CMedium * _key, std::vector< CMerkleBlock > const & _input, std::vector< CMerkleBlock > & _rejected );
private:
	//replace std::vector with std::deque ???
	std::map< common::CMedium *, std::vector< CMerkleBlock > > m_blocks;
	//replace std::vector with std::deque ???
	std::map< common::CMedium *, std::vector< CMerkleBlock > > m_acceptedBlocks;

	std::map< common::CMedium *, std::map< uint256 , std::vector< CTransaction > > > m_transactions;

	uint256 m_currentHash;

	double m_timeModifier;

    unsigned int m_updated;

	static CTrackOriginAddressAction * ms_instance;
};

}

#endif // TRACK_ORIGIN_ADDRESS_ACTION_H
