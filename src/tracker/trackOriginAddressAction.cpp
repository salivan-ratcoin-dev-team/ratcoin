// Copyright (c) 2014-2015 Dims dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "trackOriginAddressAction.h"

#include "common/originAddressScanner.h"
#include "common/setResponseVisitor.h"
#include "common/commonRequests.h"
#include "common/commonEvents.h"
#include "common/mediumKinds.h"

#include <boost/statechart/simple_state.hpp>
#include <boost/statechart/state.hpp>
#include <boost/statechart/transition.hpp>
#include <boost/statechart/custom_reaction.hpp>

#include "main.h"
#include "chainparams.h"

#include "tracker/events.h"
#include "tracker/controller.h"
#include "tracker/controllerEvents.h"
#include "tracker/filters.h"
#include "tracker/trackerNodeMedium.h"
#include "tracker/requests.h"

namespace tracker
{

unsigned int const MaxMerkleNumber = 1000;
unsigned int const WaitResultTime = 30000;
unsigned int const CleanTime = 2;
unsigned int const SynchronizedTreshold = 10;

/*
store last  tracked  block  number
track blocks  between  last  checked  and present
use  at  least  three node  and  compare  results, in case of inconsistency of data  replace  wrong node  by  other correct  one


 what I need is bicoin node  medium

*/
struct CReadingData;
struct CEvaluateProgress;

struct CUninitiatedTrackAction : boost::statechart::state< CUninitiatedTrackAction, CTrackOriginAddressAction >
{
	CUninitiatedTrackAction( my_context ctx ) : my_base( ctx )
	{
		context< CTrackOriginAddressAction >().dropRequests();
		context< CTrackOriginAddressAction >().addRequest( new common::CTimeEventRequest< common::CTrackerTypes >( 1000, new CMediumClassFilter( common::CMediumKinds::Time ) ) );
	}

	boost::statechart::result react( common::CTimeEvent const & _timeEvent )
	{
		CController::getInstance()->process_event( common::CBitcoinNetworkConnection( vNodes.size() ) );

		if ( vNodes.size() >= common::dimsParams().getUsedBitcoinNodesNumber() )
		{
			context< CTrackOriginAddressAction >().requestFiltered();// could proceed  with origin address scanning
			return transit< CReadingData >();
		}
		else
		{
			context< CTrackOriginAddressAction >().dropRequests();
			context< CTrackOriginAddressAction >().addRequest( new common::CTimeEventRequest< common::CTrackerTypes >( 1000, new CMediumClassFilter( common::CMediumKinds::Time ) ) );
		}

		return discard_event();
	}
	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CTimeEvent >
	> reactions;
};

struct CReadingData : boost::statechart::state< CReadingData, CTrackOriginAddressAction >
{
	CReadingData( my_context ctx ) : my_base( ctx )
	{
		context< CTrackOriginAddressAction >().addRequest(
					new common::CTimeEventRequest< common::CTrackerTypes >( ( unsigned int )context< CTrackOriginAddressAction >().getTimeModifier() * WaitResultTime, new CMediumClassFilter( common::CMediumKinds::Time ) ) );
	}

	boost::statechart::result react( common::CTimeEvent const & _timeEvent )
	{
		return transit< CEvaluateProgress >();
	}

	boost::statechart::result react( common::CMerkleBlocksEvent const & _merkleblockEvent )
	{
		context< CTrackOriginAddressAction >().analyseOutput( _merkleblockEvent.m_nodePtr, _merkleblockEvent.m_transactions, _merkleblockEvent.m_merkles );
		return discard_event();
	}

	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CTimeEvent >,
	boost::statechart::custom_reaction< common::CMerkleBlocksEvent >
	> reactions;
};

struct CEvaluateProgress : boost::statechart::state< CEvaluateProgress, CTrackOriginAddressAction >
{
	CEvaluateProgress( my_context ctx ) : my_base( ctx )
	{
		context< CTrackOriginAddressAction >().adjustTracking();

		context< CTrackOriginAddressAction >().addRequest(
					new common::CTimeEventRequest< common::CTrackerTypes >( ( unsigned int )1000, new CMediumClassFilter( common::CMediumKinds::Time ) ) );
	}

	boost::statechart::result react( common::CTimeEvent const & _timeEvent )
	{
		context< CTrackOriginAddressAction >().clear();
		return transit< CUninitiatedTrackAction >();
	}

