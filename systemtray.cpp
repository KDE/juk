/***************************************************************************
    copyright            : (C) 2002 by Daniel Molkentin
    email                : molkentin@kde.org

    copyright            : (C) 2002 - 2004 by Scott Wheeler
    email                : wheeler@kde.org

    copyright            : (C) 2004 - 2009 by Michael Pyne
    email                : mpyne@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "systemtray.h"

#include <klocale.h>
#include <kiconloader.h>
#include <kaction.h>
#include <kdebug.h>
#include <kactioncollection.h>
#include <kactionmenu.h>
#include <kvbox.h>
#include <kmenu.h>
#include <kwindowsystem.h>

#include <QTimer>
#include <QWheelEvent>
#include <QColor>
#include <QPushButton>
#include <QPalette>
#include <QPixmap>
#include <QLabel>
#include <QVBoxLayout>
#include <QIcon>
#include <QApplication>
#include <QTextDocument> // Qt::escape()

#include "tag.h"
#include "actioncollection.h"
#include "playermanager.h"
#include "coverinfo.h"

using namespace ActionCollection;

PassiveInfo::PassiveInfo(SystemTray *parent) :
    QFrame(static_cast<QWidget *>(0),
        Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint
    ),
    m_icon(parent),
    m_timer(new QTimer(this)),
    m_layout(new QVBoxLayout(this)),
    m_view(0),
    m_justDie(false)
{
    connect(m_timer, SIGNAL(timeout()), SLOT(timerExpired()));
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    // Workaround transparent background in Oxygen when (ab-)using Qt::ToolTip
    setAutoFillBackground(true);

    setFrameStyle(StyledPanel | Plain);
    setLineWidth(2);
}

void PassiveInfo::startTimer(int delay)
{
    m_timer->start(delay);
}

void PassiveInfo::show()
{
    m_timer->start(3500);
    setWindowOpacity(1.0);
    QFrame::show();
}

QWidget *PassiveInfo::view() const
{
    return m_view;
}

void PassiveInfo::setView(QWidget *view)
{
    m_layout->addWidget(view);
    view->show(); // We are still hidden though.
    adjustSize();
    positionSelf();
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

void PassiveInfo::hideEvent(QHideEvent *)
{
}

void PassiveInfo::wheelEvent(QWheelEvent *e)
{
    if(e->delta() >= 0) {
        emit nextSong();
    }
    else {
        emit previousSong();
    }

    e->accept();
}

void PassiveInfo::positionSelf()
{
    // Start with a QRect of our size, move it to the right spot.
    QRect r(rect());
    QRect curScreen(KWindowSystem::workArea());

    // Try to position in lower right of the screen
    QPoint anchor(curScreen.right() * 7 / 8, curScreen.bottom());

    // Now make our rect hit that anchor.
    r.moveBottomRight(anchor);

    move(r.topLeft());
}

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

SystemTray::SystemTray(PlayerManager *player, QWidget *parent) :
    KStatusNotifierItem(parent),
    m_popup(0),
    m_player(player),
    m_fadeTimer(0),
    m_fade(true),
    m_hasCompositionManager(false)
{
    // This should be initialized to the number of labels that are used.
    m_labels.fill(0, 3);

    setIconByName("juk");
    setCategory(ApplicationStatus);
    setStatus(Active); // We were told to dock in systray by user, force us visible

    m_forwardPix = SmallIcon("media-skip-forward");
    m_backPix = SmallIcon("media-skip-backward");

    // Just create this here so that it show up in the DBus interface and the
    // key bindings dialog.

    KAction *rpaction = new KAction(i18n("Redisplay Popup"), this);
    ActionCollection::actions()->addAction("showPopup", rpaction);
    connect(rpaction, SIGNAL(triggered(bool)), SLOT(slotPlay()));

    KMenu *cm = contextMenu();

    connect(m_player, SIGNAL(signalPlay()), this, SLOT(slotPlay()));
    connect(m_player, SIGNAL(signalPause()), this, SLOT(slotPause()));
    connect(m_player, SIGNAL(signalStop()), this, SLOT(slotStop()));

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

    // Handle wheel events
    connect(this, SIGNAL(scrollRequested(int, Qt::Orientation)), SLOT(scrollEvent(int, Qt::Orientation)));

    // Add a quick hook for play/pause toggle
    connect(this, SIGNAL(secondaryActivateRequested(const QPoint &)),
            action("playPause"), SLOT(trigger()));

    if(m_player->playing())
        slotPlay();
    else if(m_player->paused())
        slotPause();
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void SystemTray::slotPlay()
{
    if(!m_player->playing())
        return;

    QPixmap cover = m_player->playingFile().coverInfo()->pixmap(CoverInfo::FullSize);

    setOverlayIconByName("media-playback-start");
    setToolTip(m_player->playingString(), cover);
    createPopup();
}

void SystemTray::slotPause()
{
    setOverlayIconByName("media-playback-pause");
}

void SystemTray::slotTogglePopup()
{
    if(m_popup && m_popup->view()->isVisible())
        m_popup->startTimer(50);
    else
        slotPlay();
}

void SystemTray::slotPopupLargeCover()
{
    if(!m_player->playing())
        return;

    FileHandle playingFile = m_player->playingFile();
    playingFile.coverInfo()->popup();
}

void SystemTray::slotStop()
{
    setToolTip();
    setOverlayIconByName(QString());

    delete m_popup;
    m_popup = 0;
    m_fadeTimer->stop();
}

void SystemTray::slotPopupDestroyed()
{
    for(int i = 0; i < m_labels.size(); ++i)
        m_labels[i] = 0;
}

void SystemTray::slotNextStep()
{
    // Could happen I guess if the timeout event were queued while we're deleting m_popup
    if(!m_popup)
        return;

    ++m_step;

    // If we're not fading, immediately stop the fadeout
    if(!m_fade || m_step == STEPS) {
        m_step = 0;
        m_fadeTimer->stop();
        emit fadeDone();
        return;
    }

    if(m_hasCompositionManager) {
        m_popup->setWindowOpacity((1.0 * STEPS - m_step) / STEPS);
    }
    else {
        QColor result = interpolateColor(m_step);

        for(int i = 0; i < m_labels.size() && m_labels[i]; ++i) {
            QPalette palette;
            palette.setColor(m_labels[i]->foregroundRole(), result);
            m_labels[i]->setPalette(palette);
        }
    }
}

void SystemTray::slotFadeOut()
{
    m_startColor = m_labels[0]->palette().color( QPalette::Text ); //textColor();
    m_endColor = m_labels[0]->palette().color( QPalette::Window ); //backgroundColor();

    m_hasCompositionManager = KWindowSystem::compositingActive();

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

    if(m_hasCompositionManager)
        m_popup->setWindowOpacity(1.0);

    m_step = STEPS - 1; // Simulate end of fade to solid text
    slotNextStep();
}

////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////

KVBox *SystemTray::createPopupLayout(QWidget *parent, const FileHandle &file)
{
    KVBox *infoBox = 0;

    // We always show the popup on the right side of the current screen, so
    // this logic assumes that.  Earlier revisions has logic for popup being
    // wherever the systray icon is, so if it's decided to go that route again,
    // dig into the source control history. --mpyne

    if(file.coverInfo()->hasCover()) {
        addCoverButton(parent, file.coverInfo()->pixmap(CoverInfo::Thumbnail));
        addSeparatorLine(parent);
    }

    infoBox = new KVBox(parent);

    addSeparatorLine(parent);
    createButtonBox(parent);

    infoBox->setSpacing(3);
    infoBox->setMargin(3);
    return infoBox;
}

void SystemTray::createPopup()
{
    FileHandle playingFile = m_player->playingFile();
    Tag *playingInfo = playingFile.tag();

    // If the action exists and it's checked, do our stuff

    if(!ActionCollection::action("togglePopups")->isChecked())
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
    connect(m_popup, SIGNAL(nextSong()), SLOT(slotForward()));
    connect(m_popup, SIGNAL(previousSong()), SLOT(slotBack()));

    KHBox *box = new KHBox(m_popup);
    box->setSpacing(15); // Add space between text and buttons

    KVBox *infoBox = createPopupLayout(box, playingFile);

    for(int i = 0; i < m_labels.size(); ++i) {
        QLabel *l = new QLabel(" ", infoBox);
        l->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_labels[i] = l;
    }

    // We have to set the text of the labels after all of the
    // widgets have been added in order for the width to be calculated
    // correctly.

    int labelCount = 0;

    QString title = Qt::escape(playingInfo->title());
    m_labels[labelCount++]->setText(QString("<qt><nobr><h2>%1</h2></nobr></qt>").arg(title));

    if(!playingInfo->artist().isEmpty())
        m_labels[labelCount++]->setText(playingInfo->artist());

    if(!playingInfo->album().isEmpty()) {
        QString album = Qt::escape(playingInfo->album());
        QString s = playingInfo->year() > 0
            ? QString("<qt><nobr>%1 (%2)</nobr></qt>").arg(album).arg(playingInfo->year())
            : QString("<qt><nobr>%1</nobr></qt>").arg(album);
        m_labels[labelCount++]->setText(s);
    }

    m_popup->setView(box);
    m_popup->show();
}

void SystemTray::createButtonBox(QWidget *parent)
{
    KVBox *buttonBox = new KVBox(parent);

    buttonBox->setSpacing(3);

    QPushButton *forwardButton = new QPushButton(m_forwardPix, 0, buttonBox);
    forwardButton->setObjectName("popup_forward");
    connect(forwardButton, SIGNAL(clicked()), SLOT(slotForward()));

    QPushButton *backButton = new QPushButton(m_backPix, 0, buttonBox);
    backButton->setObjectName("popup_back");
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
    QFrame *line = new QFrame(parent);
    line->setFrameShape(QFrame::VLine);

    // Cover art takes up 80 pixels, make sure we take up at least 80 pixels
    // even if we don't show the cover art for consistency.

    line->setMinimumHeight(80);
}

void SystemTray::addCoverButton(QWidget *parent, const QPixmap &cover)
{
    QPushButton *coverButton = new QPushButton(parent);

    coverButton->setIconSize(cover.size());
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
    if(tip.isEmpty())
        KStatusNotifierItem::setToolTip("juk", i18n("JuK"), QString());
    else {
        QPixmap myCover;
        if(cover.isNull()) {
            myCover = DesktopIcon("juk");
        } else {
            //Scale to proper icon size, otherwise KStatusNotifierItem will show an unknown icon
            int iconSize = KIconLoader::global()->currentSize(KIconLoader::Desktop);
            myCover = cover.scaled(iconSize, iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        KStatusNotifierItem::setToolTip(QIcon(myCover), i18n("JuK"), tip);
    }
}

void SystemTray::scrollEvent(int delta, Qt::Orientation orientation)
{
    if(orientation == Qt::Horizontal)
        return;

    switch(QApplication::keyboardModifiers()) {
    case Qt::ShiftButton:
        if(delta > 0)
            action("volumeUp")->trigger();
        else
            action("volumeDown")->trigger();
        break;
    default:
        if(delta > 0)
            action("forward")->trigger();
        else
            action("back")->trigger();
        break;
    }
}

#include "systemtray.moc"

// vim: set et sw=4 tw=0 sta:
