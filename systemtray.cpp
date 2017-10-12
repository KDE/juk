/**
 * Copyright (C) 2002 Daniel Molkentin <molkentin@kde.org>
 * Copyright (C) 2002-2004 Scott Wheeler <wheeler@kde.org>
 * Copyright (C) 2004-2009 Michael Pyne <mpyne@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "systemtray.h"

#include <kiconloader.h>
#include <kactioncollection.h>
#include <kactionmenu.h>
#include <kwindowsystem.h>
#include <KLocalizedString>

#include <QAction>
#include <QMenu>
#include <QTimer>
#include <QWheelEvent>
#include <QColor>
#include <QPushButton>
#include <QPalette>
#include <QPixmap>
#include <QLabel>
#include <QIcon>
#include <QApplication>

#include "tag.h"
#include "actioncollection.h"
#include "playermanager.h"
#include "coverinfo.h"
#include "juk_debug.h"

using namespace ActionCollection;

PassiveInfo::PassiveInfo() :
    QFrame(nullptr,
        Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint
    ),
    m_timer(new QTimer(this)),
    m_layout(new QVBoxLayout(this)),
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
    using ActionCollection::action; // Override the KSNI::action call introduced in KF5

    // This should be initialized to the number of labels that are used.
    m_labels.fill(0, 3);

    setIconByName("juk");
    setCategory(ApplicationStatus);
    setStatus(Active); // We were told to dock in systray by user, force us visible

    m_forwardPix = SmallIcon("media-skip-forward");
    m_backPix = SmallIcon("media-skip-backward");

    // Just create this here so that it show up in the DBus interface and the
    // key bindings dialog.

    QAction *rpaction = new QAction(i18n("Redisplay Popup"), this);
    ActionCollection::actions()->addAction("showPopup", rpaction);
    connect(rpaction, SIGNAL(triggered(bool)), SLOT(slotPlay()));

    QMenu *cm = contextMenu();

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
        // FIXME
    //actionCollection()->addAction("randomplay", menu);
    menu->addAction(action("disableRandomPlay"));
    menu->addAction(action("randomPlay"));
    menu->addAction(action("albumRandomPlay"));
    cm->addAction( menu );

    cm->addAction( action("togglePopups") );

    m_fadeTimer = new QTimer(this);
    m_fadeTimer->setObjectName( QLatin1String("systrayFadeTimer" ));
    connect(m_fadeTimer, SIGNAL(timeout()), SLOT(slotNextStep()));

    // Handle wheel events
    connect(this, SIGNAL(scrollRequested(int,Qt::Orientation)), SLOT(scrollEvent(int,Qt::Orientation)));

    // Add a quick hook for play/pause toggle
    connect(this, SIGNAL(secondaryActivateRequested(QPoint)),
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

QWidget *SystemTray::createInfoBox(QBoxLayout *parentLayout, const FileHandle &file)
{
    // We always show the popup on the right side of the current screen, so
    // this logic assumes that.  Earlier revisions had logic for popup being
    // wherever the systray icon is, so if it's decided to go that route again,
    // dig into the source control history. --mpyne

    if(file.coverInfo()->hasCover()) {
        addCoverButton(parentLayout, file.coverInfo()->pixmap(CoverInfo::Thumbnail));
        addSeparatorLine(parentLayout);
    }

    auto infoBox = new QWidget;
    auto infoBoxVLayout = new QVBoxLayout(infoBox);
    infoBoxVLayout->setSpacing(3);
    infoBoxVLayout->setMargin(3);

    parentLayout->addWidget(infoBox);

    addSeparatorLine(parentLayout);
    createButtonBox(parentLayout);

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

    m_popup = new PassiveInfo;
    connect(m_popup, SIGNAL(destroyed()), SLOT(slotPopupDestroyed()));
    connect(m_popup, SIGNAL(timeExpired()), SLOT(slotFadeOut()));
    connect(m_popup, SIGNAL(nextSong()), SLOT(slotForward()));
    connect(m_popup, SIGNAL(previousSong()), SLOT(slotBack()));

    auto box = new QWidget;
    auto boxHLayout = new QHBoxLayout(box);

    boxHLayout->setSpacing(15); // Add space between text and buttons

    QWidget *infoBox = createInfoBox(boxHLayout, playingFile);
    QLayout *infoBoxLayout = infoBox->layout();

    for(int i = 0; i < m_labels.size(); ++i) {
        QLabel *l = new QLabel(" ");
        l->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_labels[i] = l;
        infoBoxLayout->addWidget(l);
    }

    // We have to set the text of the labels after all of the
    // widgets have been added in order for the width to be calculated
    // correctly.

    int labelCount = 0;

    QString title = playingInfo->title().toHtmlEscaped();
    m_labels[labelCount++]->setText(QString("<qt><nobr><h2>%1</h2></nobr></qt>").arg(title));

    if(!playingInfo->artist().isEmpty())
        m_labels[labelCount++]->setText(playingInfo->artist());

    if(!playingInfo->album().isEmpty()) {
        QString album = playingInfo->album().toHtmlEscaped();
        QString s = playingInfo->year() > 0
            ? QString("<qt><nobr>%1 (%2)</nobr></qt>").arg(album).arg(playingInfo->year())
            : QString("<qt><nobr>%1</nobr></qt>").arg(album);
        m_labels[labelCount++]->setText(s);
    }

    m_popup->setView(box);
    m_popup->show();
}

void SystemTray::createButtonBox(QBoxLayout *parentLayout)
{
    auto buttonBox = new QWidget;
    auto buttonBoxVLayout = new QVBoxLayout(buttonBox);

    buttonBoxVLayout->setSpacing(3);

    QPushButton *forwardButton = new QPushButton(m_forwardPix, QString());
    forwardButton->setObjectName(QLatin1String("popup_forward"));
    connect(forwardButton, SIGNAL(clicked()), SLOT(slotForward()));

    QPushButton *backButton = new QPushButton(m_backPix, QString());
    backButton->setObjectName(QLatin1String("popup_back"));
    connect(backButton, SIGNAL(clicked()), SLOT(slotBack()));

    buttonBoxVLayout->addWidget(forwardButton);
    buttonBoxVLayout->addWidget(backButton);
    parentLayout->addWidget(buttonBox);
}

/**
 * What happens here is that the action->trigger() call will end up invoking
 * createPopup(), which sets m_fade to true.  Before the text starts fading
 * control returns to this function, which resets m_fade to false.
 */
