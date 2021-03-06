// Copyright (c) 2014-2015 DiMS dev-team
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "testApplicaionWidgets.h"

CTextLine::CTextLine( QString const & _title, QString const & _unit, QWidget* _parent )
{
	QHBoxLayout * layout = new QHBoxLayout;
	setLayout ( layout );

	layout->addWidget( new QLabel( _title ) );

	m_spinBox = new QDoubleSpinBox;

	m_spinBox->setSingleStep( 0.1 );

	m_spinBox->setMaximum( (unsigned long long)-1 );

	m_spinBox->setDecimals(4);

	m_spinBox->setValue(0);

	layout->addWidget( m_spinBox );

	layout->addWidget( new QLabel( " " + _unit ) );

	QObject::connect( m_spinBox, SIGNAL( valueChanged ( double ) ),
					this, SLOT( valueChangedSlot( double ) ) );
}

double
CTextLine::getValue() const
{
	return m_spinBox->value();
}

void
CTextLine::valueChangedSlot( double value )
{
	emit changed();
}

CSphereWidget::CSphereWidget(QWidget* _parent ):QWidget( _parent )
{
	QVBoxLayout * layout = new QVBoxLayout;
	setLayout ( layout );
	m_radius = new CTextLine( "Radius", "m" );

	m_density = new CTextLine( "Density", QString( "g/cm" ) + QChar(0xB3) );

	layout->addWidget( m_radius );
	layout->addWidget( m_density );

	layout->addWidget( new QLabel( "Resulting mass:" ) );

	QObject::connect( m_radius, SIGNAL( changed() ), this, SLOT( evaluateResult() ) );

	QObject::connect( m_density, SIGNAL( changed() ), this, SLOT( evaluateResult() ) );

	m_result = new QLabel;

	layout->addWidget( m_result );
	evaluateResult();
}

void
CSphereWidget::evaluateResult()
{
	double volume = (4.0/3.0)* pow( m_radius->getValue(), 3.0) * M_PI;

	m_result->setText( QString().setNum( volume * m_density->getValue() * 1000 ) + QString(" kg") );
}

CCuboidWidget::CCuboidWidget( QWidget* _parent )
	: QWidget( _parent )
{
	QVBoxLayout * layout = new QVBoxLayout;
	setLayout ( layout );

	m_x = new CTextLine( "Width", "m" );
	m_y = new CTextLine( "Height", "m" );
	m_z = new CTextLine( "Depth", "m" );

	m_density = new CTextLine( "Density" , QString( "g/cm" ) + QChar(0xB3) );

	layout->addWidget( m_x );
	layout->addWidget( m_y );
	layout->addWidget( m_z );
	layout->addWidget( m_density );

	layout->addWidget( new QLabel( "Resulting mass:" ) );

	QObject::connect( m_x, SIGNAL( changed() ), this, SLOT( evaluateResult() ) );
	QObject::connect( m_y, SIGNAL( changed() ), this, SLOT( evaluateResult() ) );
	QObject::connect( m_z, SIGNAL( changed() ), this, SLOT( evaluateResult() ) );
	QObject::connect( m_density, SIGNAL( changed() ), this, SLOT( evaluateResult() ) );

	m_result = new QLabel;

	evaluateResult();

	layout->addWidget( m_result );
}


void
CCuboidWidget::evaluateResult()
{
	double volume = m_x->getValue() * m_y->getValue() * m_z->getValue();

	m_result->setText( QString::number(volume * m_density->getValue()* 1000.0, 'g', 10) + QString(" kg") );
}

CMainWidget::CMainWidget(QWidget* parent ):QTabWidget(parent)
{
	setParent(parent);
	addTab(new CSphereWidget(),"Sphere");
	addTab(new CCuboidWidget(),"Cuboid");
}
