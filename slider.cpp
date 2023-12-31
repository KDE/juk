/**
 * Copyright (c) 2003-2009 Mark Kretschmann <kretschmann@kde.org>
 * Copyright (c) 2005 Gabor Lehel <illissius@gmail.com>
 * Copyright (c) 2008 Dan Meltzer <parallelgrapefruit@gmail.com>
 * Copyright (c) 2021 Michael Pyne <mpyne@kde.org>
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

#include <KLocalizedString>

#include <QAction>
#include <QContextMenuEvent>
#include <QMenu>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionSlider>

#include "playermanager.h"
#include "svghandler.h"

Slider::Slider( Qt::Orientation orientation, uint max, QWidget *parent )
    : QSlider( orientation, parent )
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

QRectF
Slider::sliderHandleRect( const QRectF &slider, qreal percent ) const
{
    QRectF rect;
    const bool inverse = ( orientation() == Qt::Horizontal ) ?
                         ( invertedAppearance() != (layoutDirection() == Qt::RightToLeft) ) :
                         ( !invertedAppearance() );

    if(m_usingCustomStyle)
    {
        rect = The::svgHandler()->sliderKnobRect( slider, percent, inverse );
    }
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
    int step = e->angleDelta().y() * 24;
    int nval = value() + step;
    nval = qMax(nval, minimum());
    nval = qMin(nval, maximum());

    QSlider::setValue( nval );
}

void Slider::paintCustomSlider( QPainter *p )
{
    qreal percent = 0.0;
    if ( maximum() > minimum() )
        percent = ((qreal)value()) / ( maximum() - minimum() );
    QStyleOptionSlider opt;
    initStyleOption( &opt );
    if ( this->isSliderDown() ||
        ( underMouse() && sliderHandleRect( rect(), percent ).contains( mapFromGlobal(QCursor::pos()) ) ) )
    {
        opt.activeSubControls |= QStyle::SC_SliderHandle;
    }
    The::svgHandler()->setDevicePixelRatioF(devicePixelRatioF());
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

    connect(this, &QAbstractSlider::valueChanged, this, &VolumeSlider::emitVolumeChanged);
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
            this->setValue( n );
        }
    }
}

void
VolumeSlider::wheelEvent( QWheelEvent *e )
{
    static const int volumeSensitivity = 30;
    const uint step = e->angleDelta().y() / volumeSensitivity;
    this->setValue( this->value() + step );
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

    Slider::paintEvent( event );
}

void
VolumeSlider::emitVolumeChanged( int value )
{
    emit volumeChanged( float( value ) / float( maximum() ) );
}


//////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// TIMESLIDER ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

TimeSlider::TimeSlider( QWidget *parent, PlayerManager *player )
    : Slider( Qt::Horizontal, 0, parent )
    , m_player( player )
    , m_knobX( 0.0 )
{
    m_usingCustomStyle = true;
    setFocusPolicy( Qt::NoFocus );
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
        QRectF knob = sliderHandleRect( rect(), percent );
        m_knobX = knob.x();

        if (oldKnobX < m_knobX)
            update( oldKnobX, knob.y(), knob.right() + 1 - oldKnobX, knob.height() );
        else if (oldKnobX > m_knobX)
            update( m_knobX, knob.y(), oldKnobX + knob.width(), knob.height() );
    }
    else
        Slider::sliderChange( change ); // calls update()
}

void TimeSlider::showEvent(QShowEvent *)
{
    connect(m_player, &PlayerManager::tick, this, &TimeSlider::setValue);
}

void TimeSlider::hideEvent(QHideEvent *)
{
    disconnect(m_player);
}
