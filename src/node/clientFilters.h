#ifndef CLIENT_FILTERS_H
#define CLIENT_FILTERS_H

#include "common/filters.h"
#include "configureNodeActionHadler.h"

namespace  client
{

struct CMediumClassFilter : public common::CMediumFilter< NodeResponses >
{
	CMediumClassFilter( int _mediumClass, int _mediumNumber = -1 ):
		m_mediumClass( _mediumClass ),
		m_mediumNumber( _mediumNumber )
	{}

	std::list< common::CMedium< NodeResponses > *> getMediums( CSettingsConnectionProvider * _settingsMedium )
	{
		/*
		std::list< CMedium< _RequestResponses > *> mediums;
		mediums = _trackerNodesManager->getNodesByClass( m_mediumClass );

		if ( m_mediumNumber != -1 && mediums.size() > m_mediumNumber )
		{
			mediums.resize();
		}
		return mediums;*/
	}

	std::list< common::CMedium< NodeResponses > *> getMediums( client::CTrackerLocalRanking * _trackerLocalRanking )
	{
		/*
		return getMediumByClass( m_mediumClass, m_mediumNumber );
		*/
	}

	int m_mediumClass;
	int m_mediumNumber;
};

struct CSpecificMediumFilter : public common::CMediumFilter< NodeResponses >
{
	CSpecificMediumFilter( long long unsigned _ptr )
	: m_ptr( _ptr )
	{}

	std::list< common::CMedium< NodeResponses > *> getMediums( client::CTrackerLocalRanking * _trackerLocalRanking )
	{
		std::list< common::CMedium< NodeResponses > *> mediums;

		return mediums;
	}
	long long unsigned m_ptr;
};


}

#endif // CLIENT_FILTERS_H