	typedef boost::mpl::list<
	boost::statechart::custom_reaction< common::CTimeEvent >
	> reactions;
};

CTrackOriginAddressAction::CTrackOriginAddressAction()
	: m_timeModifier( 1 )
	, m_updated( 0 )
{
	initiate();

	CBlock block;

	if ( FileExist("head") )
	{
		CAutoFile file(OpenHeadFile(true), SER_DISK, CLIENT_VERSION);
		CBlockHeader header;
		file >> header;
		block = header;
	}
	else
	{
		block = const_cast<CBlock&>(Params().GenesisBlock());
	}

	m_currentHash = block.GetHash();
}

void
CTrackOriginAddressAction::accept( common::CSetResponseVisitor< common::CTrackerTypes > & _visitor )
{
	_visitor.visit( *this );
}
// current  hash  distance  get  merkle  and  transaction
// came  back  to  the  same  state  over  and  over  till
//  analysys  will be finished


void
CTrackOriginAddressAction::requestFiltered()
{
	CBlockIndex * index = chainActive.Tip();
	// for  now  for  simplicity reasons
	for ( int i = 0; i < Params().getConfirmationNumber(); i++ )
	{
		if ( index == 0 )
		{
			CController::getInstance()->process_event( common::CInitialSynchronizationDoneEvent() );
			return;
		}
		index = index->pprev;
	}

	std::vector< uint256 > requestedBlocks;
	while ( m_currentHash != index->GetBlockHash() )
	{
		requestedBlocks.push_back( index->GetBlockHash() );

		index = index->pprev;
	}
	std::reverse( requestedBlocks.begin(), requestedBlocks.end());

	tracker::CController::getInstance()->process_event( common::CSetScanBitcoinChainProgress( requestedBlocks.size() ) );

	if ( requestedBlocks.size() > MaxMerkleNumber )
		requestedBlocks.resize( MaxMerkleNumber );

	if ( requestedBlocks.size() < SynchronizedTreshold )
		CController::getInstance()->process_event( common::CInitialSynchronizationDoneEvent() );

	dropRequests();
	addRequest( new common::CAskForTransactionsRequest< common::CTrackerTypes >( requestedBlocks, new CMediumClassFilter( common::CMediumKinds::BitcoinsNodes, common::dimsParams().getUsedBitcoinNodesNumber() ) ) );

}

// it should be  done  in fency style in  final version,
// but for  now I will keep it simple as much as possible

typedef std::map< long long, std::vector< CMerkleBlock > >::value_type MerkleResult;

typedef std::map< long long, std::map< uint256 , std::vector< CTransaction > > >::value_type TransactionsResult;


struct CCompareTransactions
{
	CCompareTransactions():m_correct( true ),m_initialised( false ){}

	void verifyIfCorrect( std::vector< CTransaction > const & _transactions );

	bool isCorrect() const;

	std::vector< uint256 > m_hashes;

