/***************************************************************************
                          systray.cpp  -  description
                             -------------------
    copyright            : (C) 2002 by Daniel Molkentin <molkentin@kde.org>
                           (C) 2002, 2003 Scott Wheeler <wheeler@kde.org>
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
#include "jukIface.h"

static bool copyImage(QImage &dest, QImage &src, int x, int y);

class PassiveInfo : public KPassivePopup
{
public:
    PassiveInfo(QWidget *parent = 0, const char *name = 0) :
        KPassivePopup(parent, name) {}

protected:
    virtual void enterEvent(QEvent *)
    {
        setTimeout(3000000); // Make timeout damn near infinite
    }
    
    virtual void leaveEvent(QEvent *)
    {
        setTimeout(250); // Close quickly
    }
};

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

SystemTray::SystemTray(KMainWindow *parent, const char *name) : KSystemTray(parent, name),
                                                                m_popup(0)

{
    m_appPix = loadIcon("juk_dock");

    m_playPix = createPixmap("player_play");
    m_pausePix = createPixmap("player_pause");

    m_backPix = loadIcon("player_start");
    m_forwardPix = loadIcon("player_end");

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
        m_popup = new PassiveInfo(this);

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

        // We don't want an autodelete popup.  There are times when it will need
        // to be hidden before the timeout.

        m_popup->setAutoDelete(false);
        m_popup->show();
    }
}

QPixmap SystemTray::createPixmap(const QString &pixName)
{
    QPixmap buffer(m_appPix.width(), m_appPix.height());
    buffer.fill(this, 0, 0);

    QPixmap bgPix = m_appPix;
    QPixmap fgPix = SmallIcon(pixName);

    QImage bgImage = bgPix.convertToImage(); // Probably 22x22
    QImage fgImage = fgPix.convertToImage(); // Should be 16x16

    KIconEffect::semiTransparent (bgImage);
    KIconEffect::semiTransparent (bgImage);
    copyImage(bgImage, fgImage, (bgImage.width() - fgImage.width()) / 2,
              (bgImage.height() - fgImage.height()) / 2);

    bgImage.setAlphaBuffer (true);
    bgPix.convertFromImage (bgImage);
    return bgPix;
}

void SystemTray::setToolTip(const QString &tip)
{
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

    // I already know the type here, but this file doesn't (and I don't want it
    // to) know about the JuK class, so a static_cast won't work, and I was told
    // that a reinterpret_cast isn't portable when combined with multiple
    // inheritance.  (This is why I don't check the result.)

    JuKIface *juk = dynamic_cast<JuKIface *>(parent());

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
            m_forwardAction->activate();
        else
            m_backAction->activate();
        break;
    }
    e->accept();
}

/*
 * This function copies the entirety of src into dest, starting in
 * dest at x and y.  This function exists because I was unable to find
 * a function like it in either QImage or kdefx
 */

static bool copyImage(QImage &dest, QImage &src, int x, int y)
{
    if(dest.depth() != src.depth())
        return false;   
    if((x + src.width()) >= dest.width())
        return false;
    if((y + src.height()) >= dest.height())
        return false;
    
    // We want to use KIconEffect::overlay to do this, since it handles
    // alpha, but the images need to be the same size.  We can handle that.

    QImage large_src(dest);

    // It would perhaps be better to create large_src based on a size, but
    // this is the easiest way to make a new image with the same depth, size,
    // etc.

    large_src.detach();
	
    // However, we do have to specifically ensure that setAlphaBuffer is set
    // to false

    large_src.setAlphaBuffer (false);
    large_src.fill(0); // All transparent pixels
    large_src.setAlphaBuffer (true);
    
    int w = src.width();
    int h = src.height();
    for(int dx = 0; dx < w; dx++)
        for(int dy = 0; dy < h; dy++)
            large_src.setPixel(dx + x, dy + y, src.pixel(dx, dy));
    
    // Apply effect to image

    KIconEffect::overlay (dest, large_src);
    
    return true;
}

#include "systemtray.moc"

// vim: ts=8
