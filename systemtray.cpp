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
#include <kmenu.h>
#include <kglobalsettings.h>
#include <kdebug.h>
#include <kactionmenu.h>
#include <ktoggleaction.h>

#include <q3vbox.h>
#include <QTimer>
#include <QColor>
#include <QPushButton>
#include <QToolTip>
#include <qpainter.h>
#include <q3valuevector.h>
#include <q3stylesheet.h>
#include <qpalette.h>

#include <QWheelEvent>
#include <QPixmap>
#include <QEvent>
#include <Q3MimeSourceFactory>
#include <Q3Frame>
#include <QLabel>
#include <QMouseEvent>

#include <netwm.h>
#include <QX11Info>
#include <QTextDocument>

#include "tag.h"
#include "systemtray.h"
#include "actioncollection.h"
#include "playermanager.h"
#include "collectionlist.h"
#include "coverinfo.h"
#include <kactioncollection.h>

using namespace ActionCollection;

static bool copyImage(QImage &dest, QImage &src, int x, int y);

#if 0  // not necessary in Qt-4.1
class FlickerFreeLabel : public QLabel
{
public:
    FlickerFreeLabel(const QString &text, QWidget *parent, const char *name = 0) :
        QLabel(text, parent, name)
    {
        m_textColor = paletteForegroundColor();
        m_bgColor = parentWidget()->paletteBackgroundColor();
        //setBackgroundMode(Qt::NoBackground);
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
#endif

PassiveInfo::PassiveInfo(QWidget *parent) :
    KPassivePopup(parent), m_timer(new QTimer), m_justDie(false)
{
    // I'm so sick and tired of KPassivePopup screwing this up
    // that I'll just handle the timeout myself, thank you very much.
    KPassivePopup::setTimeout(0);

    connect(m_timer, SIGNAL(timeout()), SLOT(timerExpired()));
}

void PassiveInfo::setTimeout(int delay)
{
    m_timer->start(delay);
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

SystemTray::SystemTray(QWidget *parent) : KSystemTrayIcon(parent),
                                          m_popup(0),
                                          m_fadeTimer(0),
                                          m_fade(true)

{
    // This should be initialized to the number of labels that are used.

    m_labels.reserve(3);

    m_appPix = loadIcon("juk_dock");

    m_playPix = createPixmap("player_play");
    m_pausePix = createPixmap("player_pause");

    m_forwardPix = SmallIcon("player_end");
    m_backPix = SmallIcon("player_start");

    setIcon(m_appPix);

    setToolTip();

    // Just create this here so that it show up in the DCOP interface and the key
    // bindings dialog.

    KAction *rpaction = new KAction(i18n("Redisplay Popup"), this);
    ActionCollection::actions()->addAction("showPopup", rpaction);
    connect(rpaction, SIGNAL(triggered(bool) ), SLOT(slotPlay()));

    QMenu *cm = contextMenu();

    connect(PlayerManager::instance(), SIGNAL(signalPlay()), this, SLOT(slotPlay()));
    connect(PlayerManager::instance(), SIGNAL(signalPause()), this, SLOT(slotPause()));
    connect(PlayerManager::instance(), SIGNAL(signalStop()), this, SLOT(slotStop()));

    cm->addAction( action("play") );
    cm->addAction( action("pause") );
    cm->addAction( action("stop") );
    cm->addAction( action("forward") );
    cm->addAction( action("back") );

    cm->addSeparator();

    // Pity the actionCollection doesn't keep track of what sub-menus it has.

    KActionMenu *menu = new KActionMenu(i18n("&Random Play"), this);
    actionCollection()->addAction("randomplay", menu);
    menu->addAction(action("disableRandomPlay"));
    menu->addAction(action("randomPlay"));
    menu->addAction(action("albumRandomPlay"));
    cm->addAction( menu );

    cm->addAction( action("togglePopups") );

    m_fadeTimer = new QTimer(this);
    m_fadeTimer->setObjectName("systrayFadeTimer");
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

    setIcon(m_playPix);
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
    setIcon(m_appPix);
    setToolTip();

    delete m_popup;
    m_popup = 0;
}

void SystemTray::slotPopupDestroyed()
{
    for(int i = 0; i < m_labels.capacity(); ++i)
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

    for(int i = 0; i < m_labels.capacity() && m_labels[i]; ++i) {
        QPalette palette;
        palette.setColor(m_labels[i]->foregroundRole(), result);
        m_labels[i]->setPalette(palette);
    }

    if(m_step == STEPS) {
        m_step = 0;
        m_fadeTimer->stop();
        emit fadeDone();
    }
}

void SystemTray::slotFadeOut()
{
    m_startColor = m_labels[0]->palette().color( QPalette::Text ); //textColor();
    m_endColor = m_labels[0]->palette().color( QPalette::Window ); //backgroundColor();

    connect(this, SIGNAL(fadeDone()), m_popup, SLOT(hide()));
    connect(m_popup, SIGNAL(mouseEntered()), this, SLOT(slotMouseInPopup()));
    m_fadeTimer->start(1500 / STEPS);
}

// If we receive this signal, it's because we were called during fade out.
// That means there is a single shot timer about to call slotNextStep, so we
// don't have to do it ourselves.
void SystemTray::slotMouseInPopup()
{
    m_endColor = m_labels[0]->palette().color( QPalette::Text ); //textColor();
    disconnect(SIGNAL(fadeDone()));

    m_step = STEPS - 1; // Simulate end of fade to solid text
    slotNextStep();
}

////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////

Q3VBox *SystemTray::createPopupLayout(QWidget *parent, const FileHandle &file)
{
    Q3VBox *infoBox = 0;

    if(buttonsToLeft()) {

        // They go to the left because JuK is on that side

        createButtonBox(parent);
        addSeparatorLine(parent);

        infoBox = new Q3VBox(parent);

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

        infoBox = new Q3VBox(parent);

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

    if(!ActionCollection::action<KToggleAction>("togglePopups")->isChecked())
        return;

    delete m_popup;
    m_popup = 0;
    m_fadeTimer->stop();

    // This will be reset after this function call by slot(Forward|Back)
    // so it's safe to set it true here.
    m_fade = true;
    m_step = 0;

#warning FIXME: this will not be associated with the systray any longer
    m_popup = new PassiveInfo(0);
    connect(m_popup, SIGNAL(destroyed()), SLOT(slotPopupDestroyed()));
    connect(m_popup, SIGNAL(timeExpired()), SLOT(slotFadeOut()));

    Q3HBox *box = new Q3HBox(m_popup, "popupMainLayout");
    box->setSpacing(15); // Add space between text and buttons

    Q3VBox *infoBox = createPopupLayout(box, playingFile);

    for(int i = 0; i < m_labels.capacity(); ++i) {
        m_labels[i] = new QLabel /*FlickerFreeLabel*/(" ", infoBox);
        m_labels[i]->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    }

    // We don't want an autodelete popup.  There are times when it will need
    // to be hidden before the timeout.

    m_popup->setAutoDelete(false);

    // We have to set the text of the labels after all of the
    // widgets have been added in order for the width to be calculated
    // correctly.

    int labelCount = 0;

    QString title = Qt::escape(playingInfo->title());
    m_labels[labelCount++]->setText(QString("<qt><nobr><h2>%1</h2></nobr><qt>").arg(title));

    if(!playingInfo->artist().isEmpty())
        m_labels[labelCount++]->setText(playingInfo->artist());

    if(!playingInfo->album().isEmpty()) {
        QString album = Qt::escape(playingInfo->album());
        QString s = playingInfo->year() > 0
            ? QString("<qt><nobr>%1 (%2)</nobr></qt>").arg(album).arg(playingInfo->year())
            : QString("<qt><nobr>%1</nobr></qt>").arg(album);
        m_labels[labelCount++]->setText(s);
    }

    m_startColor = m_labels[0]->palette().color( QPalette::Window ); //backgroundColor();
    m_endColor = m_labels[0]->palette().color( QPalette::Text ); //textColor();
    //m_startColor = m_labels[0]->backgroundColor();
    //m_endColor = m_labels[0]->textColor();

    slotNextStep();
    m_fadeTimer->start(1500 / STEPS);

    m_popup->setView(box);
    m_popup->show();
}

bool SystemTray::buttonsToLeft() const
{
    // The following code was nicked from kpassivepopup.cpp

#warning the systray is no longer a widget
    NETWinInfo ni(QX11Info::display(), /* winId() */ 0, QX11Info::appRootWindow(),
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
    QImage bgImage = m_appPix.pixmap(22).toImage(); // 22x22
    QImage fgImage = SmallIcon(pixName).toImage(); // Should be 16x16

    KIconEffect::semiTransparent(bgImage);
    copyImage(bgImage, fgImage, (bgImage.width() - fgImage.width()) / 2,
              (bgImage.height() - fgImage.height()) / 2);

    return QPixmap::fromImage(bgImage);
}

void SystemTray::createButtonBox(QWidget *parent)
{
    Q3VBox *buttonBox = new Q3VBox(parent);

    buttonBox->setSpacing(3);

    QPushButton *forwardButton = new QPushButton(m_forwardPix, 0, buttonBox);
    forwardButton->setObjectName("popup_forward");
    forwardButton->setFlat(true);
    connect(forwardButton, SIGNAL(clicked()), SLOT(slotForward()));

    QPushButton *backButton = new QPushButton(m_backPix, 0, buttonBox);
    backButton->setObjectName("popup_back");
    backButton->setFlat(true);
    connect(backButton, SIGNAL(clicked()), SLOT(slotBack()));
}

/**
 * What happens here is that the action->trigger() call will end up invoking
 * createPopup(), which sets m_fade to true.  Before the text starts fading
 * control returns to this function, which resets m_fade to false.
 */
void SystemTray::slotBack()
{
    action("back")->trigger();
    m_fade = false;
}

void SystemTray::slotForward()
{
    action("forward")->trigger();
    m_fade = false;
}

void SystemTray::addSeparatorLine(QWidget *parent)
{
    Q3Frame *line = new Q3Frame(parent);
    line->setFrameShape(Q3Frame::VLine);

    // Cover art takes up 80 pixels, make sure we take up at least 80 pixels
    // even if we don't show the cover art for consistency.

    line->setMinimumHeight(80);
}

void SystemTray::addCoverButton(QWidget *parent, const QPixmap &cover)
{
    QPushButton *coverButton = new QPushButton(parent);

    coverButton->setIcon(cover);
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
    if(tip.isNull())
        KSystemTrayIcon::setToolTip( i18n("JuK"));
    else {
        QPixmap myCover = cover;
        if(cover.isNull())
            myCover = DesktopIcon("juk");

        QImage coverImage = myCover.toImage();
        if(coverImage.size().width() > 32 || coverImage.size().height() > 32)
            coverImage = coverImage.scaled(32, 32, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

        Q3MimeSourceFactory::defaultFactory()->setImage("tipCover", coverImage);

        QString html = i18nc("%1 is Cover Art, %2 is the playing track, %3 is the appname",
                            "<center><table cellspacing=\"2\"><tr><td valign=\"middle\">%1</td>"
                            "<td valign=\"middle\">%2</td></tr></table><em>%3</em></center>",
                            QString("<img valign=\"middle\" src=\"tipCover\""),
                            QString("<nobr>%1</nobr>").arg(tip), i18n("JuK"));

        KSystemTrayIcon::setToolTip( html);
    }
}

void SystemTray::wheelEvent(QWheelEvent *e)
{
    if(e->orientation() == Qt::Horizontal)
        return;

    // I already know the type here, but this file doesn't (and I don't want it
    // to) know about the JuK class, so a static_cast won't work, and I was told
    // that a reinterpret_cast isn't portable when combined with multiple
    // inheritance.  (This is why I don't check the result.)

    switch(e->modifiers()) {
    case Qt::ShiftButton:
        if(e->delta() > 0)
            action("volumeUp")->trigger();
        else
            action("volumeDown")->trigger();
        break;
    default:
        if(e->delta() > 0)
            action("forward")->trigger();
        else
            action("back")->trigger();
        break;
    }
    e->accept();
}

/*
 * Reimplemented this in order to use the middle mouse button
 */
void SystemTray::slotActivated(QSystemTrayIcon::ActivationReason reason)
{
     if (reason != MiddleClick)
        return;

        if(action("pause")->isEnabled())
            action("pause")->trigger();
        else
            action("play")->trigger();
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

    large_src.convertToFormat(QImage::Format_RGB32); // Turn off alpha
    large_src.fill(0); // All transparent pixels
    large_src.convertToFormat(QImage::Format_ARGB32); // Turn on alpha

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

// vim: set et sw=4 tw=0 sta:
