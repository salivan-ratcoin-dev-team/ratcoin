#ifndef TRACKER_REQUESTS_LIST_H
#define TRACKER_REQUESTS_LIST_H

#include "getBalanceRequest.h"

namespace common
{
template < class RequestType >
class CIdentifyRequest;

template < class RequestType >
class CContinueReqest;

template < class RequestType >
class CIdentifyResponse;

template < class RequestType >
class CNetworkRoleRequest;

template < class RequestType >
class CKnownNetworkInfoRequest;

template < class RequestType >
class CAckRequest;
}

namespace tracker
{

class CValidateTransactionsRequest;
class CConnectToTrackerRequest;
class CAskForTransactionsRequest;
class CSetBloomFilterRequest;
class CGetSynchronizationInfoRequest;
class CSynchronizationAssistanceRequest;
class CGetNextBlockRequest;
class CSetNextBlockRequest;
class CTransactionsKnownRequest;
class CTransactionsAckRequest;
class CTransactionsDoublespendRequest;
class CTransactionsNotOkRequest;

typedef boost::mpl::list< CGetBalanceRequest, CValidateTransactionsRequest, CConnectToTrackerRequest, common::CIdentifyRequest<TrackerResponses>, common::CIdentifyResponse<TrackerResponses>, common::CContinueReqest<TrackerResponses>, CAskForTransactionsRequest, CSetBloomFilterRequest, CGetSynchronizationInfoRequest, CSynchronizationAssistanceRequest, CGetNextBlockRequest, CSetNextBlockRequest, CTransactionsKnownRequest, CTransactionsAckRequest, CTransactionsDoublespendRequest, CTransactionsNotOkRequest, common::CNetworkRoleRequest< TrackerResponses >, common::CKnownNetworkInfoRequest< TrackerResponses >, common::CAckRequest< TrackerResponses > > TrackerRequestsList;

}

#endif // TRACKER_REQUESTS_LIST_H
