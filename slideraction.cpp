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

#include <ktoolbar.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kdebug.h>

#include <qtooltip.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qtimer.h>

#include "slideraction.h"

////////////////////////////////////////////////////////////////////////////////
// convenience class
////////////////////////////////////////////////////////////////////////////////

/**
 * This "custom" slider reverses the left and middle buttons.  Typically the
 * middle button "instantly" seeks rather than moving the slider towards the
 * click position in fixed intervals.  This behavior has now been mapped on
 * to the left mouse button.
 */

class TrackPositionSlider : public QSlider
{
public:
    TrackPositionSlider(QWidget *parent, const char *name) : QSlider(parent, name)
    {
        setFocusPolicy(NoFocus);
    }

protected:
    virtual void mousePressEvent(QMouseEvent *e)
    {
        if(e->button() == LeftButton) {
            QMouseEvent reverse(QEvent::MouseButtonPress, e->pos(), MidButton, e->state());
            QSlider::mousePressEvent(&reverse);
            emit sliderPressed();
        }
        else if(e->button() == MidButton) {
            QMouseEvent reverse(QEvent::MouseButtonPress, e->pos(), LeftButton, e->state());
            QSlider::mousePressEvent(&reverse);
        }
    }
};

////////////////////////////////////////////////////////////////////////////////
// VolumeSlider implementation
////////////////////////////////////////////////////////////////////////////////

VolumeSlider::VolumeSlider(Orientation o, QWidget *parent, const char *name) :
    QSlider(o, parent, name)
{
    connect(this, SIGNAL(valueChanged(int)), this, SLOT(slotValueChanged(int)));
}

void VolumeSlider::wheelEvent(QWheelEvent *e)
{
    if(orientation() == Horizontal) {
        QWheelEvent transposed(e->pos(), -(e->delta()), e->state(), e->orientation());
        QSlider::wheelEvent(&transposed);
    }
    else
        QSlider::wheelEvent(e);
}

void VolumeSlider::focusInEvent(QFocusEvent *)
{
    clearFocus();
}

int VolumeSlider::volume() const
{
    if(orientation() == Horizontal)
        return value();
    else
        return maxValue() - value();    
}

void VolumeSlider::setVolume(int value)
{
    if(orientation() == Horizontal)
        setValue(value);
    else
        setValue(maxValue() - value); 
}

void VolumeSlider::setOrientation(Orientation o)
{
    if(o == orientation())
        return;

    blockSignals(true);
    setValue(maxValue() - value());
    blockSignals(false);
    QSlider::setOrientation(o);
}

void VolumeSlider::slotValueChanged(int value)
{
    if(orientation() == Horizontal)
        emit signalVolumeChanged(value);
    else
        emit signalVolumeChanged(maxValue() - value);
}

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

const int SliderAction::minPosition = 0;
const int SliderAction::maxPosition = 1000;

SliderAction::SliderAction(const QString &text, QObject *parent, const char *name)
    : KAction(text, 0, parent, name),
      m_toolBar(0),
      m_layout(0),
      m_trackPositionSlider(0),
      m_volumeSlider(0),
      m_dragging(false),
      m_volumeDragging(false)
{

}

SliderAction::~SliderAction()
{

}

int SliderAction::plug(QWidget *parent, int index)
{
    QWidget *w = createWidget(parent);

    if(!w)
        return -1;

    // the check for null makes sure that there is only one toolbar that this is
    // "plugged" in to

    if(parent->inherits("KToolBar") && !m_toolBar) {
        m_toolBar = static_cast<KToolBar *>(parent);

        int id = KAction::getToolButtonID();

        m_toolBar->insertWidget(id, w->width(), w, index);

        addContainer(m_toolBar, id);

        connect(m_toolBar, SIGNAL(destroyed()), this, SLOT(slotToolbarDestroyed()));
        connect(m_toolBar, SIGNAL(orientationChanged(Orientation)),
                this, SLOT(slotUpdateOrientation()));
        connect(m_toolBar, SIGNAL(placeChanged(QDockWindow::Place)),
                this, SLOT(slotUpdateOrientation()));

        slotUpdateOrientation();
        return (containerCount() - 1);
    }
    else
        slotUpdateOrientation();

    return -1;
}


