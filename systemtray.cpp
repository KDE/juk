/***************************************************************************
    copyright            : (C) 2002 by Daniel Molkentin
    email                : molkentin@kde.org

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

#include <klocale.h>
#include <kiconloader.h>
#include <kpassivepopup.h>
#include <kiconeffect.h>
#include <kaction.h>
#include <kpopupmenu.h>
#include <kglobalsettings.h>
#include <kdebug.h>

#include <qvbox.h>
#include <qtimer.h>
#include <qcolor.h>
#include <qpushbutton.h>
#include <qtooltip.h>
#include <qpainter.h>
#include <qvaluevector.h>
#include <qstylesheet.h>

#include <netwm.h>

#include "tag.h"
#include "systemtray.h"
#include "actioncollection.h"
#include "playermanager.h"
#include "collectionlist.h"

using namespace ActionCollection;

static bool copyImage(QImage &dest, QImage &src, int x, int y);

class FlickerFreeLabel : public QLabel
{
public:
    FlickerFreeLabel(const QString &text, QWidget *parent, const char *name = 0) :
        QLabel(text, parent, name)
    {
        m_textColor = paletteForegroundColor();
        setBackgroundMode(NoBackground);
    }

    QColor textColor() const
    {
        return m_textColor;
    }

protected:
    virtual void drawContents(QPainter *p)
    {
        // We want to intercept the drawContents call and draw on a pixmap
        // instead of the window to keep flicker to an absolute minimum.
        // Since Qt doesn't refresh the background, we need to do so
        // ourselves.

        QPixmap pix(size());
        QPainter pixPainter(&pix);

        pixPainter.fillRect(rect(), paletteBackgroundColor());
        QLabel::drawContents(&pixPainter);

        bitBlt(p->device(), QPoint(0, 0), &pix, rect(), CopyROP);
    }

    private:
    QColor m_textColor;
};

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

SystemTray::SystemTray(QWidget *parent, const char *name) : KSystemTray(parent, name),
                                                            m_popup(0)

{
    // This should be initialized to the number of labels that are used.

    m_labels.reserve(3);

    m_appPix = loadIcon("juk_dock");

    m_playPix = createPixmap("player_play");
    m_pausePix = createPixmap("player_pause");

    m_forwardPix = loadIcon("player_end");
    m_backPix = loadIcon("player_start");

    setPixmap(m_appPix);

    setToolTip();

    // Just create this here so that it show up in the DCOP interface and the key
    // bindings dialog.

    new KAction(i18n("Redisplay Popup"), KShortcut(), this,
                SLOT(slotPlay()), actions(), "showPopup");
    
    KPopupMenu *cm = contextMenu();

    connect(PlayerManager::instance(), SIGNAL(signalPlay()), this, SLOT(slotPlay()));
    connect(PlayerManager::instance(), SIGNAL(signalPause()), this, SLOT(slotPause()));
    connect(PlayerManager::instance(), SIGNAL(signalStop()), this, SLOT(slotStop()));

    action("play")->plug(cm);
    action("pause")->plug(cm);
    action("stop")->plug(cm);
    action("forward")->plug(cm);
    action("back")->plug(cm);

    cm->insertSeparator();

    // Pity the actionCollection doesn't keep track of what sub-menus it has.

    KActionMenu *menu = new KActionMenu(i18n("&Random Play"), this);
    menu->insert(action("disableRandomPlay"));
    menu->insert(action("randomPlay"));
    menu->insert(action("albumRandomPlay"));
    menu->plug(cm);

    action("togglePopups")->plug(cm);

    if(PlayerManager::instance()->playing())
        slotPlay();
    else if(PlayerManager::instance()->paused())
        slotPause();
}

SystemTray::~SystemTray()
{

}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void SystemTray::slotPlay()
{
    if(!PlayerManager::instance()->playing())
        return;

    setPixmap(m_playPix);
    setToolTip(PlayerManager::instance()->playingString());
    createPopup();
}

void SystemTray::slotTogglePopup()
{
    if(m_popup && m_popup->view()->isVisible())
        m_popup->setTimeout(50);
    else
        slotPlay();
}

void  SystemTray::slotPopupLargeCover()
{
    if(!PlayerManager::instance()->playing())
        return;

    FileHandle playingFile = PlayerManager::instance()->playingFile();
    playingFile.coverInfo()->popupLargeCover();
}

void SystemTray::slotStop()
{
    setPixmap(m_appPix);
    setToolTip();

    delete m_popup;
    m_popup = 0;
}

void SystemTray::slotClearLabels()
{
    for(unsigned i = 0; i < m_labels.capacity(); ++i)
        m_labels[i] = 0;
}

void SystemTray::slotNextStep()
{
    static const int steps = 20;
    QColor result, start, end;
    int r, g, b;

    ++m_step;
    for(unsigned i = 0; i < m_labels.capacity() && m_labels[i]; ++i) {
        start = m_labels[i]->paletteBackgroundColor();
        result = end = m_labels[i]->textColor();

        if(m_step < steps) {
            r = (m_step * end.red() + (steps - m_step) * start.red()) / steps;
            g = (m_step * end.green() + (steps - m_step) * start.green()) / steps;
            b = (m_step * end.blue() + (steps - m_step) * start.blue()) / steps;
            result = QColor(r, g, b);
        }

        m_labels[i]->setPaletteForegroundColor(result);
    }

    if(m_step < steps)
        QTimer::singleShot(1500 / steps, this, SLOT(slotNextStep()));
}

////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////

void SystemTray::createPopup()
{
    FileHandle playingFile = PlayerManager::instance()->playingFile();
    Tag *playingInfo = playingFile.tag();
    
    // If the action exists and it's checked, do our stuff

    if(action<KToggleAction>("togglePopups")->isChecked()) {

        delete m_popup;
        m_popup = new PassiveInfo(this);

        connect(m_popup, SIGNAL(destroyed()), SLOT(slotClearLabels()));
        m_step = 0;

        QHBox *box = new QHBox(m_popup);
        box->setSpacing(15); // Add space between text and buttons

        // See where to put the buttons

        bool onLeft = buttonsToLeft();
        QVBox *infoBox;

        if(onLeft) {

            // They go to the left because JuK is on that side

            createButtonBox(box);
            addSeparatorLine(box);

            infoBox = new QVBox(box);

            // Another line, and the cover, if there's a cover, and if
            // it's selected to be shown

            if(playingFile.coverInfo()->hasCover())
            {
                addSeparatorLine(box);
                addCoverButton(box, playingFile.coverInfo()->coverPixmap());
            }
        }
        else {

            // Buttons go on right because JuK is there

            // Another line, and the cover, if there's a cover, and if
            // it's selected to be shown

            if(playingFile.coverInfo()->hasCover())
            {
                addCoverButton(box, playingFile.coverInfo()->coverPixmap());
                addSeparatorLine(box);
            }

            infoBox = new QVBox(box);

            addSeparatorLine(box);
            createButtonBox(box);
        }

        infoBox->setSpacing(3);
        infoBox->setMargin(3);
        
        for(unsigned i = 0; i < m_labels.capacity(); ++i) {
            m_labels[i] = new FlickerFreeLabel(" ", infoBox);
            m_labels[i]->setAlignment(AlignRight | AlignVCenter);
        }

        // We don't want an autodelete popup.  There are times when it will need
        // to be hidden before the timeout.

        m_popup->setAutoDelete(false);

        // We have to set the text of the labels after all of the
        // widgets have been added in order for the width to be calculated
        // correctly.

        int labelCount = 0;

        QString title = QStyleSheet::escape(playingInfo->title());
        m_labels[labelCount++]->setText(QString("<qt><nobr><h2>%1</h2></nobr><qt>").arg(title));

        if(!playingInfo->artist().isEmpty())
            m_labels[labelCount++]->setText(playingInfo->artist());

        if(!playingInfo->album().isEmpty()) {
            QString album = QStyleSheet::escape(playingInfo->album());
            QString s = playingInfo->year() > 0
                ? QString("%1 (%2)").arg(album).arg(playingInfo->year())
                : album;
            m_labels[labelCount++]->setText(s);
        }

        slotNextStep();

        m_popup->setView(box);
        m_popup->show();
    }
}

bool SystemTray::buttonsToLeft() const
{
    // The following code was nicked from kpassivepopup.cpp

    NETWinInfo ni(qt_xdisplay(), winId(), qt_xrootwin(), 
                  NET::WMIconGeometry | NET::WMKDESystemTrayWinFor);
    NETRect frame, win;
    ni.kdeGeometry(frame, win);
    
    QRect bounds = KGlobalSettings::desktopGeometry(QPoint(win.pos.x, win.pos.y));

    // This seems to accurately guess what side of the icon that
    // KPassivePopup will popup on.
    return(win.pos.x < bounds.center().x());
}

QPixmap SystemTray::createPixmap(const QString &pixName)
{
    QPixmap bgPix = m_appPix;
    QPixmap fgPix = SmallIcon(pixName);

    QImage bgImage = bgPix.convertToImage(); // Probably 22x22
    QImage fgImage = fgPix.convertToImage(); // Should be 16x16

    KIconEffect::semiTransparent(bgImage);
    copyImage(bgImage, fgImage, (bgImage.width() - fgImage.width()) / 2,
              (bgImage.height() - fgImage.height()) / 2);

    bgPix.convertFromImage(bgImage);
    return bgPix;
}

void SystemTray::createButtonBox(QWidget *parent)
{
    QVBox *buttonBox = new QVBox(parent);
    
    buttonBox->setSpacing(3);

    QPushButton *forwardButton = new QPushButton(m_forwardPix, 0, buttonBox, "popup_forward");
    forwardButton->setFlat(true);
    connect(forwardButton, SIGNAL(clicked()), action("forward"), SLOT(activate()));

    QPushButton *backButton = new QPushButton(m_backPix, 0, buttonBox, "popup_back");
    backButton->setFlat(true);
    connect(backButton, SIGNAL(clicked()), action("back"), SLOT(activate()));
}

void SystemTray::addSeparatorLine(QWidget *parent)
{
    QFrame *line = new QFrame(parent);
    line->setFrameShape(QFrame::VLine);

    // Cover art takes up 80 pixels, make sure we take up at least 80 pixels
    // even if we don't show the cover art for consistency.

    line->setMinimumHeight(80);
}

void SystemTray::addCoverButton(QWidget *parent, const QPixmap *cover)
{
    QPushButton *coverButton = new QPushButton(parent);

    coverButton->setPixmap(*cover);
    coverButton->setFixedSize(cover->size());
    coverButton->setFlat(true);

    connect(coverButton, SIGNAL(clicked()), this, SLOT(slotPopupLargeCover()));
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

    switch(e->state()) {
    case ShiftButton:
        if(e->delta() > 0)
            action("volumeUp")->activate();
        else
            action("volumeDown")->activate();
        break;
    default:
        if(e->delta() > 0)
            action("forward")->activate();
        else
            action("back")->activate();
        break;
    }
    e->accept();
}

/*
 * Reimplemented this in order to use the middle mouse button
 */
void SystemTray::mousePressEvent(QMouseEvent *e)
{
    switch(e->button()) {
    case LeftButton:
    case RightButton:
    default:
        KSystemTray::mousePressEvent(e);
        break;
    case MidButton:
        if(!rect().contains(e->pos()))
            return;
        if(action("pause")->isEnabled())
            action("pause")->activate();
        else
            action("play")->activate();
        break;
    }
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

    large_src.setAlphaBuffer(false);
    large_src.fill(0); // All transparent pixels
    large_src.setAlphaBuffer(true);

    int w = src.width();
    int h = src.height();
    for(int dx = 0; dx < w; dx++)
        for(int dy = 0; dy < h; dy++)
            large_src.setPixel(dx + x, dy + y, src.pixel(dx, dy));

    // Apply effect to image

    KIconEffect::overlay(dest, large_src);

    return true;
}


#include "systemtray.moc"

// vim: et sw=4 ts=8
