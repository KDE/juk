/***************************************************************************
                          playlistsplitter.cpp  -  description
                             -------------------
    begin                : Fri Sep 13 2002
    copyright            : (C) 2002, 2003 by Scott Wheeler
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
#include <kdebug.h>

#include <qlayout.h>

#include "playlistsplitter.h"
#include "searchwidget.h"
#include "playlistsearch.h"
#include "actioncollection.h"
#include "tageditor.h"
#include "collectionlist.h"

using namespace ActionCollection;

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

PlaylistSplitter::PlaylistSplitter(QWidget *parent, const char *name) :
    QSplitter(Qt::Horizontal, parent, name),
    m_playlistBox(0),
    m_searchWidget(0),
    m_playlistStack(0),
    m_editor(0)
{
    setupActions();
    setupLayout();
    readConfig();

    m_editor->slotUpdateCollection();
}

PlaylistSplitter::~PlaylistSplitter()
{
    saveConfig();
    delete m_playlistBox;
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void PlaylistSplitter::setupActions()
{
    KToggleAction *showSearch =
        new KToggleAction(i18n("Show &Search Bar"), "filefind", 0, actions(), "showSearch");
    showSearch->setCheckedState(i18n("Hide &Search Bar"));
}

void PlaylistSplitter::setupLayout()
{
    setOpaqueResize(false);

    // Create a splitter to go between the playlists and the editor.

    QSplitter *editorSplitter = new QSplitter(Qt::Vertical, this, "editorSplitter");

    // Create the playlist and the editor.

    QWidget *top = new QWidget(editorSplitter);
    QVBoxLayout *topLayout = new QVBoxLayout(top);

    m_playlistStack = new QWidgetStack(top, "playlistStack");
    m_editor = new TagEditor(editorSplitter, "tagEditor");

    // Make the editor as small as possible (or at least as small as recommended)

    editorSplitter->setResizeMode(m_editor, QSplitter::FollowSizeHint);

    // Create the PlaylistBox

    m_playlistBox = new PlaylistBox(this, m_playlistStack, "playlistBox");

    connect(m_playlistBox->object(), SIGNAL(signalSelectedItemsChanged()),
            this, SLOT(slotPlaylistSelectionChanged()));

    moveToFirst(m_playlistBox);

    connect(CollectionList::instance(), SIGNAL(signalCollectionChanged()),
            m_editor, SLOT(slotUpdateCollection()));

    // Create the search widget -- this must be done after the CollectionList is created.

    m_searchWidget = new SearchWidget(top, "searchWidget");
    connect(m_searchWidget, SIGNAL(signalQueryChanged()),
            this, SLOT(slotShowSearchResults()));
    connect(m_searchWidget, SIGNAL(signalAdvancedSearchClicked()),
            m_playlistBox->object(), SLOT(slotCreateSearchPlaylist()));
    connect(m_searchWidget, SIGNAL(signalShown(bool)),
            m_playlistBox->object(), SLOT(slotSetSearchEnabled(bool)));
    connect(action<KToggleAction>("showSearch"), SIGNAL(toggled(bool)),
            m_searchWidget, SLOT(setShown(bool)));

    topLayout->addWidget(m_searchWidget);
    topLayout->addWidget(m_playlistStack);

    // Show the collection on startup.
    m_playlistBox->setSelected(0, true);
}

void PlaylistSplitter::readConfig()
{
    KConfigGroup config(KGlobal::config(), "Splitter");

    QValueList<int> splitterSizes = config.readIntListEntry("PlaylistSplitterSizes");
    if(splitterSizes.isEmpty()) {
        splitterSizes.append(100);
        splitterSizes.append(640);
    }
    setSizes(splitterSizes);

    bool showSearch = config.readBoolEntry("ShowSearch", true);
    action<KToggleAction>("showSearch")->setChecked(showSearch);
    m_searchWidget->setShown(showSearch);
}

void PlaylistSplitter::saveConfig()
{
    KConfigGroup config(KGlobal::config(), "Splitter");
    config.writeEntry("PlaylistSplitterSizes", sizes());
    config.writeEntry("ShowSearch", action<KToggleAction>("showSearch")->isChecked());
}

void PlaylistSplitter::slotShowSearchResults()
{
    PlaylistList playlists;
    playlists.append(visiblePlaylist());
    PlaylistSearch search = m_searchWidget->search(playlists);
    visiblePlaylist()->setSearch(search);
}

void PlaylistSplitter::slotPlaylistSelectionChanged()
{
    m_editor->slotSetItems(static_cast<PlaylistCollection *>(m_playlistBox)->selectedItems());
}

#include "playlistsplitter.moc"
