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
#include <qpalette.h>

#include <netwm.h>

#include "tag.h"
#include "systemtray.h"
#include "actioncollection.h"
#include "playermanager.h"
#include "collectionlist.h"
#include "coverinfo.h"

using namespace ActionCollection;

static bool copyImage(QImage &dest, QImage &src, int x, int y);

class FlickerFreeLabel : public QLabel
{
public:
    FlickerFreeLabel(const QString &text, QWidget *parent, const char *name = 0) :
        QLabel(text, parent, name)
    {
        m_textColor = paletteForegroundColor();
        m_bgColor = parentWidget()->paletteBackgroundColor();
        setBackgroundMode(NoBackground);
    }

    QColor textColor() const
    {
        return m_textColor;
    }

    QColor backgroundColor() const
    {
        return m_bgColor;
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

        pixPainter.fillRect(rect(), m_bgColor);
        QLabel::drawContents(&pixPainter);

        bitBlt(p->device(), QPoint(0, 0), &pix, rect(), CopyROP);
    }

    private:
    QColor m_textColor;
    QColor m_bgColor;
};

PassiveInfo::PassiveInfo(QWidget *parent, const char *name) :
    KPassivePopup(parent, name), m_timer(new QTimer), m_justDie(false)
{
    // I'm so sick and tired of KPassivePopup screwing this up
    // that I'll just handle the timeout myself, thank you very much.
    KPassivePopup::setTimeout(0);

    connect(m_timer, SIGNAL(timeout()), SLOT(timerExpired()));
}

void PassiveInfo::setTimeout(int delay)
{
    m_timer->changeInterval(delay);
}

void PassiveInfo::show()
{
    KPassivePopup::show();
    m_timer->start(3500);
}

void PassiveInfo::timerExpired()
{
    // If m_justDie is set, we should just go, otherwise we should emit the
    // signal and wait for the system tray to delete us.
    if(m_justDie)
        hide();
    else
        emit timeExpired();
}

void PassiveInfo::enterEvent(QEvent *)
{
    m_timer->stop();
    emit mouseEntered();
}

void PassiveInfo::leaveEvent(QEvent *)
{
    m_justDie = true;
    m_timer->start(50);
}

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

SystemTray::SystemTray(QWidget *parent, const char *name) : KSystemTray(parent, name),
                                                            m_popup(0),
                                                            m_fadeTimer(0),
                                                            m_fade(true)

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

    m_fadeTimer = new QTimer(this, "systrayFadeTimer");
    connect(m_fadeTimer, SIGNAL(timeout()), SLOT(slotNextStep()));

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

    QPixmap cover = PlayerManager::instance()->playingFile().coverInfo()->pixmap(CoverInfo::Thumbnail);

    setPixmap(m_playPix);
    setToolTip(PlayerManager::instance()->playingString(), cover);
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
    playingFile.coverInfo()->popup();
}

void SystemTray::slotStop()
{
    setPixmap(m_appPix);
    setToolTip();

    delete m_popup;
    m_popup = 0;
}

void SystemTray::slotPopupDestroyed()
{
    for(unsigned i = 0; i < m_labels.capacity(); ++i)
        m_labels[i] = 0;
}

void SystemTray::slotNextStep()
{
    QColor result;

    ++m_step;

    // If we're not fading, immediately show the labels
    if(!m_fade)
        m_step = STEPS;

    result = interpolateColor(m_step);

    for(unsigned i = 0; i < m_labels.capacity() && m_labels[i]; ++i)
        m_labels[i]->setPaletteForegroundColor(result);

    if(m_step == STEPS) {
        m_step = 0;
        m_fadeTimer->stop();
        emit fadeDone();
    }
}

void SystemTray::slotFadeOut()
{
    m_startColor = m_labels[0]->textColor();
    m_endColor = m_labels[0]->backgroundColor();

    connect(this, SIGNAL(fadeDone()), m_popup, SLOT(hide()));
    connect(m_popup, SIGNAL(mouseEntered()), this, SLOT(slotMouseInPopup()));
    m_fadeTimer->start(1500 / STEPS);
}

// If we receive this signal, it's because we were called during fade out.
// That means there is a single shot timer about to call slotNextStep, so we
// don't have to do it ourselves.
void SystemTray::slotMouseInPopup()
{
    m_endColor = m_labels[0]->textColor();
    disconnect(SIGNAL(fadeDone()));

    m_step = STEPS - 1; // Simulate end of fade to solid text
    slotNextStep();
}

////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////

QVBox *SystemTray::createPopupLayout(QWidget *parent, const FileHandle &file)
{
    QVBox *infoBox = 0;

    if(buttonsToLeft()) {

        // They go to the left because JuK is on that side

        createButtonBox(parent);
        addSeparatorLine(parent);

        infoBox = new QVBox(parent);

        // Another line, and the cover, if there's a cover, and if
        // it's selected to be shown

        if(file.coverInfo()->hasCover()) {
            addSeparatorLine(parent);
            addCoverButton(parent, file.coverInfo()->pixmap(CoverInfo::Thumbnail));
        }
    }
    else {

        // Like above, but reversed.

        if(file.coverInfo()->hasCover()) {
            addCoverButton(parent, file.coverInfo()->pixmap(CoverInfo::Thumbnail));
            addSeparatorLine(parent);
        }

        infoBox = new QVBox(parent);

        addSeparatorLine(parent);
        createButtonBox(parent);
    }

    infoBox->setSpacing(3);
    infoBox->setMargin(3);
    return infoBox;
}

