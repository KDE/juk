/***************************************************************************
                          genrelisteditor.cpp  -  description
                             -------------------
    begin                : Sun Dec 8 2002
    copyright            : (C) 2002 by Scott Wheeler
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

#include <kcombobox.h>
#include <klistview.h>
#include <klineedit.h>
#include <kdebug.h>

#include "genrelisteditor.h"
#include "genrelistlist.h"

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

GenreListEditor::GenreListEditor(QWidget *parent, const char *name ) : GenreListEditorBase(parent, name, true) 
{
    loadID3v1Genres();
    loadLists();
}

GenreListEditor::~GenreListEditor()
{

}

////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////

void GenreListEditor::loadID3v1Genres()
{
    GenreList ID3v1List = GenreListList::ID3v1List();

    for(GenreList::Iterator it = ID3v1List.begin(); it != ID3v1List.end(); it++) {
	ID3v1Box->insertItem(*it);
    }
}

void GenreListEditor::loadLists()
{
    GenreListList lists = GenreListList::lists();

    for(GenreListList::Iterator it = lists.begin(); it != lists.end(); it++) {
	listDict.insert((*it).name(), &(*it));
	selectListBox->insertItem((*it).name());
    }
    
    updateGenreList();
}

void GenreListEditor::updateGenreList()
{
    GenreList *currentList = listDict[selectListBox->currentText()];
    if(currentList) {
	genreList->clear();

	for(GenreList::Iterator it = currentList->begin(); it != currentList->end(); it++)
	    new KListViewItem(genreList, *it, currentList->ID3v1Name((*it).getID3v1()));
    }
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void GenreListEditor::updateGenreBoxes(QListViewItem *item)
{
    if(item) {
	genreNameBox->setText(item->text(0));
	ID3v1Box->setCurrentItem(GenreListList::ID3v1List().findIndex(item->text(1)));
    }
}

void GenreListEditor::updateGenreName(const QString &name)
{
    QListViewItem *current = genreList->currentItem();
    if(!name.isEmpty() && current)
	current->setText(0, name);
}

#include "genrelisteditor.moc"
