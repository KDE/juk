/***************************************************************************
                          slideraction.cpp  -  description
                             -------------------
    begin                : Wed Feb 6 2002
    copyright            : (C) 2002 by Scott Wheeler
    email                : wheeler@kde.org
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <klocale.h>
#include <kdebug.h>

#include <qslider.h>
#include <qlabel.h>
#include <qtooltip.h>

#include "slideraction.h"

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

SliderAction::SliderAction(const QString &text, QObject *parent, const char *name)
    : CustomAction(text, parent, name)
{

}

SliderAction::~SliderAction()
{

}

QSlider *SliderAction::getTrackPositionSlider() const
{
    if(trackPositionSlider)
        return (trackPositionSlider);
    else
        return(0);
}

QSlider *SliderAction::getVolumeSlider() const
{
    if(volumeSlider)
        return (volumeSlider);
    else
        return(0);
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void SliderAction::updateOrientation(QDockWindow *dockWindow)
{
    // if the toolbar is not null and either the dockWindow not defined or is the toolbar
    if(customWidget && toolbar && (!dockWindow || dockWindow == dynamic_cast<QDockWindow *>(toolbar))) {
        if(toolbar->barPos() == KToolBar::Right || toolbar->barPos() == KToolBar::Left) {
            trackPositionSlider->setOrientation(Qt::Vertical);
            volumeSlider->setOrientation(Qt::Vertical);
            layout->setDirection(QBoxLayout::TopToBottom);
        }
        else {
            trackPositionSlider->setOrientation(Qt::Horizontal);
            volumeSlider->setOrientation(Qt::Horizontal);
            layout->setDirection(QBoxLayout::LeftToRight);
        }
    }
    updateSize();
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

QWidget *SliderAction::createWidget(QWidget *parent) // virtual -- used by base class
{
    if(parent) {
        QWidget *base = new QWidget(parent);
	base->setName("kde toolbar widget");
//	base->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum));

	layout = new QBoxLayout(base, QBoxLayout::TopToBottom, 5, 5);

        trackPositionSlider = new QSlider(base, "trackPositionSlider");
        trackPositionSlider->setMaxValue(1000);
        QToolTip::add(trackPositionSlider, i18n("Track Position"));
        layout->addWidget(trackPositionSlider);

	volumeSlider = new QSlider(base, "volumeSlider" );
        volumeSlider->setMaxValue(100);
        QToolTip::add(volumeSlider, i18n("Volume"));
        layout->addWidget(volumeSlider);

	volumeSlider->setName("kde toolbar widget");
        trackPositionSlider->setName("kde toolbar widget");

        layout->setStretchFactor(trackPositionSlider, 4);
        layout->setStretchFactor(volumeSlider, 1);

//        setWidget(base);

        connect(parent, SIGNAL(modechange()), this, SLOT(updateLabels()));
        connect(parent, SIGNAL(modechange()), this, SLOT(updateSize()));
        return(base);
    }
    else
        return(0);
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void SliderAction::updateLabels()
{

}

void SliderAction::updateSize()
{
    static const int offset = 3;
    static const int absoluteMax = 10000;

    if(customWidget && toolbar) {
        if(toolbar->barPos() == KToolBar::Right || toolbar->barPos() == KToolBar::Left) {
            volumeSlider->setMaximumWidth(toolbar->iconSize() - offset);
            volumeSlider->setMaximumHeight(volumeMax);

            trackPositionSlider->setMaximumWidth(toolbar->iconSize() - offset);
            trackPositionSlider->setMaximumHeight(absoluteMax);
        }
        else {
            volumeSlider->setMaximumHeight(toolbar->iconSize() - offset);
            volumeSlider->setMaximumWidth(volumeMax);

            trackPositionSlider->setMaximumHeight(toolbar->iconSize() - offset);
            trackPositionSlider->setMaximumWidth(absoluteMax);
        }
        //    kdDebug() << "SliderAction::updateLabels()" << endl;
        //    kdDebug() << toolbar->iconSize() << endl;
    }

}
#include "slideraction.moc"
