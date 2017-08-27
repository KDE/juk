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

#include "slideraction.h"

#include <ktoolbar.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kactioncollection.h>

#include <QMouseEvent>
#include <QWheelEvent>
#include <QFocusEvent>
#include <QBoxLayout>

#include "volumepopupbutton.h"
#include "slider.h"
#include "playermanager.h"
#include "juk.h"
#include "juk_debug.h"

TrackPositionAction::TrackPositionAction(const QString &text, QObject *parent) :
    QWidgetAction(parent)
{
    this->setText(text);
}

QWidget *TrackPositionAction::createWidget(QWidget *parent)
{
    Slider *slider = new TimeSlider(parent);
    slider->setObjectName(QLatin1String("timeSlider"));

    PlayerManager *player = JuK::JuKInstance()->playerManager();

    // TODO only connect the tick signal when actually visible
    connect(player, &PlayerManager::tick, slider, &Slider::setValue);
    connect(player, &PlayerManager::seekableChanged, slider, &Slider::setEnabled);
    connect(player, &PlayerManager::seekableChanged, slider, [slider](bool seekable) {
        static const QString noSeekMsg =
            i18n("Seeking is not supported in this file with your audio settings.");
        slider->setToolTip(seekable ? QString() : noSeekMsg);
    });
    connect(player, &PlayerManager::totalTimeChanged, slider, &Slider::setMaximum);
    connect(slider, &Slider::valueChanged, player, &PlayerManager::seek);

    return slider;
}

VolumeAction::VolumeAction(const QString &text, QObject *parent)
    : QWidgetAction(parent)
{
    this->setText(text);
}

QWidget *VolumeAction::createWidget(QWidget *parent)
{
    return new VolumePopupButton(parent);
}

// vim: set et sw=4 tw=0 sta:
