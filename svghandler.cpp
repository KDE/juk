/**
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>
 * Copyright (c) 2008 Jeff Mitchell <kde-dev@emailgoeshere.com>
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>
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

#include "svghandler.h"
#include "juk_debug.h"

#include <QPainter>
#include <QReadLocker>
#include <QStandardPaths>
#include <QStyleOptionSlider>
#include <QSvgRenderer>
#include <QWriteLocker>

namespace The {
    static SvgHandler* s_SvgHandler_instance = nullptr;

    SvgHandler* svgHandler()
    {
        if( !s_SvgHandler_instance )
            s_SvgHandler_instance = new SvgHandler();

        return s_SvgHandler_instance;
    }
}


SvgHandler::SvgHandler( QObject* parent )
    : QObject( parent )
    , m_themeFile( "juk/pics/theme.svg" )
{
}

SvgHandler::~SvgHandler()
{
    The::s_SvgHandler_instance = nullptr;
}


bool SvgHandler::loadSvg( const QString& name )
{
    const QString &svgFilename =
        QStandardPaths::locate( QStandardPaths::GenericDataLocation, name );
    QSvgRenderer *renderer = new QSvgRenderer( svgFilename, this );

    if ( !renderer->isValid() )
    {
        delete renderer;
        return false;
    }
    QWriteLocker writeLocker( &m_lock );
    m_renderer.reset(renderer);

    return true;
}

QPixmap SvgHandler::renderSvg( const QString& keyname,
                               int width,
                               int height,
                               const QString& element )
{
    QString key = keyname % QChar( ':' ) % QString::number( width ) %
        QChar( 'x' ) % QString::number( height );

    QPixmap pixmap;
    if( auto it = m_cache.constFind(key); it != m_cache.constEnd() )
    {
        pixmap = *it;
    }
    else
    {
        pixmap = QPixmap( width * dpr, height * dpr );
        pixmap.setDevicePixelRatio(dpr);
        pixmap.fill( Qt::transparent );

        QReadLocker readLocker( &m_lock );
        if( ! m_renderer )
        {
            readLocker.unlock();
            if( !loadSvg( m_themeFile ) )
            {
                return pixmap;
            }
            readLocker.relock();
        }

        QPainter pt( &pixmap );
        if ( element.isEmpty() )
            m_renderer->render( &pt, QRectF( 0, 0, width, height ) );
        else {
            m_renderer->render( &pt, element, QRectF( 0, 0, width, height ) );
        }

        m_cache.insert( key, pixmap );
    }

    return pixmap;
}

void SvgHandler::reTint()
{
    // The::svgTinter()->init();
    if ( !loadSvg( m_themeFile ))
        qCDebug(JUK_LOG) << "Unable to load theme file: " << m_themeFile;
    emit retinted();
}

QString SvgHandler::themeFile()
{
    return m_themeFile;
}

QRectF SvgHandler::sliderKnobRect( const QRectF &slider, qreal percent, bool inverse ) const
{
    if ( inverse )
        percent = 1.0 - percent;
    const int knobSize = slider.height() - 4;
    QRectF ret( 0, 0, knobSize, knobSize );
    ret.moveTo( slider.x() + qRound( ( slider.width() - knobSize ) * percent ), slider.y() + 1 );
    return ret;
}

// Experimental, using a mockup from Nuno Pinheiro (new_slider_nuno)
void SvgHandler::paintCustomSlider( QPainter *p, QStyleOptionSlider *slider, qreal percentage )
{
    qreal sliderHeight = slider->rect.height() - 6;
    const bool inverse = ( slider->orientation == Qt::Vertical ) ? slider->upsideDown :
                         ( (slider->direction == Qt::RightToLeft) != slider->upsideDown );
    QRectF knob = sliderKnobRect( slider->rect, percentage, inverse );
    QPointF pt = slider->rect.topLeft() + QPointF( 0, 2 );

    //debug() << "rel: " << knobRelPos << ", width: " << width << ", height:" << height << ", %: " << percentage;

    // Draw the slider background in 3 parts

    p->drawPixmap( pt, renderSvg( "progress_slider_left", sliderHeight, sliderHeight, "progress_slider_left" ) );

    pt.rx() += sliderHeight;
    QRectF midRect(pt, QSize(slider->rect.width() - sliderHeight * 2, sliderHeight) );
    p->drawTiledPixmap( midRect, renderSvg( "progress_slider_mid", 32, sliderHeight, "progress_slider_mid" ) );

    pt = midRect.topRight() + QPoint( 1, 0 );
    p->drawPixmap( pt, renderSvg( "progress_slider_right", sliderHeight, sliderHeight, "progress_slider_right" ) );

    //draw the played background.

    qreal playedBarHeight = sliderHeight - 6;
    qreal min = 0;

    qreal sizeOfLeftPlayed = qBound( min, inverse ? slider->rect.right() - knob.right() + 2 :
                                   knob.x() - 2, playedBarHeight );

    if( sizeOfLeftPlayed > 0 )
    {
        QPointF tl, br;
        if ( inverse )
        {
            tl = knob.topRight() + QPointF( -5, 5 ); // 5px x padding to avoid a "gap" between it and the top and bottom of the round knob.
            br = slider->rect.topRight() + QPointF( -3, 5 + playedBarHeight - 1 );
            QPixmap rightEnd = renderSvg( "progress_slider_played_right", playedBarHeight, playedBarHeight, "progress_slider_played_right" );
            p->drawPixmap( br.x() - rightEnd.width() + 1, tl.y(), rightEnd, qMax(qreal(0), rightEnd.width() - (sizeOfLeftPlayed + 3)), 0, sizeOfLeftPlayed + 3, playedBarHeight );
            br.rx() -= playedBarHeight;
        }
        else
        {
            tl = slider->rect.topLeft() + QPointF( 3, 5 );
            br = QPointF( knob.x() + 5, tl.y() + playedBarHeight - 1 );
            QPixmap leftEnd = renderSvg( "progress_slider_played_left", playedBarHeight, playedBarHeight, "progress_slider_played_left" );
            p->drawPixmap( tl.x(), tl.y(), leftEnd, 0, 0, sizeOfLeftPlayed + 3, playedBarHeight );
            tl.rx() += playedBarHeight;
        }
        if ( sizeOfLeftPlayed == playedBarHeight )
            p->drawTiledPixmap( QRectF(tl, br), renderSvg( "progress_slider_played_mid", 32, playedBarHeight, "progress_slider_played_mid" ) );

    }

    if ( slider->state & QStyle::State_Enabled )
    {   // Draw the knob (handle)
        const char *string = ( slider->activeSubControls & QStyle::SC_SliderHandle ) ?
                             "slider_knob_200911_active" : "slider_knob_200911";
        p->drawPixmap( knob.topLeft(), renderSvg( string, knob.width(), knob.height(), string ) );
    }
}

void SvgHandler::setDevicePixelRatioF(qreal dpr)
{
    this->dpr = dpr;
}
