/***************************************************************************
                          systray.cpp  -  description
                             -------------------
    copyright            : (C) 2002 by Daniel Molkentin 
    email                : molkentin@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <kiconloader.h>
#include <kpopupmenu.h>
#include <kpassivepopup.h>
#include <kaction.h>
#include <kmainwindow.h>
#include <kiconeffect.h>
#include <kdebug.h>

#include <qhbox.h>
#include <qpushbutton.h>
#include <qtooltip.h>
#include <qpainter.h>

#include "systemtray.h"
#include "juk.h"

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

SystemTray::SystemTray(KMainWindow *parent, const char *name) : KSystemTray(parent, name),
								m_popup(0)

{
    m_appPix = UserIcon("juk_dock");

    m_playPix = createPixmap("player_play");
    m_pausePix = createPixmap("player_pause");

    m_backPix = SmallIcon("player_start");
    m_forwardPix = SmallIcon("player_end");

    setPixmap(m_appPix);

    setToolTip();

    KPopupMenu *cm = contextMenu();

    m_actionCollection = parent->actionCollection();

    m_playAction    = m_actionCollection->action("play");
    m_pauseAction   = m_actionCollection->action("pause");
    m_stopAction    = m_actionCollection->action("stop");
    m_backAction    = m_actionCollection->action("back");
    m_forwardAction = m_actionCollection->action("forward");

    m_togglePopupsAction = static_cast<KToggleAction *>(m_actionCollection->action("togglePopups"));

    m_playAction->plug(cm);
    m_pauseAction->plug(cm);
    m_stopAction->plug(cm);
    m_backAction->plug(cm);
    m_forwardAction->plug(cm);

    cm->insertSeparator();

    m_togglePopupsAction->plug(cm);
}

SystemTray::~SystemTray()
{

}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void SystemTray::slotNewSong(const QString& songName)
{
    setToolTip(songName);
    createPopup(songName, true);
}

void SystemTray::slotStop()
{
    setPixmap(m_appPix);
    setToolTip();

    delete m_popup;
    m_popup = 0;
}

////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////

void SystemTray::createPopup(const QString &songName, bool addButtons) 
{
    // If the action exists and it's checked, do our stuff
    if(m_togglePopupsAction && m_togglePopupsAction->isChecked()) {

	delete m_popup;
        m_popup = new KPassivePopup(this);

        QHBox *box = new QHBox(m_popup);

        // The buttons are now optional - no real reason, but it might be useful later
        if(addButtons) {
            QPushButton *backButton = new QPushButton(m_backPix, 0, box, "popup_back");
            backButton->setFlat(true);
            connect(backButton, SIGNAL(clicked()), m_backAction, SLOT(activate()));
        }

        QLabel *l = new QLabel(songName, box);
	l->setMargin(3);

        if(addButtons) {
            QPushButton *forwardButton = new QPushButton (m_forwardPix, 0, box, "popup_forward");
            forwardButton->setFlat(true);
            connect(forwardButton, SIGNAL(clicked()), m_forwardAction, SLOT(activate()));
        }

        m_popup->setView(box);

        // We don't want an autodelete popup.  There are times when it will need to be hidden before the timeout.
        m_popup->setAutoDelete(false);
        m_popup->show();
    }
}

QPixmap SystemTray::createPixmap(const QString &pixName)
{
    QPixmap buffer(22, 22);
    buffer.fill(this, 0, 0);

    QPixmap bgPix = m_appPix;
    QPixmap fgPix = SmallIcon(pixName);

    // Make this pretty close to invisible.  I'm certain there's a cleaner way to
    // do this, but I am exceedingly lazy.
    KIconEffect::semiTransparent(bgPix);
    KIconEffect::semiTransparent(bgPix);

    QPainter p(&buffer);
    p.drawPixmap(0, 0, bgPix);

    p.drawPixmap((buffer.width() - fgPix.width()) / 2, 
	(buffer.height() - fgPix.height()) / 2, fgPix);

    return buffer;
}

void SystemTray::setToolTip(const QString &tip)
{
    // if(!QToolTip::textFor(this).isNull())
    QToolTip::remove(this);

    if(tip.isNull())
        QToolTip::add(this, "JuK");
    else
        QToolTip::add(this, tip);
}

void SystemTray::wheelEvent(QWheelEvent *e)
{
    if(e->orientation() == Horizontal)
	return;
    
    JuKIface *juk = static_cast<JuK *>(parent());

    switch(e->state()) {
    case ShiftButton:
	if(juk) {
	    if(e->delta() > 0)
		juk->volumeUp();
	    else
		juk->volumeDown();
	}
	break;
    default:
	if(e->delta() > 0)
	    m_backAction->activate();
	else
	    m_forwardAction->activate();
	break;
    }
    e->accept();
}

#include "systemtray.moc"

// vim: ts=8