void SystemTray::slotBack()
{
    ActionCollection::action("back")->trigger();
    m_fade = false;
}

void SystemTray::slotForward()
{
    ActionCollection::action("forward")->trigger();
    m_fade = false;
}

void SystemTray::addSeparatorLine(QBoxLayout *parentLayout)
{
    QFrame *line = new QFrame;
    line->setFrameShape(QFrame::VLine);

    // Cover art takes up 80 pixels, make sure we take up at least 80 pixels
    // even if we don't show the cover art for consistency.

    line->setMinimumHeight(80);

    parentLayout->addWidget(line);
}

void SystemTray::addCoverButton(QBoxLayout *parentLayout, const QPixmap &cover)
{
    QPushButton *coverButton = new QPushButton;

    coverButton->setIconSize(cover.size());
    coverButton->setIcon(cover);
    coverButton->setFixedSize(cover.size());
    coverButton->setFlat(true);

    connect(coverButton, SIGNAL(clicked()), this, SLOT(slotPopupLargeCover()));

    parentLayout->addWidget(coverButton);
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
            (step * m_endColor.red()   + (steps - step) * m_startColor.red())   / steps,
            (step * m_endColor.green() + (steps - step) * m_startColor.green()) / steps,
            (step * m_endColor.blue()  + (steps - step) * m_startColor.blue())  / steps
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
    case Qt::ShiftModifier:
        if(delta > 0)
            ActionCollection::action("volumeUp")->trigger();
        else
            ActionCollection::action("volumeDown")->trigger();
        break;
    default:
        if(delta > 0)
            ActionCollection::action("forward")->trigger();
        else
            ActionCollection::action("back")->trigger();
        break;
    }
}

// vim: set et sw=4 tw=0 sta:
