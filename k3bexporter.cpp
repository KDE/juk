/***************************************************************************
    begin                : Mon May 31 2004
    copyright            : (C) 2004 by Michael Pyne
    email                : michael.pyne@kdemail.net
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "k3bexporter.h"

#include <kprocess.h>
#include <kmessagebox.h>
#include <kurl.h>
#include <klocale.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <kstandarddirs.h>
#include <kicon.h>
#include <kiconloader.h>
#include <kapplication.h>

#include <QMap>

#include "playlistitem.h"
#include "playlist.h"
#include "playlistbox.h"
#include "actioncollection.h"

using ActionCollection::actions;

// static member variable definition

PlaylistAction *K3bExporter::m_action = 0;

// Special KAction subclass used to automatically call a slot when activated,
// depending on the visible playlist at the time.  In other words, use *one*
// instance of this action for many playlists.
//
// This is used to handle some actions in the Playlist context menu.

PlaylistAction::PlaylistAction(const QString &userText,
                const QIcon &pix,
                const char *slot,
                const KShortcut &cut) :
    KAction(userText, actions()),
    m_slot(slot)
{
    setShortcut(cut);
    QAction::setIcon(pix);
    connect(this, SIGNAL(triggered()), this, SLOT(slotActivated()));
}

/**
 * Defines a QObject to call (using the m_slot SLOT) when an action is
 * emitted from a Playlist.
 */
void PlaylistAction::addCallMapping(const Playlist *p, QObject *obj)
{
    m_playlistRecipient[p] = obj;
}

void PlaylistAction::slotActivated()
{
    // Determine current playlist, and call its slot.
    Playlist *p = PlaylistCollection::instance()->visiblePlaylist();
    if(!p)
        return;
    // Make sure we're supposed to notify someone about this playlist.
    QObject *recipient = m_playlistRecipient[p];
    if(!recipient)
        return;
    // Invoke the slot using some trickery.
    recipient->metaObject()->invokeMethod(recipient, m_slot);
}

K3bExporter::K3bExporter(Playlist *parent) : PlaylistExporter(parent), m_parent(parent)
{
}

KAction *K3bExporter::action()
{
    if(!m_action && !KStandardDirs::findExe("k3b").isNull()) {
        m_action = new PlaylistAction(
            i18n("Add Selected Items to Audio or Data CD"),
            KIcon("k3b"),
            "slotExport"
        );
        m_action->setShortcutConfigurable(false);
    }
    // Tell the action to let us know when it is activated when
    // m_parent is the visible playlist.  This allows us to reuse the
    // action to avoid duplicate entries in KActionCollection.
    if(m_action)
        m_action->addCallMapping(m_parent, this);
    return m_action;
}

void K3bExporter::exportPlaylistItems(const PlaylistItemList &items)
{
    if(items.empty())
        return;
    exportViaCmdLine(items);
}

void K3bExporter::slotExport()
{
    if(m_parent)
        exportPlaylistItems(m_parent->selectedItems());
}

void K3bExporter::exportViaCmdLine(const PlaylistItemList &items)
{
    K3bOpenMode mode = openMode();
    QByteArray cmdOption;

    switch(mode) {
    case AudioCD:
        cmdOption = "--audiocd";
        break;

    case DataCD:
        cmdOption = "--data";
        break;

    case Abort:
        return;
    }

    KProcess process;

    process << "k3b";
    process << cmdOption;

    PlaylistItemList::ConstIterator it;
    for(it = items.begin(); it != items.end(); ++it)
        process << (*it)->file().absFilePath();

    if(!process.startDetached())
        KMessageBox::error(m_parent, i18n("Unable to start K3b."));
}

K3bExporter::K3bOpenMode K3bExporter::openMode()
{
    int reply = KMessageBox::questionYesNoCancel(
        m_parent,
        i18n("Create an audio mode CD suitable for CD players, or a data "
             "mode CD suitable for computers and other digital music "
             "players?"),
        i18n("Create K3b Project"),
        KGuiItem(i18n("Audio Mode")),
        KGuiItem(i18n("Data Mode"))
    );

    switch(reply) {
    case KMessageBox::Cancel:
        return Abort;

    case KMessageBox::No:
        return DataCD;

    case KMessageBox::Yes:
        return AudioCD;
    }

    return Abort;
}

K3bPlaylistExporter::K3bPlaylistExporter(PlaylistBox *parent) : K3bExporter(0),
    m_playlistBox(parent)
{
}

KAction *K3bPlaylistExporter::action()
{
    if(!KStandardDirs::findExe("k3b").isNull()) {
        KAction *action = new KAction(KIcon("k3b"),
            i18n("Add Playlist to Audio or Data CD"),
            actions()
        );
        connect(action, SIGNAL(triggered(bool)), SLOT(slotExport()));
        return action;
    }

    return 0;
}

void K3bPlaylistExporter::slotExport()
{
    if(m_playlistBox) {
        setPlaylist(m_playlistBox->visiblePlaylist());
        exportPlaylistItems(m_playlistBox->visiblePlaylist()->items());
    }
}

#include "k3bexporter.moc"

// vim: set et sw=4 tw=0 sta:
