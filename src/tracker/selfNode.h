// Copyright (c) 2014 Ratcoin dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SELF_NODE_H
#define SELF_NODE_H

#include "net.h"
#include "key.h"

namespace tracker
{

class CSelfNode : public CNode
{
public:
	CSelfNode(SOCKET hSocketIn, CAddress addrIn, std::string addrNameIn = "", bool fInboundIn=false):CNode( hSocketIn, addrIn, addrNameIn, fInboundIn ){};
	CKeyID getPubKeyId();
private:
	CPubKey m_pubKey;
};

}

#endif