	bool m_correct;
	bool m_initialised;
};

void
CCompareTransactions::verifyIfCorrect( std::vector< CTransaction > const & _transactions )
{
	if ( !m_correct )
		return;

	std::vector< uint256 > hashes;

	BOOST_FOREACH( CTransaction const & transaction, _transactions )
	{
		hashes.push_back( transaction.GetHash() );
	}

	if ( !m_initialised )
	{
		m_hashes = hashes;
		m_initialised = true;
	}
	else
	{
		m_correct = m_hashes == hashes;
	}
}

bool
CCompareTransactions::isCorrect() const
{
	return m_correct;
}

//
void
CTrackOriginAddressAction::analyseOutput( long long _key, std::map< uint256 ,std::vector< CTransaction > > const & _newTransactions, std::vector< CMerkleBlock > const & _newInput )
{
	std::map< long long, std::map< uint256 , std::vector< CTransaction > > > ::iterator transactionIterator = m_transactions.find( _key );

	if ( transactionIterator == m_transactions.end() )
	{
		m_transactions.insert( std::make_pair( _key , _newTransactions ) );
	}
	else
	{
		transactionIterator->second.insert( _newTransactions.begin(), _newTransactions.end() );
	}

	std::map< long long, std::vector< CMerkleBlock > >::iterator iterator = m_blocks.find( _key );

	if ( iterator == m_blocks.end() )
	{
		m_blocks.insert( std::make_pair( _key , _newInput ) );

		std::vector< CMerkleBlock > & merkle = m_blocks.find( _key )->second;

		validPart( _key, merkle, merkle );
	}
	else
	{
		std::vector< CMerkleBlock > & merkle = iterator->second;

		merkle.insert( merkle.end(), _newInput.begin(), _newInput.end() );

		validPart( _key, merkle, merkle );
	}

	unsigned int size = -1;

	BOOST_FOREACH( MerkleResult & nodeResults, m_acceptedBlocks )
	{
		if ( size > nodeResults.second.size() )
			size = nodeResults.second.size();
	}

	if ( size == (unsigned int)-1 || size == 0 )
		return;

	// go  through transaction  queue analyse  if  the  same  content

	std::vector< uint256 > blocksToAccept;

	std::vector< CMerkleBlock > blockVector = m_acceptedBlocks.begin()->second;

	CBlockHeader headerToSave;

	for ( unsigned int i = 0; size > i; ++i )
	{
		blocksToAccept.push_back( blockVector.at( i ).header.GetHash() );

		if ( i == size -1 )
			headerToSave = blockVector.at( i ).header;

	}

	unsigned int const serviced = size;

	while( size-- )
	{
		std::vector<uint256> hashesVector, match;
		BOOST_FOREACH( MerkleResult & merkleVector, m_acceptedBlocks )
		{
			if ( hashesVector.empty() )
			{
				if ( !merkleVector.second.at( size ).txn.ExtractMatches(hashesVector) )
					assert( !"shouldn't be here" );// there is problem
			}
			else
			{
				if ( !merkleVector.second.at( size ).txn.ExtractMatches(match) )
					assert( !"shouldn't be here" );// there is problem
				if ( hashesVector != match )
					assert( !"shouldn't be here" );// problem which have to be resolved
			}
		}
		// compare  transactions


		BOOST_FOREACH( uint256 & hash, blocksToAccept )
		{
			CCompareTransactions compareTransactions;
			std::vector< CTransaction > toInclude;
			BOOST_FOREACH( TransactionsResult & transaction, m_transactions )
			{
				if ( toInclude.empty() )
				{
					if ( transaction.second.find( hash ) != transaction.second.end() )
						toInclude = transaction.second.find( hash )->second;
				}

				if ( transaction.second.find( hash ) != transaction.second.end() )
					compareTransactions.verifyIfCorrect( transaction.second.find( hash )->second );
				else
					compareTransactions.verifyIfCorrect( std::vector< CTransaction >() );
			}

			if ( !compareTransactions.isCorrect() )
				assert( !"data from various nodes vary!" );// react  to  this
			else
			{
				BOOST_FOREACH( CTransaction const & transaction, toInclude )
				{
					common::COriginAddressScanner::getInstance()->addTransaction( 0, transaction );
				}
			}

		}
	}
	// write  transactions to origin transaction scaner
	clearAccepted( serviced );

	CAutoFile file(OpenHeadFile(false), SER_DISK, CLIENT_VERSION);
	file << headerToSave;

	fflush(file);

	FileCommit(file);

	m_currentHash = headerToSave.GetHash();

	//remove accepted
}

// return list of  hashes
// later return  list of  problems


void
CTrackOriginAddressAction::validPart( long long _key, std::vector< CMerkleBlock > const & _input, std::vector< CMerkleBlock > & _rejected )
{
	if ( _input.empty() )
	{
		m_acceptedBlocks.insert( std::make_pair( _key , _input ) );
		return;
	}

	std::vector< CMerkleBlock > output = _input, accepted;

	std::reverse( output.begin(), output.end());

	CMerkleBlock block = output.back();

	std::map< long long, std::vector< CMerkleBlock > >::iterator iterator;
	iterator = m_acceptedBlocks.find( _key );

	uint256 lastAcceptedHash = iterator != m_acceptedBlocks.end() && !iterator->second.empty() ? iterator->second.back().header.GetHash() : this->m_currentHash;

	while ( !output.empty() )
	{
		if ( block.header.hashPrevBlock == lastAcceptedHash )
		{
			lastAcceptedHash = block.header.GetHash();
			accepted.push_back( block );
			output.pop_back();
			if ( !output.empty() )
				block = output.back();
		}
		else
		{
			std::vector< CMerkleBlock >::iterator iterator = output.begin();
			while( iterator != output.end() )
			{
				if ( lastAcceptedHash == iterator->header.hashPrevBlock )
				{
					accepted.push_back( *iterator );
					output.erase( iterator );
					break;
				}

				iterator++;
			}
			break;
		}
	}
	_rejected = output;

	if ( iterator == m_acceptedBlocks.end() )
	{
		m_acceptedBlocks.insert( std::make_pair( _key , accepted ) );
	}
	else
	{
		iterator->second.insert( iterator->second.end(), accepted.begin(), accepted.end() );
	}
}

void
CTrackOriginAddressAction::adjustTracking()
{
	if ( m_updated < MaxMerkleNumber * 0.1 )
	{
		unsigned int size;
		BOOST_FOREACH( MerkleResult & nodeResults, m_acceptedBlocks )
		{
			size = nodeResults.second.size();

			if ( m_updated + size < MaxMerkleNumber * 0.1 )
			{
				CInternalMediumProvider::getInstance()->stopCommunicationWithNode( nodeResults.first );
			}
		}
	}
	else if ( m_updated < MaxMerkleNumber / 2 )
	{
		increaseModifier();
	}
}

void
CTrackOriginAddressAction::clearAccepted( unsigned int const _number )
{
	BOOST_FOREACH( MerkleResult & nodeResults, m_acceptedBlocks )
	{
		nodeResults.second.erase(nodeResults.second.begin(), nodeResults.second.begin() + _number );
	}

	m_updated += _number;
}

void
CTrackOriginAddressAction::clear()
{
	m_blocks.clear();

	m_acceptedBlocks.clear();

	m_transactions.clear();

	m_updated = 0;
}

}