void SliderAction::unplug(QWidget *parent)
{
    if (parent->inherits("KToolBar")) {
        m_toolBar = static_cast<KToolBar *>(parent);

        int index = findContainer(m_toolBar);
        if (index != -1) {
            m_toolBar->removeItem(itemId(index));
            removeContainer(index);

            m_toolBar = 0;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void SliderAction::slotUpdateOrientation()
{
    // if the toolbar is not null and either the dockWindow not defined or is the toolbar

    if(!m_toolBar)
        return;

    if(m_toolBar->barPos() == KToolBar::Right || m_toolBar->barPos() == KToolBar::Left) {
        m_trackPositionSlider->setOrientation(Vertical);
        m_volumeSlider->setOrientation(Vertical);
        m_layout->setDirection(QBoxLayout::TopToBottom);
    }
    else {
        m_trackPositionSlider->setOrientation(Horizontal);
        m_volumeSlider->setOrientation(Horizontal);
        m_layout->setDirection(QBoxLayout::LeftToRight);
    }
    slotUpdateSize();
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

QWidget *SliderAction::createWidget(QWidget *parent) // virtual -- used by base class
{
    if(parent) {
        QWidget *base = new QWidget(parent);
        base->setBackgroundMode(parent->backgroundMode());
        base->setName("kde toolbar widget");

        KToolBar *toolBar = dynamic_cast<KToolBar *>(parent);

        if(toolBar)
            toolBar->setStretchableWidget(base);

        Orientation orientation;

        if(toolBar && toolBar->barPos() == KToolBar::Right || toolBar->barPos() == KToolBar::Left)
            orientation = Vertical;
        else
            orientation = Horizontal;

        m_layout = new QBoxLayout(base, QBoxLayout::TopToBottom, 5, 5);

        m_layout->addItem(new QSpacerItem(20, 1));

        QLabel *trackPositionLabel = new QLabel(base);
        trackPositionLabel->setName("kde toolbar widget");
        trackPositionLabel->setPixmap(SmallIcon("player_time"));
        QToolTip::add(trackPositionLabel, i18n("Track position"));
        m_layout->addWidget(trackPositionLabel);

        m_trackPositionSlider = new TrackPositionSlider(base, "trackPositionSlider");
        m_trackPositionSlider->setMaxValue(maxPosition);
        QToolTip::add(m_trackPositionSlider, i18n("Track position"));
        m_layout->addWidget(m_trackPositionSlider);
        connect(m_trackPositionSlider, SIGNAL(sliderPressed()), this, SLOT(slotSliderPressed()));
        connect(m_trackPositionSlider, SIGNAL(sliderReleased()), this, SLOT(slotSliderReleased()));

        m_layout->addItem(new QSpacerItem(10, 1));

        QLabel *volumeLabel = new QLabel(base);
        volumeLabel->setName("kde toolbar widget");
        volumeLabel->setPixmap(SmallIcon("player_volume"));
        QToolTip::add(volumeLabel, i18n("Volume"));
        m_layout->addWidget(volumeLabel);

        m_volumeSlider = new VolumeSlider(orientation, base, "volumeSlider");
        m_volumeSlider->setMaxValue(100);
        QToolTip::add(m_volumeSlider, i18n("Volume"));
        m_layout->addWidget(m_volumeSlider);
        connect(m_volumeSlider, SIGNAL(signalVolumeChanged(int)), SIGNAL(signalVolumeChanged(int)));
        connect(m_volumeSlider, SIGNAL(sliderPressed()), this, SLOT(slotVolumeSliderPressed()));
        connect(m_volumeSlider, SIGNAL(sliderReleased()), this, SLOT(slotVolumeSliderReleased()));

        m_volumeSlider->setName("kde toolbar widget");
        m_trackPositionSlider->setName("kde toolbar widget");

        m_layout->setStretchFactor(m_trackPositionSlider, 4);
        m_layout->setStretchFactor(m_volumeSlider, 1);

        connect(parent, SIGNAL(modechange()), this, SLOT(slotUpdateSize()));

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

    if(m_toolBar->barPos() == KToolBar::Right || m_toolBar->barPos() == KToolBar::Left) {
        m_volumeSlider->setMaximumWidth(m_toolBar->iconSize() - offset);
        m_volumeSlider->setMaximumHeight(volumeMax);

        m_trackPositionSlider->setMaximumWidth(m_toolBar->iconSize() - offset);
        m_trackPositionSlider->setMaximumHeight(absoluteMax);
    }
    else {
        m_volumeSlider->setMaximumHeight(m_toolBar->iconSize() - offset);
        m_volumeSlider->setMaximumWidth(volumeMax);

        m_trackPositionSlider->setMaximumHeight(m_toolBar->iconSize() - offset);
        m_trackPositionSlider->setMaximumWidth(absoluteMax);
    }
}

void SliderAction::slotSliderPressed()
{
    m_dragging = true;
}

void SliderAction::slotSliderReleased()
{
    m_dragging = false;
    emit signalPositionChanged(m_trackPositionSlider->value());
}

void SliderAction::slotVolumeSliderPressed()
{
    m_volumeDragging = true;
}

void SliderAction::slotVolumeSliderReleased()
{
    m_volumeDragging = false;
    emit signalVolumeChanged(m_volumeSlider->value());
}

void SliderAction::slotToolbarDestroyed()
{
    int index = findContainer(m_toolBar);
    if(index != -1)
        removeContainer(index);

    m_toolBar = 0;

    // This is probably a leak, but this code path hardly ever occurs, and it's
    // too hard to debug correctly.

    m_trackPositionSlider = 0;
    m_volumeSlider = 0;
}

#include "slideraction.moc"

// vim: set et sw=4 ts=4:
