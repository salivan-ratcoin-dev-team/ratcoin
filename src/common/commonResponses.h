#ifndef TRACKER_RESPONSES_H
#define TRACKER_RESPONSES_H


#include "coins.h"
#include "nodeMessages.h"

#include <boost/variant.hpp>
#include <boost/statechart/event.hpp>

#include <exception>
#include "errorResponse.h"
#include "net.h"
#include "communicationProtocol.h"

namespace common
{

struct CDummy
{
	uint256 m_token;
	IMPLEMENT_SERIALIZE
	(
		READWRITE( m_token );
	)
};

struct CAvailableCoins
{
	CAvailableCoins( std::map< uint256, CCoins > const & _availableCoins, uint256 const & _hash );
	CAvailableCoins();

	IMPLEMENT_SERIALIZE
	(
		READWRITE(m_hash);
		READWRITE(m_availableCoins);
	)

	static CMainRequestType::Enum const  m_requestType;
	uint256 m_hash;
	std::map< uint256, CCoins > m_availableCoins;
};

struct CIdentificationResult
{
	CIdentificationResult( std::vector<unsigned char> const & _payload, std::vector<unsigned char> const & _signed, CPubKey const & _key, CAddress const & _address = CAddress() ):m_payload( _payload ),m_signed( _signed ),m_key( _key ),m_address(_address){};
	std::vector<unsigned char> m_payload;
	std::vector<unsigned char> m_signed;
	CPubKey m_key;
	CAddress m_address;
};


struct CContinueResult
{
	CContinueResult( uint256 const &_key ):m_id(_key){};
	uint256 m_id;
};

struct CAckPromptResult : boost::statechart::event< CAckPromptResult >
{
};

struct CGetPrompt
{
	CGetPrompt( int _type ):m_type( _type ){}
	int m_type;
};

struct CMediumException : public std::exception
{
public:
	CMediumException(ErrorType::Enum _error):m_error(_error){};
	ErrorType::Enum m_error;
};

struct CRoleResult
{
	CRoleResult(int _role):m_role( _role ){}
	int m_role;
};

struct CNetworkInfoResult
{
	CNetworkInfoResult( std::vector< CValidNodeInfo > const & _networkInfo ):m_networkInfo( _networkInfo ){}

	CNetworkInfoResult(){}

	IMPLEMENT_SERIALIZE
	(
		READWRITE(m_networkInfo);
	)

	std::vector< CValidNodeInfo > m_networkInfo;
};


struct CClientNetworkInfoResult
{

	CClientNetworkInfoResult( std::vector< CValidNodeInfo > const & _networkInfo, CPubKey const & _selfKey, int _selfRole  ):m_networkInfo( _networkInfo ),m_selfKey( _selfKey ), m_selfRole( _selfRole ){}

	CClientNetworkInfoResult( std::vector< CValidNodeInfo > const & _networkInfo, CPubKey const & _selfKey, int _selfRole , std::string _ip, uintptr_t _nodeIndicator ):m_networkInfo( _networkInfo ),m_selfKey( _selfKey ), m_selfRole( _selfRole ){}

	CClientNetworkInfoResult(){}

	IMPLEMENT_SERIALIZE
	(
		READWRITE(m_networkInfo);
		READWRITE(m_selfKey);
		READWRITE(m_selfRole);
	)

	std::vector< CValidNodeInfo > m_networkInfo;
	CPubKey m_selfKey;
	int m_selfRole;
};

struct CTransactionStatusResponse
{
	IMPLEMENT_SERIALIZE
	(
	READWRITE(m_status);
	READWRITE(m_transactionHash);
	)
	CTransactionStatusResponse(){}
	CTransactionStatusResponse(	int _status, uint256 const & _transactionHash ):m_status( _status ), m_transactionHash( _transactionHash ){}
	int m_status;
	uint256 m_transactionHash;
};

struct CTransactionAck
{
	IMPLEMENT_SERIALIZE
	(
	READWRITE(m_status);
	READWRITE(m_transaction);
	)
	CTransactionAck(){}
	CTransactionAck( int _status, CTransaction const & _transaction ):m_status(_status),m_transaction(_transaction){}

	int m_status;
	CTransaction m_transaction;
};

struct CTrackerSpecificStats
{
	IMPLEMENT_SERIALIZE
	(
	READWRITE(m_price);
	READWRITE(m_maxPrice);
	READWRITE(m_minPrice);
	)
	CTrackerSpecificStats(){};
	CTrackerSpecificStats( float _price, unsigned int _maxPrice, unsigned int _minPrice ):m_price( _price ), m_maxPrice( _maxPrice ), m_minPrice( _minPrice ){};

	float m_price;
	unsigned int m_maxPrice;
	unsigned int m_minPrice;
};

template < class _Stats >
struct CNodeSpecific : public _Stats
{
	CNodeSpecific( _Stats const & _stats, std::string const & _ip, uintptr_t _nodeIndicator ):_Stats( _stats ), m_ip( _ip ), m_nodeIndicator( _nodeIndicator ){}
	std::string m_ip;
	uintptr_t m_nodeIndicator;
};

class CSelfNode;

struct CConnectedNode
{
	CConnectedNode( common::CSelfNode * _node ):m_node( _node ){};
	common::CSelfNode * m_node;
};

struct CAckResult
{
};

struct CMessageResult
{
	CMessageResult( CMessage const & _message, uintptr_t _nodeIndicator, CPubKey const & _pubKey ): m_message( _message ),m_nodeIndicator( _nodeIndicator ), m_pubKey( _pubKey ){}

	CMessage m_message;
	uintptr_t m_nodeIndicator;
	CPubKey m_pubKey;
};


}

#endif // TRACKER_RESPONSES_H