void SystemTray::createPopup()
{
    FileHandle playingFile = PlayerManager::instance()->playingFile();
    Tag *playingInfo = playingFile.tag();
    
    // If the action exists and it's checked, do our stuff

    if(!action<KToggleAction>("togglePopups")->isChecked())
        return;

    delete m_popup;
    m_popup = 0;
    m_fadeTimer->stop();

    // This will be reset after this function call by slot(Forward|Back)
    // so it's safe to set it true here.
    m_fade = true;
    m_step = 0;

    m_popup = new PassiveInfo(this);
    connect(m_popup, SIGNAL(destroyed()), SLOT(slotPopupDestroyed()));
    connect(m_popup, SIGNAL(timeExpired()), SLOT(slotFadeOut()));

    QHBox *box = new QHBox(m_popup, "popupMainLayout");
    box->setSpacing(15); // Add space between text and buttons

    QVBox *infoBox = createPopupLayout(box, playingFile);

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
            ? QString("<qt><nobr>%1 (%2)</nobr></qt>").arg(album).arg(playingInfo->year())
            : QString("<qt><nobr>%1</nobr></qt>").arg(album);
        m_labels[labelCount++]->setText(s);
    }

    m_startColor = m_labels[0]->backgroundColor();
    m_endColor = m_labels[0]->textColor();

    slotNextStep();
    m_fadeTimer->start(1500 / STEPS);

    m_popup->setView(box);
    m_popup->show();
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
    connect(forwardButton, SIGNAL(clicked()), SLOT(slotForward()));

    QPushButton *backButton = new QPushButton(m_backPix, 0, buttonBox, "popup_back");
    backButton->setFlat(true);
    connect(backButton, SIGNAL(clicked()), SLOT(slotBack()));
}

/**
 * What happens here is that the action->activate() call will end up invoking
 * createPopup(), which sets m_fade to true.  Before the text starts fading
 * control returns to this function, which resets m_fade to false.
 */
void SystemTray::slotBack()
{
    action("back")->activate();
    m_fade = false;
}

void SystemTray::slotForward()
{
    action("forward")->activate();
    m_fade = false;
}

void SystemTray::addSeparatorLine(QWidget *parent)
{
    QFrame *line = new QFrame(parent);
    line->setFrameShape(QFrame::VLine);

    // Cover art takes up 80 pixels, make sure we take up at least 80 pixels
    // even if we don't show the cover art for consistency.

    line->setMinimumHeight(80);
}

void SystemTray::addCoverButton(QWidget *parent, const QPixmap &cover)
{
    QPushButton *coverButton = new QPushButton(parent);

    coverButton->setPixmap(cover);
    coverButton->setFixedSize(cover.size());
    coverButton->setFlat(true);

    connect(coverButton, SIGNAL(clicked()), this, SLOT(slotPopupLargeCover()));
}

QColor SystemTray::interpolateColor(int step, int steps)
{
    if(step < 0)
        return m_startColor;
    if(step >= steps)
        return m_endColor;

    // TODO: Perhaps the algorithm here could be better?  For example, it might
    // make sense to go rather quickly from start to end and then slow down
    // the progression.
    return QColor(
            (step * m_endColor.red() + (steps - step) * m_startColor.red()) / steps,
            (step * m_endColor.green() + (steps - step) * m_startColor.green()) / steps,
            (step * m_endColor.blue() + (steps - step) * m_startColor.blue()) / steps
           );
}

void SystemTray::setToolTip(const QString &tip, const QPixmap &cover)
{
    QToolTip::remove(this);

    if(tip.isNull())
        QToolTip::add(this, i18n("JuK"));
    else {
        QPixmap myCover = cover;
        if(cover.isNull())
            myCover = DesktopIcon("juk");

        QImage coverImage = myCover.convertToImage();
        if(coverImage.size().width() > 32 || coverImage.size().height() > 32)
            coverImage = coverImage.smoothScale(32, 32);

        QMimeSourceFactory::defaultFactory()->setImage("tipCover", coverImage);

        QString html = i18n("%1 is Cover Art, %2 is the playing track, %3 is the appname",
                            "<center><table cellspacing=\"2\"><tr><td valign=\"middle\">%1</td>"
                            "<td valign=\"middle\">%2</td></tr></table><em>%3</em></center>");
        html = html.arg("<img valign=\"middle\" src=\"tipCover\"");
        html = html.arg(QString("<nobr>%1</nobr>").arg(tip), i18n("JuK"));

        QToolTip::add(this, html);
    }
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
