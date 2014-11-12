// This autogenerated skeleton file illustrates how to build a server.
// You should copy it to another filename to avoid overwriting it.

#include "gen-cpp/MonitorsScaner.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

#include "informationProvider.h"

#include "common/actionHandler.h"
#include "node/configureNodeActionHadler.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

using namespace  ::monitorsScaner;


class MonitorsScanerHandler : virtual public MonitorsScanerIf
{
 public:
  MonitorsScanerHandler()
  {}

  void getInfo(Data& _return, const InfoRequest& infoRequest)
  {
	m_informationProvider.getInfo( _return, infoRequest );
  }

CInforamtionProvider m_informationProvider;
};


template<>
unsigned int const common::CActionHandler< client::NodeResponses >::m_sleepTime = 100;
template<>
common::CActionHandler< client::NodeResponses > * common::CActionHandler< client::NodeResponses >::ms_instance = NULL;

/*
template<>
common::CPeriodicActionExecutor< client::NodeResponses > * common::CPeriodicActionExecutor< client::NodeResponses >::ms_instance = NULL;

template<>
unsigned int const common::CPeriodicActionExecutor< client::NodeResponses >::m_sleepTime = 100;
*/
void
init()
{
	common::CActionHandler< client::NodeResponses >::getInstance();

}


int main(int argc, char **argv) {
  init();

	int port = 9090;
  shared_ptr<MonitorsScanerHandler> handler(new MonitorsScanerHandler());
  shared_ptr<TProcessor> processor(new MonitorsScanerProcessor(handler));
  shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
  shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
  shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());



  boost::thread switchStorageThread( boost::bind( &CInforamtionProvider::changeStorageThread, &handler->m_informationProvider ) );

  TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
  server.serve();

  return 0;
}

