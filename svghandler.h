/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2008 Jeff Mitchell <kde-dev@emailgoeshere.com>                         *
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>                            *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef SVGHANDLER_H
#define SVGHANDLER_H

class QStyleOptionSlider;

#include <KPixmapCache>
#include <QReadWriteLock>
#include <QSvgRenderer>

#include <QPixmap>
#include <QString>

class SvgHandler;

namespace The {
    SvgHandler* svgHandler();
}

/**
A class to abstract out some common operations of users of tinted svgs
*/
class SvgHandler : public QObject
{
    Q_OBJECT

    friend SvgHandler* The::svgHandler();

    public:
        ~SvgHandler();

        QSvgRenderer* getRenderer( const QString &name );
        QSvgRenderer* getRenderer();
        /**
        * Overloaded function that uses the current theme
        * @param keyname the name of the key to save in the cache
        * @param width Width of the resulting pixmap
        * @param height Height of the resulting pixmap
        * @param element The theme element to render ( if none the entire svg is rendered )
        * @param skipCache If true, the pixmap will always get rendered and never fetched from the cache.
        * @return The svg element/file rendered into a pixmap
        */
        QPixmap renderSvg( const QString& keyname, int width, int height, const QString& element = QString(), bool skipCache = false );
        /**
         * Paint a custom slider using the specified painter. The slider consists
         * of a background part, a "knob" that moves along it to show the current
         * position, and 2 end markers to clearly mark the ends of the slider.
         * The background part before the knob, is painted in a different color than the
         * part after (and under) the knob.
         * @param p The painter to use.
         * @param x The x position to begin painting at.
         * @param y The y position to begin painting at.
         * @param width The width of the slider to paint.
         * @param height The height of the slider. The background part does not scale in height, it will always be a relatively thin line, but the knob and end markers do.
         * @param percentage The percentange of the slider that the knob is positioned at.
         * @param active Specifies whether the slider should be painted "active" using the current palettes active colors, to specify that it currently has mouse focus or hover.
         */
        void paintCustomSlider( QPainter *p, QStyleOptionSlider *slider, qreal percentage );

        /**
         * Calculate the visual slider knob rect from its value, use it instead the QStyle functions
         * QStyle::sliderPositionFromValue() and QStyle::subControlRect();
         */
        QRect sliderKnobRect( const QRect &slider, qreal percent, bool inverse ) const;

    private:
        SvgHandler( QObject* parent = 0 );

        bool loadSvg( const QString& name );

        QPixmap sliderHandle( const QColor &color, bool pressed, int size );
        QColor calcLightColor(const QColor &color) const;
        QColor calcDarkColor(const QColor &color) const;
        bool lowThreshold(const QColor &color) const;

        KPixmapCache * m_cache;

        QHash<QString,QSvgRenderer*> m_renderers;
        QReadWriteLock m_lock;
        bool m_customTheme;
};

#endif
