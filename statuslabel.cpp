/***************************************************************************
    begin                : Fri Oct 18 2002
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

#include <kaction.h>
#include <kpushbutton.h>
#include <kiconloader.h>
#include <ksqueezedtextlabel.h> 
#include <klocale.h>
#include <kdebug.h>

#include <qtooltip.h>
#include <qlayout.h>

#include "statuslabel.h"
#include "filehandle.h"
#include "playlistinterface.h"
#include "actioncollection.h"
#include "tag.h"

using namespace ActionCollection;

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

StatusLabel::StatusLabel(PlaylistInterface *playlist, QWidget *parent, const char *name) :
    QHBox(parent, name),
    PlaylistObserver(playlist),
    m_showTimeRemaining(false)
{
    QFrame *trackAndPlaylist = new QFrame(this);
    trackAndPlaylist->setFrameStyle(Box | Sunken);
    trackAndPlaylist->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Make sure that we have enough of a margin to suffice for the borders,
    // hence the "lineWidth() * 2"
    QHBoxLayout *trackAndPlaylistLayout = new QHBoxLayout(trackAndPlaylist,
                                                          trackAndPlaylist->lineWidth() * 2,
                                                          5, "trackAndPlaylistLayout");
    trackAndPlaylistLayout->addSpacing(5);

    m_playlistLabel = new KSqueezedTextLabel(trackAndPlaylist, "playlistLabel");
    trackAndPlaylistLayout->addWidget(m_playlistLabel);
    m_playlistLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_playlistLabel->setTextFormat(PlainText);
    m_playlistLabel->setAlignment(AlignLeft | AlignVCenter);

    m_trackLabel = new KSqueezedTextLabel(trackAndPlaylist, "trackLabel");
    trackAndPlaylistLayout->addWidget(m_trackLabel);
    m_trackLabel->setAlignment(AlignRight | AlignVCenter);
    m_trackLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_trackLabel->setTextFormat(PlainText);

    trackAndPlaylistLayout->addSpacing(5);

    m_itemTimeLabel = new QLabel(this);
    QFontMetrics fontMetrics(font());
    m_itemTimeLabel->setAlignment(AlignCenter);
    m_itemTimeLabel->setMinimumWidth(fontMetrics.boundingRect("000:00 / 000:00").width());
    m_itemTimeLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    m_itemTimeLabel->setFrameStyle(Box | Sunken);
    m_itemTimeLabel->installEventFilter(this);

    setItemTotalTime(0);
    setItemCurrentTime(0);

    QHBox *jumpBox = new QHBox(this);
    jumpBox->setFrameStyle(Box | Sunken);
    jumpBox->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);

    QPushButton *jumpButton = new QPushButton(jumpBox);
    jumpButton->setPixmap(SmallIcon("up"));
    jumpButton->setFlat(true);

    QToolTip::add(jumpButton, i18n("Jump to the currently playing item"));
    connect(jumpButton, SIGNAL(clicked()), action("showPlaying"), SLOT(activate()));

    installEventFilter(this);

    updateData();
}

StatusLabel::~StatusLabel()
{

}

void StatusLabel::updateCurrent()
{
    if(playlist()->playing()) {
        FileHandle file = playlist()->currentFile();

        QString mid =  file.tag()->artist().isEmpty() || file.tag()->title().isEmpty()
            ? QString::null : QString(" - ");

        QString text = file.tag()->artist() + mid + file.tag()->title();

        m_trackLabel->setText(text);
        m_playlistLabel->setText(playlist()->name().simplifyWhiteSpace());
    }
}

void StatusLabel::updateData()
{
    updateCurrent();

    if(!playlist()->playing()) {
        setItemTotalTime(0);
        setItemCurrentTime(0);

        int time = playlist()->time();

        int days = time / (60 * 60 * 24);
        int hours = time / (60 * 60) % 24;
        int minutes = time / 60 % 60;
        int seconds = time % 60;

        QString timeString;

        if(days > 0) {
            timeString = i18n("1 day", "%n days", days);
            timeString.append(" ");
        }

        if(days > 0 || hours > 0)
            timeString.append(QString().sprintf("%1d:%02d:%02d", hours, minutes, seconds));
        else
            timeString.append(QString().sprintf("%1d:%02d", minutes, seconds));

        m_playlistLabel->setText(playlist()->name());
        m_trackLabel->setText(i18n("1 item", "%n items", playlist()->count()) + " - " + timeString);
    }
}

////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////

void StatusLabel::updateTime()
{
    int minutes;
    int seconds;

    if(m_showTimeRemaining) {
        minutes = int((m_itemTotalTime - m_itemCurrentTime) / 60);
        seconds = (m_itemTotalTime - m_itemCurrentTime) % 60;
    }
    else {
        minutes = int(m_itemCurrentTime / 60);
        seconds = m_itemCurrentTime % 60;
    }

    int totalMinutes = int(m_itemTotalTime / 60);
    int totalSeconds = m_itemTotalTime % 60;

    QString timeString = formatTime(minutes, seconds) +  " / " +
        formatTime(totalMinutes, totalSeconds);
    m_itemTimeLabel->setText(timeString);
}

bool StatusLabel::eventFilter(QObject *o, QEvent *e)
{
    if(!o || !e)
        return false;

    QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(e);
    if(e->type() == QEvent::MouseButtonRelease &&
       mouseEvent->button() == LeftButton)
    {
        if(o == m_itemTimeLabel) {
            m_showTimeRemaining = !m_showTimeRemaining;
            updateTime();
        }
        else
            action("showPlaying")->activate();

        return true;
    }
    return false;
}

QString StatusLabel::formatTime(int minutes, int seconds) // static
{
    QString m = QString::number(minutes);
    if(m.length() == 1)
        m = "0" + m;
    QString s = QString::number(seconds);
    if(s.length() == 1)
        s = "0" + s;
    return m + ":" + s;
}

#include "statuslabel.moc"
