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
#include <kdebug.h>

#include <QMouseEvent>
#include <QWheelEvent>
#include <QFocusEvent>
#include <QBoxLayout>

#include "volumepopupbutton.h"
#include "slider.h"
#include "playermanager.h"
#include "juk.h"

TrackPositionAction::TrackPositionAction(const QString &text, QObject *parent) :
    KAction(text, parent)
{

}

Slider *TrackPositionAction::slider() const
{
    return parent()->findChild<Slider *>("timeSlider");
}

QWidget *TrackPositionAction::createWidget(QWidget *parent)
{
    Slider *slider = new TimeSlider(parent);
    slider->setObjectName(QLatin1String("timeSlider"));

    PlayerManager *player = JuK::JuKInstance()->playerManager();

    connect(player, SIGNAL(tick(int)), slider, SLOT(setValue(int)));
    connect(player, SIGNAL(seekableChanged(bool)), this, SLOT(seekableChanged(bool)));
    connect(player, SIGNAL(totalTimeChanged(int)), this, SLOT(totalTimeChanged(int)));
    connect(slider, SIGNAL(sliderMoved(int)), player, SLOT(seek(int)));

    return slider;
}

void TrackPositionAction::seekableChanged(bool seekable)
{
    slider()->setEnabled(seekable);
    slider()->setToolTip(seekable ?
                         QString() :
                         i18n("Seeking is not supported in this file with your audio settings."));
}

void TrackPositionAction::totalTimeChanged(int ms)
{
    slider()->setRange(0, ms);
}

VolumeAction::VolumeAction(const QString &text, QObject *parent) :
    KAction(text, parent),
    m_button(0)
{

}

QWidget *VolumeAction::createWidget(QWidget *parent)
{
    m_button = new VolumePopupButton(parent);
    return m_button;
}

#include "slideraction.moc"

// vim: set et sw=4 tw=0 sta:
