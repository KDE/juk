/***************************************************************************
    begin                : Wed Feb 6 2002
    copyright            : (C) 2002 - 2004 by Scott Wheeler
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

////////////////////////////////////////////////////////////////////////////////
// VolumeSlider implementation
////////////////////////////////////////////////////////////////////////////////

VolumeSlider::VolumeSlider(QWidget *parent) :
    Phonon::VolumeSlider(parent)
{
    setOrientation(Qt::Horizontal);
    setMinimumWidth(70);
}

void VolumeSlider::focusInEvent(QFocusEvent *)
{
    clearFocus();
}

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

SliderAction::SliderAction(const QString &text, QObject* parent)
    : KAction(text, parent),
      m_toolBar(0),
      m_widget(0),
      m_layout(0),
      m_trackPositionSlider(0),
      m_volumeSlider(0)
{
}

SliderAction::~SliderAction()
{

}

int SliderAction::plug(QWidget *parent, int index)
{
    Q_UNUSED(index)

    m_widget = createWidget(parent);

    if(!m_widget)
        return -1;

    // the check for null makes sure that there is only one toolbar that this is
    // "plugged" in to

    if(!m_toolBar && qobject_cast<KToolBar *>(parent)) {
        m_toolBar = static_cast<KToolBar *>(parent);
        m_toolBar->addWidget(m_widget);
        connect(m_toolBar, SIGNAL(destroyed()), this, SLOT(slotToolbarDestroyed()));
        return (associatedWidgets().count() - 1);
    }

    return -1;
}


void SliderAction::unplug(QWidget *parent)
{
    if (parent->inherits("KToolBar")) {
        delete m_widget;
        m_widget = 0;
        m_toolBar = 0;
    }
}

QWidget *SliderAction::createToolBarWidget(QToolBar *parent)
{
    if(parent) {
        QWidget *base = new QWidget(parent);
        base->setBackgroundRole(parent->backgroundRole());
        base->setObjectName(QLatin1String("kde toolbar widget"));

        m_layout = new QHBoxLayout(base);
        //m_layout->setDirection(QBoxLayout::TopToBottom);
	m_layout->setMargin(5);
        m_layout->setSpacing(5);

        m_layout->addItem(new QSpacerItem(20, 1));

        m_trackPositionSlider = new Phonon::SeekSlider(base);
        m_trackPositionSlider->setObjectName(QLatin1String("trackPositionSlider"));
        m_trackPositionSlider->setOrientation(Qt::Horizontal);
        //m_trackPositionSlider->setToolTip( i18n("Track position"));
        m_layout->addWidget(m_trackPositionSlider);
        m_layout->addItem(new QSpacerItem(10, 1));

        m_volumeSlider = new VolumeSlider(base);
        m_volumeSlider->setObjectName(QLatin1String("volumeSlider"));
        m_layout->addWidget(m_volumeSlider);
        connect(parent, SIGNAL(iconSizeChanged(QSize)), m_volumeSlider,
                SLOT(setIconSize(QSize)));

        m_volumeSlider->setObjectName(QLatin1String("kde toolbar widget"));
        m_trackPositionSlider->setObjectName(QLatin1String("kde toolbar widget"));

        m_layout->setStretchFactor(m_trackPositionSlider, 4);
        m_layout->setStretchFactor(m_volumeSlider, 1);

        connect(parent, SIGNAL(modechange()), this, SLOT(slotUpdateSize()));

        return base;
    }
    else
        return 0;
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

QWidget *SliderAction::createWidget(QWidget *parent) // virtual -- used by base class
{
    if(parent) {
        QWidget *base = new QWidget(parent);
        base->setBackgroundRole(parent->backgroundRole());
        base->setObjectName( QLatin1String("kde toolbar widget" ));

        m_layout = new QHBoxLayout(base);
        m_layout->setMargin(5);
        m_layout->setSpacing(5);

        m_layout->addItem(new QSpacerItem(20, 1));

        m_trackPositionSlider = new Phonon::SeekSlider(base);
        m_trackPositionSlider->setObjectName(QLatin1String("trackPositionSlider"));
        //m_trackPositionSlider->setToolTip(i18n("Track position"));
        m_layout->addWidget(m_trackPositionSlider);
        m_layout->addItem(new QSpacerItem(10, 1));

        m_volumeSlider = new VolumeSlider(base);
        m_volumeSlider->setObjectName(QLatin1String("volumeSlider"));
        m_layout->addWidget(m_volumeSlider);
        m_volumeSlider->setObjectName(QLatin1String("kde toolbar widget"));
        m_trackPositionSlider->setObjectName(QLatin1String("kde toolbar widget"));

        m_layout->setStretchFactor(m_trackPositionSlider, 4);
        m_layout->setStretchFactor(m_volumeSlider, 1);

        return base;
    }
    else
        return 0;
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void SliderAction::slotUpdateSize()
{
    static const int offset = 3;
    static const int absoluteMax = 10000;

    if(!m_toolBar)
        return;

    if(m_toolBar->orientation() == Qt::Vertical) {
        m_volumeSlider->setMaximumWidth(m_toolBar->iconSize().width() - offset);
        m_volumeSlider->setMaximumHeight(volumeMax);

        m_trackPositionSlider->setMaximumWidth(m_toolBar->iconSize().width() - offset);
        m_trackPositionSlider->setMaximumHeight(absoluteMax);
    }
    else {
        m_volumeSlider->setMaximumHeight(m_toolBar->iconSize().height() - offset);
        m_volumeSlider->setMaximumWidth(volumeMax);

        m_trackPositionSlider->setMaximumHeight(m_toolBar->iconSize().height() - offset);
        m_trackPositionSlider->setMaximumWidth(absoluteMax);
    }
}

void SliderAction::slotToolbarDestroyed()
{
#if 0 // what's going on here?
    int index = findContainer(m_toolBar);
    if(index != -1)
        removeContainer(index);

#endif
    m_toolBar = 0;

    // This is probably a leak, but this code path hardly ever occurs, and it's
    // too hard to debug correctly.

    m_trackPositionSlider = 0;
    m_volumeSlider = 0;
}

#include "slideraction.moc"

// vim: set et sw=4 tw=0 sta:
