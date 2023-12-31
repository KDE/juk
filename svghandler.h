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

#ifndef SVGHANDLER_H
#define SVGHANDLER_H

class QStyleOptionSlider;

#include <QMap>
#include <QScopedPointer>
#include <QReadWriteLock>

#include <QPixmap>
#include <QString>
#include <QObject>

class SvgHandler;
class QSvgRenderer;

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

        /**
        * Overloaded function that uses the current theme
        * @param keyname the name of the key to save in the cache
        * @param width Width of the resulting pixmap
        * @param height Height of the resulting pixmap
        * @param element The theme element to render ( if none the entire svg is rendered )
        * @return The svg element/file rendered into a pixmap
        */
        QPixmap renderSvg( const QString& keyname, int width, int height, const QString& element = QString() );

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
        QRectF sliderKnobRect( const QRectF &slider, qreal percent, bool inverse ) const;

        /**
         * Get the path of the currently used svg theme file.
         *
         * @return the path of the currently used theme file.
         */
        QString themeFile();

        void setDevicePixelRatioF(qreal dpr);

    public slots:
        void reTint();

    signals:
        void retinted();

    private:
        SvgHandler( QObject* parent = nullptr );

        bool loadSvg( const QString& name );

        QPixmap sliderHandle( const QColor &color, bool pressed, int size );

        QMap<QString, QPixmap> m_cache;

        QScopedPointer<QSvgRenderer> m_renderer;
        QReadWriteLock m_lock;

        QString m_themeFile;

        qreal dpr = 1;
};

#endif
