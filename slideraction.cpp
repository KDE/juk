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
// convenience class
////////////////////////////////////////////////////////////////////////////////

/**
 * This "custom" slider reverses the left and middle buttons.  Typically the
 * middle button "instantly" seeks rather than moving the slider towards the
 * click position in fixed intervals.  This behavior has now been mapped on
 * to the left mouse button.
 */

#if 0
class TrackPositionSlider : public QSlider
{
public:
    TrackPositionSlider(QWidget *parent) : QSlider(Qt::Horizontal, parent)
    {
        setFocusPolicy(Qt::NoFocus);
    }

protected:
    virtual void mousePressEvent(QMouseEvent *e)
    {
        if(e->button() == Qt::LeftButton) {
            QMouseEvent reverse(QEvent::MouseButtonPress, e->pos(), Qt::MidButton, e->buttons(), e->modifiers());
            QSlider::mousePressEvent(&reverse);
            emit sliderPressed();
        }
        else if(e->button() == Qt::MidButton) {
            QMouseEvent reverse(QEvent::MouseButtonPress, e->pos(), Qt::LeftButton, e->buttons(), e->modifiers());
            QSlider::mousePressEvent(&reverse);
        }
    }
};
#endif

////////////////////////////////////////////////////////////////////////////////
// VolumeSlider implementation
////////////////////////////////////////////////////////////////////////////////

VolumeSlider::VolumeSlider(Qt::Orientation o, QWidget *parent) :
    Phonon::VolumeSlider(parent)
{
    setOrientation(o);
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

//        m_toolBar->insertWidget(id, m_widget->width(), m_widget, index);
        m_toolBar->addWidget(m_widget);

        connect(m_toolBar, SIGNAL(destroyed()), this, SLOT(slotToolbarDestroyed()));
        connect(m_toolBar, SIGNAL(orientationChanged(Orientation)),
                this, SLOT(slotUpdateOrientation()));
        connect(m_toolBar, SIGNAL(placeChanged(Q3DockWindow::Place)),
                this, SLOT(slotUpdateOrientation()));
        if(m_volumeSlider)
        {
            connect(m_toolBar, SIGNAL(iconSizeChanged(QSize)), m_volumeSlider,
                    SLOT(setIconSize(QSize)));
            connect(m_toolBar, SIGNAL(iconSizeChanged(QSize)), m_trackPositionSlider,
                    SLOT(setIconSize(QSize)));
        }

        slotUpdateOrientation();
        return (associatedWidgets().count() - 1);
    }
    else
        slotUpdateOrientation();

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

QWidget *SliderAction::createToolBarWidget( QToolBar * parent )
{
    if(parent) {
        QWidget *base = new QWidget(parent);
        base->setBackgroundRole(parent->backgroundRole());
        base->setObjectName( QLatin1String("kde toolbar widget" ));

        KToolBar *toolBar = dynamic_cast<KToolBar *>(parent);

        Qt::Orientation orientation = Qt::Horizontal;

        if(toolBar) {
//            toolBar->setStretchableWidget(base);
            orientation = toolBar->orientation();
        }

        m_layout = new QHBoxLayout(base);
        //m_layout->setDirection(QBoxLayout::TopToBottom);
	m_layout->setMargin(5);
        m_layout->setSpacing(5);

        m_layout->addItem(new QSpacerItem(20, 1));

        m_trackPositionSlider = new Phonon::SeekSlider(base);
        m_trackPositionSlider->setObjectName( QLatin1String("trackPositionSlider" ));
        m_trackPositionSlider->setOrientation(orientation);
        //m_trackPositionSlider->setToolTip( i18n("Track position"));
        m_layout->addWidget(m_trackPositionSlider);
        connect(parent, SIGNAL(iconSizeChanged(QSize)), m_trackPositionSlider,
                SLOT(setIconSize(QSize)));

        m_layout->addItem(new QSpacerItem(10, 1));

        m_volumeSlider = new VolumeSlider(orientation, base);
        m_volumeSlider->setObjectName( QLatin1String("volumeSlider" ));
        m_layout->addWidget(m_volumeSlider);
        connect(parent, SIGNAL(iconSizeChanged(QSize)), m_volumeSlider,
                SLOT(setIconSize(QSize)));

        m_volumeSlider->setObjectName( QLatin1String("kde toolbar widget" ));
        m_trackPositionSlider->setObjectName( QLatin1String("kde toolbar widget" ));

        m_layout->setStretchFactor(m_trackPositionSlider, 4);
        m_layout->setStretchFactor(m_volumeSlider, 1);

        connect(parent, SIGNAL(modechange()), this, SLOT(slotUpdateSize()));

        return base;
    }
    else
        return 0;
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void SliderAction::slotUpdateOrientation()
{
    // if the toolbar is not null and either the dockWindow not defined or is the toolbar

    if(!m_toolBar)
        return;

    if(m_toolBar->orientation() == Qt::Vertical) {
        m_trackPositionSlider->setOrientation(Qt::Vertical);
        m_volumeSlider->setOrientation(Qt::Vertical);
        m_layout->setDirection(QBoxLayout::TopToBottom);
    }
    else {
        m_trackPositionSlider->setOrientation(Qt::Horizontal);
        m_volumeSlider->setOrientation(Qt::Horizontal);
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
        base->setBackgroundRole(parent->backgroundRole());
        base->setObjectName( QLatin1String("kde toolbar widget" ));

        KToolBar *toolBar = dynamic_cast<KToolBar *>(parent);

        Qt::Orientation orientation = Qt::Horizontal;

        if(toolBar) {
//            toolBar->setStretchableWidget(base);
            orientation = toolBar->orientation();
        }

        if(orientation == Qt::Horizontal)
            m_layout = new QHBoxLayout(base);
        else
            m_layout = new QVBoxLayout(base);
        m_layout->setMargin(5);
        m_layout->setSpacing(5);

        m_layout->addItem(new QSpacerItem(20, 1));

        m_trackPositionSlider = new Phonon::SeekSlider(base);
        m_trackPositionSlider->setObjectName( QLatin1String("trackPositionSlider" ));
        //m_trackPositionSlider->setToolTip( i18n("Track position"));
        m_layout->addWidget(m_trackPositionSlider);
        connect(parent, SIGNAL(iconSizeChanged(QSize)), m_trackPositionSlider,
                SLOT(setIconSize(QSize)));

        m_layout->addItem(new QSpacerItem(10, 1));

        m_volumeSlider = new VolumeSlider(orientation, base);
        m_volumeSlider->setObjectName( QLatin1String("volumeSlider" ));
        m_layout->addWidget(m_volumeSlider);
        connect(parent, SIGNAL(iconSizeChanged(QSize)), m_volumeSlider,
                SLOT(setIconSize(QSize)));

        m_volumeSlider->setObjectName( QLatin1String("kde toolbar widget" ));
        m_trackPositionSlider->setObjectName( QLatin1String("kde toolbar widget" ));

        m_layout->setStretchFactor(m_trackPositionSlider, 4);
        m_layout->setStretchFactor(m_volumeSlider, 1);

        // Style changes like icon only, text only, icon + text
        connect(parent, SIGNAL(toolButtonStyleChanged(Qt::ToolButtonStyle)), this, SLOT(slotUpdateSize()));

        // Icon size changed
        connect(parent, SIGNAL(iconSizeChanged(const QSize &)), this, SLOT(slotUpdateSize()));

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
