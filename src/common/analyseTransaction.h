// Copyright (c) 2014-2015 Dims dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ANALYSE_TRANSACTION_H
#define ANALYSE_TRANSACTION_H

class CTransaction;
class CTxOut;
class CKeyID;

struct CAvailableCoin;

namespace common
{

bool findOutputInTransaction( CTransaction const & _tx, CKeyID const & _findId , CTxOut & _txout, unsigned int & _id );

std::vector< CAvailableCoin > getAvailableCoins( CCoins const & _coins, uint160 const & _pubId, uint256 const & _hash );

bool findKeyInInputs( CTransaction const & _tx, CKeyID const & _keyId );
}

#endif // ANALYSE_TRANSACTION_H
