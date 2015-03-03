// Copyright (c) 2014-2015 Dims dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "networkClient.h"
#include "serialize.h"
#include "version.h"
#include "common/requestHandler.h"
#include "sendBalanceInfoAction.h"
#include "sendTransactionAction.h"
#include "sendInfoRequestAction.h"
#include "clientRequests.h"

#include "common/support.h"


#include <QHostAddress>

namespace client
{

unsigned const
CNetworkClient::m_timeout = 5000;

CNetworkClient::CNetworkClient(QString const  &_ipAddr, ushort const _port )
	: m_ip( _ipAddr )
	, m_port( _port )
	, m_sleepTime(100)
{
	m_socket = new QTcpSocket(this);

	m_pushStream = new CBufferAsStream( (char*)m_pushBuffer.m_buffer, MaxBufferSize, SER_NETWORK, CLIENT_VERSION);
	m_pushStream->SetPos(0);
	startThread();
}

int CNetworkClient::waitForInput()
{
	int bytesAvail = 0;

	if( m_socket->state() == QAbstractSocket::ConnectedState && m_socket->bytesAvailable () )
		bytesAvail = m_socket->bytesAvailable();

	return bytesAvail;
}

unsigned int
CNetworkClient::read()
{
	int bytesAvail = waitForInput();
	m_pullBuffer.m_usedSize = 0;

	if (bytesAvail > 0)
	{
		bool endOfLine = false;
		bool endOfStream = false;

		if ( MaxBufferSize <= bytesAvail )
		{
			//throw error
			return 0; // this is  error so handle it as such  at some point
		}

		while ( m_pullBuffer.m_usedSize < bytesAvail && (!endOfLine) && (!endOfStream) )
		{
			m_pullBuffer.m_usedSize += m_socket->read( m_pullBuffer.m_buffer + m_pullBuffer.m_usedSize, MaxBufferSize - m_pullBuffer.m_usedSize );

		}
	}
	return m_pullBuffer.m_usedSize;
}


void
CNetworkClient::write()
{
	if (m_pushBuffer.m_usedSize > 0)
	{
		m_socket->write( m_pushBuffer.m_buffer, m_pushBuffer.m_usedSize );

		if (! m_socket->waitForBytesWritten())
		{
			// error handle it somehow
		}

		m_pushBuffer.m_usedSize = 0;
	}
}

common::CRequest< ClientResponses >*
CNetworkClient::takeMatching( uint256 const & _token )
{
	std::map< uint256, common::CRequest< ClientResponses >* >::iterator iterator
			= m_matching.find( _token );

	common::CRequest< ClientResponses >* request;

	if( iterator != m_matching.end() )
	{
		request = iterator->second;
		m_matching.erase( iterator );
	}
	else
	{
		assert( !"can't be here" );
		return 0;
	}
	return request;
}

bool
CNetworkClient::processSomething() const
{
	return !m_matching.empty() || !m_workingRequest.empty();
}

void CNetworkClient::startThread()
{
	start();
}


void CNetworkClient::run()
{
	while(1)
	{
		{
			QMutexLocker lock( &m_mutex );
			QHostAddress hostAddr( m_ip );

			if ( !processSomething() )
			{
				m_socket->disconnectFromHost();
				continue;
			}
			if ( !m_socket->isOpen () )
				m_socket->connectToHost( hostAddr, m_port );

			if ( m_socket->waitForConnected( m_timeout ) )
			{
				write();
				read();
			}
			else
			{
				assert( !"hadle this" );
			}
		}
		MilliSleep( m_sleepTime );
	}
}

bool
CNetworkClient::serviced() const throw(common::CMediumException)
{
	return m_pullBuffer.m_usedSize > 0;
}


void 
CNetworkClient::add( common::CRequest< ClientResponses > const * _request )
{
}

void
CNetworkClient::add( CBalanceRequest const * _request )
{
	QMutexLocker lock( &m_mutex );
	serializeEnum(*m_pushStream, common::CMainRequestType::BalanceInfoReq );
	*m_pushStream << _request->m_address;

	m_workingRequest.push_back( ( common::CRequest< ClientResponses >* )_request );
}

void
CNetworkClient::add( CTransactionSendRequest const * _request )
{
	QMutexLocker lock( &m_mutex );
	serializeEnum(*m_pushStream, common::CMainRequestType::Transaction );
	*m_pushStream  << _request->m_transaction;

	m_workingRequest.push_back( ( common::CRequest< ClientResponses >* )_request );
}

void
CNetworkClient::add( CTransactionStatusRequest const * _request )
{
	common::serializeEnum( *m_pushStream, common::CMainRequestType::TransactionStatusReq );

	*m_pushStream << _request->m_transactionHash;
	m_workingRequest.push_back( ( common::CRequest< ClientResponses >* )_request );
}

void
CNetworkClient::add( CInfoRequestContinue const * _request )
{
	common::serializeEnum( *m_pushStream, common::CMainRequestType::ContinueReq );

	*m_pushStream << _request->m_token;
	m_workingRequest.push_back( ( common::CRequest< ClientResponses >* )_request );
}

void
CNetworkClient::add( CRecognizeNetworkRequest const * _request )
{
	common::serializeEnum( *m_pushStream, common::CMainRequestType::NetworkInfoReq );
	m_workingRequest.push_back( ( common::CRequest< ClientResponses >* )_request );
}

void
CNetworkClient::add( CTrackersInfoRequest const * _request )
{
	common::serializeEnum( *m_pushStream, common::CMainRequestType::TrackerInfoReq );
	m_workingRequest.push_back( ( common::CRequest< ClientResponses >* )_request );
}

void
CNetworkClient::add( CMonitorInfoRequest const * _request )
{
	common::serializeEnum( *m_pushStream, common::CMainRequestType::MonitorInfoReq );
	m_workingRequest.push_back( ( common::CRequest< ClientResponses >* )_request );
}

bool
CNetworkClient::flush()
{
	QMutexLocker lock( &m_mutex );
	m_pushBuffer.m_usedSize = m_pushStream->GetPos();

	if ( !m_pushBuffer.m_usedSize )
		return true;
	m_pushStream->SetPos( 0 );

	return true;
}

void
CNetworkClient::clearResponses()
{
	m_pullBuffer.m_usedSize = 0;
}

bool
CNetworkClient::getResponseAndClear( std::multimap< common::CRequest< ClientResponses >const*, ClientResponses > & _requestResponse )
{
	QMutexLocker lock( &m_mutex );
	CBufferAsStream stream(
				(char*)m_pullBuffer.m_buffer
				, m_pullBuffer.m_usedSize
				, SER_NETWORK
				, CLIENT_VERSION);

	uint256 token;

	while( !stream.eof() )
	{
		int messageType;

		stream >> messageType;

		if ( messageType == common::CMainRequestType::TransactionStatusReq )
		{
			common::CTransactionStatusResponse transactionStatus;
			stream >> token;
			stream >> transactionStatus;

			_requestResponse.insert(
						std::make_pair(
							takeMatching( token ),
							common::CTransactionStatus( ( common::TransactionsStatus::Enum )transactionStatus.m_status, transactionStatus.m_transactionHash, transactionStatus.m_signedHash ) ) );

		}
		else if ( messageType == common::CMainRequestType::Transaction )
		{
			common::CTransactionAck transactionAck;
			stream >> token;
			stream >> transactionAck;

			_requestResponse.insert( std::make_pair( takeMatching( token ), transactionAck ) );
		}
		else if ( messageType == common::CMainRequestType::MonitorInfoReq )
		{
			common::CMonitorData monitorData;
			stream >> token;
			stream >> monitorData;

			common::CNodeSpecific< common::CMonitorData > stats( monitorData, m_ip.toStdString(), common::convertToInt(this));

			_requestResponse.insert( std::make_pair( takeMatching( token ), stats ) );
		}
		else if ( messageType == common::CMainRequestType::TrackerInfoReq )
		{
			common::CTrackerSpecificStats trackerSpecificStats;
			stream >> token;
			stream >> trackerSpecificStats;

			common::CNodeSpecific< common::CTrackerSpecificStats > stats( trackerSpecificStats, m_ip.toStdString(), common::convertToInt(this));

			_requestResponse.insert( std::make_pair( takeMatching( token ), stats ) );
		}
		else if ( messageType == common::CMainRequestType::RequestSatatusReq )
		{
		}
		else if ( messageType == common::CMainRequestType::BalanceInfoReq )
		{
			common::CAvailableCoins availableCoins;
			stream >> token;
			stream >> availableCoins;
			_requestResponse.insert( std::make_pair( takeMatching( token ), availableCoins ) );
		}
		else if ( messageType == common::CMainRequestType::NetworkInfoReq )
		{
			common::CClientNetworkInfoResult networkResult;
			stream >> token;
			stream >> networkResult;

			common::CNodeSpecific< common::CClientNetworkInfoResult > stats( networkResult, m_ip.toStdString(), common::convertToInt(this));

			_requestResponse.insert( std::make_pair( takeMatching( token ), stats ) );
		}
		else if ( messageType == common::CMainRequestType::ContinueReq )
		{
			stream >> token;
			_requestResponse.insert( std::make_pair( m_workingRequest.front(), common::CPending( token, common::convertToInt(this) ) ) );
			m_matching.insert( std::make_pair( token, m_workingRequest.front() ) );
			//ok only one  response  for one  request, right now
			m_workingRequest.erase( m_workingRequest.begin() );
		}
		else
		{
			throw;
		}

		clearResponses();
	}
}

CNetworkClient::~CNetworkClient()
{
	//  is this right??, maybe invent some other  way  to  terminate?
	terminate();
	delete m_pushStream;
}

}
