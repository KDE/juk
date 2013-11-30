/**
 * Copyright (c) 2003-2009 Mark Kretschmann <kretschmann@kde.org>
 * Copyright (c) 2005 Gabor Lehel <illissius@gmail.com>
 * Copyright (c) 2008 Dan Meltzer <parallelgrapefruit@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "slider.h"
#include "svghandler.h"

#include <KIcon>
#include <KLocale>
#include <KStandardDirs>
#include <KGlobalSettings>

#include <QAction>
#include <QContextMenuEvent>
#include <QFontMetrics>
#include <QMenu>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionSlider>

Slider::Slider( Qt::Orientation orientation, uint max, QWidget *parent )
    : QSlider( orientation, parent )
    , m_sliding( false )
    , m_outside( false )
    , m_prevValue( 0 )
    , m_needsResize( true )
{
    setMouseTracking( true );
    setRange( 0, max );
    setAttribute( Qt::WA_NoMousePropagation, true );
    setAttribute( Qt::WA_Hover, true );
    if ( orientation == Qt::Vertical )
    {
        setInvertedAppearance( true );
        setInvertedControls( true );
    }
}

QRect
Slider::sliderHandleRect( const QRect &slider, qreal percent ) const
{
    QRect rect;
    const bool inverse = ( orientation() == Qt::Horizontal ) ?
                         ( invertedAppearance() != (layoutDirection() == Qt::RightToLeft) ) :
                         ( !invertedAppearance() );

    if(m_usingCustomStyle)
        rect = The::svgHandler()->sliderKnobRect( slider, percent, inverse );
    else
    {
        if ( inverse )
            percent = 1.0 - percent;
        const int handleSize = style()->pixelMetric( QStyle::PM_SliderControlThickness );
        rect = QRect( 0, 0, handleSize, handleSize );
        rect.moveTo( slider.x() + qRound( ( slider.width() - handleSize ) * percent ), slider.y() + 1 );
    }

    return rect;
}

void
Slider::wheelEvent( QWheelEvent *e )
{
    if( orientation() == Qt::Vertical )
    {
        // Will be handled by the parent widget
        e->ignore();
        return;
    }

    // Position Slider (horizontal)
    // only used for progress slider now!
    int step = e->delta() * 24;
    int nval = value() + step;
    nval = qMax(nval, minimum());
    nval = qMin(nval, maximum());

    QSlider::setValue( nval );

    emit sliderReleased( value() );
}

void
Slider::mouseMoveEvent( QMouseEvent *e )
{
    if ( m_sliding )
    {
        //feels better, but using set value of 20 is bad of course
        QRect rect( -20, -20, width()+40, height()+40 );

        if ( orientation() == Qt::Horizontal && !rect.contains( e->pos() ) )
        {
            if ( !m_outside )
            {
                QSlider::setValue( m_prevValue );
                //if mouse released outside of slider, emit sliderMoved to previous value
                emit sliderMoved( m_prevValue );
            }
            m_outside = true;
        }
        else
        {
            m_outside = false;
            slideEvent( e );
            emit sliderMoved( value() );
        }
    }
    else
        QSlider::mouseMoveEvent( e );
}

void
Slider::slideEvent( QMouseEvent *e )
{
    QRect knob;
    if ( maximum() > minimum() )
        knob = sliderHandleRect( rect(), ((qreal)value()) / ( maximum() - minimum() ) );

    int position;
    int span;

    if( orientation() == Qt::Horizontal )
    {
        position = e->pos().x() - knob.width() / 2;
        span = width() - knob.width();
    }
    else
    {
        position = e->pos().y() - knob.height() / 2;
        span = height() - knob.height();
    }

    const bool inverse = ( orientation() == Qt::Horizontal ) ?
                         ( invertedAppearance() != (layoutDirection() == Qt::RightToLeft) ) :
                         ( !invertedAppearance() );
    const int val = QStyle::sliderValueFromPosition( minimum(), maximum(), position, span, inverse );
    QSlider::setValue( val );
}

void
Slider::mousePressEvent( QMouseEvent *e )
{
    m_sliding   = true;
    m_prevValue = value();

    QRect knob;
    if ( maximum() > minimum() )
        knob = sliderHandleRect( rect(), ((qreal)value()) / ( maximum() - minimum() ) );
    if ( !knob.contains( e->pos() ) )
        mouseMoveEvent( e );
}

void
Slider::mouseReleaseEvent( QMouseEvent* )
{
    if( !m_outside && value() != m_prevValue )
       emit sliderReleased( value() );

    m_sliding = false;
    m_outside = false;
}

void
Slider::setValue( int newValue )
{
    //don't adjust the slider while the user is dragging it!
    if ( !m_sliding || m_outside )
        QSlider::setValue( newValue );
    else
        m_prevValue = newValue;
}

void Slider::paintCustomSlider( QPainter *p )
{
    qreal percent = 0.0;
    if ( maximum() > minimum() )
        percent = ((qreal)value()) / ( maximum() - minimum() );
    QStyleOptionSlider opt;
    initStyleOption( &opt );
    if ( m_sliding ||
        ( underMouse() && sliderHandleRect( rect(), percent ).contains( mapFromGlobal(QCursor::pos()) ) ) )
    {
        opt.activeSubControls |= QStyle::SC_SliderHandle;
    }
    The::svgHandler()->paintCustomSlider( p, &opt, percent );
}

//////////////////////////////////////////////////////////////////////////////////////////
/// CLASS VolumeSlider
//////////////////////////////////////////////////////////////////////////////////////////

VolumeSlider::VolumeSlider( uint max, QWidget *parent, bool customStyle )
    : Slider( customStyle ? Qt::Horizontal : Qt::Vertical, max, parent )
{
    m_usingCustomStyle = customStyle;
    setFocusPolicy( Qt::NoFocus );
    setInvertedAppearance( false );
    setInvertedControls( false );
    connect( this, SIGNAL(sliderMoved(int)),
             this, SLOT(emitVolumeChanged(int)) );

    connect( this, SIGNAL(sliderReleased(int)),
             this, SLOT(emitVolumeChanged(int)) );
}

void
VolumeSlider::mousePressEvent( QMouseEvent *e )
{
    if( e->button() != Qt::RightButton )
    {
        Slider::mousePressEvent( e );
        slideEvent( e );
    }
}

void
VolumeSlider::contextMenuEvent( QContextMenuEvent *e )
{
    QMenu menu;
    menu.setTitle(   i18n( "Volume" ) );
    menu.addAction(  i18n(   "100%" ) )->setData( 100 );
    menu.addAction(  i18n(    "80%" ) )->setData(  80 );
    menu.addAction(  i18n(    "60%" ) )->setData(  60 );
    menu.addAction(  i18n(    "40%" ) )->setData(  40 );
    menu.addAction(  i18n(    "20%" ) )->setData(  20 );
    menu.addAction(  i18n(     "0%" ) )->setData(   0 );

    QAction* a = menu.exec( mapToGlobal( e->pos() ) );
    if( a )
    {
        const int n = a->data().toInt();
        if( n >= 0 )
        {
            QSlider::setValue( n );
            emit volumeChanged( float( n ) / float( maximum() ) );
        }
    }
}

void
VolumeSlider::wheelEvent( QWheelEvent *e )
{
    static const int volumeSensitivity = 30;
    const uint step = e->delta() / volumeSensitivity;
    QSlider::setValue( QSlider::value() + step );

    emit volumeChanged( float( value() ) / float( maximum() ) );
}

void
VolumeSlider::paintEvent( QPaintEvent *event )
{
    if( m_usingCustomStyle )
    {
        QPainter p( this );
        paintCustomSlider( &p );
        p.end();
        return;
    }

    QSlider::paintEvent( event );
}

void
VolumeSlider::emitVolumeChanged( int value )
{
    emit volumeChanged( float( value ) / float( maximum() ) );
}


//////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// TIMESLIDER ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

TimeSlider::TimeSlider( QWidget *parent )
    : Slider( Qt::Horizontal, 0, parent )
    , m_knobX( 0.0 )
{
    m_usingCustomStyle = true;
    setFocusPolicy( Qt::NoFocus );
}

void
TimeSlider::setSliderValue( int value )
{
    Slider::setValue( value );
}

void
TimeSlider::paintEvent( QPaintEvent *pe )
{
    QPainter p( this );
    p.setClipRegion( pe->region() );
    paintCustomSlider( &p );
    p.end();

}

void TimeSlider::sliderChange( SliderChange change )
{
    if ( change == SliderValueChange || change == SliderRangeChange )
    {
        int oldKnobX = m_knobX;
        qreal percent = 0.0;
        if ( maximum() > minimum() )
            percent = ((qreal)value()) / ( maximum() - minimum() );
        QRect knob = sliderHandleRect( rect(), percent );
        m_knobX = knob.x();

        if (oldKnobX < m_knobX)
            update( oldKnobX, knob.y(), knob.right() + 1 - oldKnobX, knob.height() );
        else if (oldKnobX > m_knobX)
            update( m_knobX, knob.y(), oldKnobX + knob.width(), knob.height() );
    }
    else
        Slider::sliderChange( change ); // calls update()
}

void TimeSlider::mousePressEvent( QMouseEvent *event )
{
    // We should probably eat this event if we're not able to seek
    Slider::mousePressEvent( event );
}

#include "slider.moc"
