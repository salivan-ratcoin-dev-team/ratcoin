// Copyright (c) 2014 Ratcoin dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef VISITOR_CONFIGURATION_UTILITIES_H
#define VISITOR_CONFIGURATION_UTILITIES_H

#include <boost/mpl/list.hpp>
#include <boost/mpl/less.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/if.hpp>

#define VisitorParam( num )  if_< less< int_< num >, size< ParametersList >::type >::type, at< ParametersList ,if_< less< int_< num >, size< ParametersList >::type >::type, int_< num >, int_< 0 > >::type >::type ,at< DummyList ,int_< num > >::type  >::type

namespace common
{

typedef ParametersList this_have_to_be_defined; // mpl list  ParametersList have to be  defined till here

using namespace boost::mpl;

struct D1{};struct D2{};struct D3{};struct D4{};struct D5{};struct D6{};struct D7{};struct D8{};struct D9{};struct D10{};
struct D11{};struct D12{};struct D13{};struct D14{};struct D15{};struct D16{};struct D17{};struct D18{};struct D19{};struct D20{};

typedef list< D1,D2,D3,D4,D5,D6,D7,D8,D9,D10,D11,D12,D13,D14,D15,D16,D17,D18,D18,D20 >  DummyList;

}

#endif // VISITOR_CONFIGURATION_UTILITIES_H
