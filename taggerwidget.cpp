/***************************************************************************
                      taggerwidget.cpp  -  description
                             -------------------
    begin                : Tue Feb 5 2002
    copyright            : (C) 2002 by Scott Wheeler
    email                : scott@slackorama.net
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <kmessagebox.h>
#include <klocale.h>
#include <kdebug.h>

#include <qsplitter.h>
#include <qvbox.h>
#include <qhbox.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qdir.h>

#include "taggerwidget.h"
#include "genrelistlist.h"

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

TaggerWidget::TaggerWidget(QWidget *parent) : QWidget(parent)
{
    setupLayout();
}

TaggerWidget::~TaggerWidget()
{

}

void TaggerWidget::add(const QString &item)
{
    taggerList->append(item);
}

void TaggerWidget::add(const QStringList &items)
{
    taggerList->append(items);
}

Playlist *TaggerWidget::getTaggerList()
{
    return(taggerList);
}

QPtrList<PlaylistItem> TaggerWidget::getSelectedItems()
{
    return(taggerList->selectedItems());
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void TaggerWidget::save()
{
    if(editor)
	editor->save();
}

void TaggerWidget::remove()
{
    taggerList->remove();
}


void TaggerWidget::remove(const QPtrList<PlaylistItem> &items)
{
    taggerList->remove(items);
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void TaggerWidget::setupLayout()
{
    //////////////////////////////////////////////////////////////////////////////
    // define a main layout box -- all that should be in this is the splitter
    //////////////////////////////////////////////////////////////////////////////
    QVBoxLayout *taggerMainLayout = new QVBoxLayout(this);

    //////////////////////////////////////////////////////////////////////////////
    // set up a vertical splitter between the file list and the track information
    //////////////////////////////////////////////////////////////////////////////
    QSplitter *split = new QSplitter(Qt::Vertical, this);
    split->setOpaqueResize();

    //////////////////////////////////////////////////////////////////////////////
    // add the splitter to the main layout -- once again, it should for now be
    // the only thing in this layout object
    //////////////////////////////////////////////////////////////////////////////
    taggerMainLayout->addWidget(split);

    //////////////////////////////////////////////////////////////////////////////
    // initialze the tagger list -- the top of the splitter
    //////////////////////////////////////////////////////////////////////////////
    taggerList = new Playlist(split, "taggerList");

    //////////////////////////////////////////////////////////////////////////////
    // now set up a bottom widget of the splitter and make it stay small at
    // start up.  also define a bottem layout to organize things in
    //////////////////////////////////////////////////////////////////////////////
    editor = new TagEditor(split, "editor");
    split->setResizeMode(editor, QSplitter::FollowSizeHint);

    //////////////////////////////////////////////////////////////////////////////
    // set up some connections
    //////////////////////////////////////////////////////////////////////////////

    // Should be moved to CollectionList
    connect(taggerList, SIGNAL(collectionChanged(Playlist *)), editor, SLOT(updateCollection(Playlist *)));
    // Should be moved to Playlist constructor
    connect(taggerList, SIGNAL(selectionChanged(const QPtrList<PlaylistItem> &)), editor, SLOT(setItems(const QPtrList<PlaylistItem> &)));

}

#include "taggerwidget.moc"
