/***************************************************************************
                        k3bexporter.cpp  -  description
                             -------------------
    begin                : Mon May 31 2004
    copyright            : (C) 2004 by Michael Pyne
    email                : pynm0001@comcast.net
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <kprocess.h>
#include <kmessagebox.h>
#include <kurl.h>
#include <klocale.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <kstandarddirs.h>
#include <kiconloader.h>
#include <kapplication.h>

#include <qcstring.h>

#include <dcopref.h>
#include <dcopclient.h>

#include "k3bexporter.h"
#include "playlistitem.h"
#include "playlist.h"
#include "playlistbox.h"
#include "actioncollection.h"

using ActionCollection::actions;

K3bExporter::K3bExporter(Playlist *parent) : PlaylistExporter(parent), m_parent(parent)
{
}

KAction *K3bExporter::action()
{
    if(!KStandardDirs::findExe("k3b").isNull()) {
        return new KAction(
            i18n("Add Selected Items to K3B project"),
            SmallIconSet("k3b"),
            0,
            this,
            SLOT(slotExport()),
            actions(),
            "export_to_k3b"
        );
    }

    return 0;
}

void K3bExporter::exportPlaylistItems(const PlaylistItemList &items)
{
    if(items.empty())
        return;

    DCOPClient *client = DCOPClient::mainClient();
    QCString appId, appObj;
    QByteArray data;

    if(!client->findObject("k3b-*", "K3bInterface", "", data, appId, appObj))
        exportViaCmdLine(items);
    else {
        DCOPRef ref(appId, appObj);
        exportViaDCOP(items, ref);
    }
}

void K3bExporter::slotExport()
{
    if(m_parent)
        exportPlaylistItems(m_parent->selectedItems());
}

void K3bExporter::exportViaCmdLine(const PlaylistItemList &items)
{
    K3bOpenMode mode = openMode();
    QCString cmdOption;

    switch(mode) {
    case AudioCD:
        cmdOption = "--audiocd";
        break;

    case DataCD:
        cmdOption = "--datacd";
        break;

    case Abort:
        return;
    }

    KProcess *process = new KProcess;
    
    *process << "k3b";
    *process << cmdOption;

    PlaylistItemList::ConstIterator it;
    for(it = items.begin(); it != items.end(); ++it)
        *process << (*it)->file().absFilePath();
    
    if(!process->start(KProcess::DontCare))
        KMessageBox::error(m_parent, i18n("Unable to start K3B!"));
}

void K3bExporter::exportViaDCOP(const PlaylistItemList &items, DCOPRef &ref)
{
    QValueList<DCOPRef> projectList;
    DCOPReply projectListReply = ref.call("projects()");

    if(!projectListReply.get<QValueList<DCOPRef> >(projectList, "QValueList<DCOPRef>")) {
        DCOPErrorMessage();
        return;
    }

    if(projectList.count() == 0 && !startNewK3bProject(ref))
        return;

    KURL::List urlList;
    PlaylistItemList::ConstIterator it;

    for(it = items.begin(); it != items.end(); ++it) {
        KURL item;

        item.setPath((*it)->file().absFilePath());
        urlList.append(item);
    }

    if(!ref.send("addUrls(KURL::List)", DCOPArg(urlList, "KURL::List"))) {
        DCOPErrorMessage();
        return;
    }
}

void K3bExporter::DCOPErrorMessage()
{
    KMessageBox::error(m_parent, i18n("There was a DCOP Communication error with K3b!"));
}

bool K3bExporter::startNewK3bProject(DCOPRef &ref)
{
    QCString request;
    K3bOpenMode mode = openMode();

    switch(mode) {
    case AudioCD:
        request = "createAudioCDProject()";
        break;
        
    case DataCD:
        request = "createDataCDProject()";
        break;
        
    case Abort:
        return false;
    }

    if(!ref.send(request)) {
        DCOPErrorMessage();
        return false;
    }

    return true;
}

K3bExporter::K3bOpenMode K3bExporter::openMode()
{
    int reply = KMessageBox::questionYesNoCancel(
        m_parent,
        i18n("Create an audio mode CD suitable for CD players, or a data "
             "mode CD suitable for computers and other digital music "
             "players?"),
        i18n("Create K3B project"),
        i18n("Audio Mode"),
        i18n("Data Mode")
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
        return new KAction(
            i18n("Add Playlist to K3B project"),
            SmallIconSet("k3b"),
            0,
            this,
            SLOT(slotExport()),
            actions(),
            "export_playlist_to_k3b"
        );
    }

    return 0;
}

void K3bPlaylistExporter::slotExport()
{
    if(m_playlistBox) {
        setPlaylist(m_playlistBox->currentPlaylist());
        exportPlaylistItems(m_playlistBox->currentPlaylist()->items());
    }
}

#include "k3bexporter.moc"

// vim: set et sw=4 ts=4:
