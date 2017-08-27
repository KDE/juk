/**
 * Copyright (C) 2002-2004 Scott Wheeler <wheeler@kde.org>
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

#ifndef JUK_SLIDERACTION_H
#define JUK_SLIDERACTION_H

#include <QWidgetAction>
#include <QBoxLayout>

#include "volumepopupbutton.h"

class Slider;

class TrackPositionAction : public QWidgetAction
{
    Q_OBJECT
public:
    TrackPositionAction(const QString &text, QObject *parent);

protected:
    QWidget *createWidget(QWidget *parent) override;
};

class VolumeAction : public QWidgetAction
{
    Q_OBJECT
public:
    VolumeAction(const QString &text, QObject *parent);

protected:
    QWidget *createWidget(QWidget *parent) override;

signals:
    void volumeChanged(float newVolume);
    void muteStateChanged(bool muted);
};

#endif

// vim: set et sw=4 tw=0 sta:
