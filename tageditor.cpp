/***************************************************************************
                          tageditor.cpp  -  description
                             -------------------
    begin                : Sat Sep 7 2002
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

#include <kapplication.h>
#include <kcombobox.h>
#include <klineedit.h>
#include <knuminput.h>
#include <keditcl.h>
#include <kmessagebox.h>
#include <kconfig.h>
#include <klocale.h>
#include <kdebug.h>

#include <qlabel.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qdir.h>

#include "tageditor.h"
#include "tag.h"
#include "collectionlist.h"
#include "genrelistlist.h"

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

TagEditor::TagEditor(QWidget *parent, const char *name) : QWidget(parent, name) 
{
    setupLayout();
    readConfig();
    dataChanged = false;
}

TagEditor::~TagEditor()
{
    saveConfig();
}

void TagEditor::setGenreList(const GenreList &list)
{
    genreList = list;
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void TagEditor::setItems(const PlaylistItemList &list)
{
    saveChangesPrompt();
    items = list;
    if(isVisible())
	refresh();
}

void TagEditor::refresh()
{
    // This method takes the list of currently selected items and tries to 
    // figure out how to show that in the tag editor.  The current strategy --
    // the most common case -- is to just process the first item.  Then we
    // check after that to see if there are other items and adjust accordingly.

    PlaylistItem *item = items.getFirst();

    if(item) {
	Tag *tag = item->tag();
	
	artistNameBox->setEditText(tag->artist());
	trackNameBox->setText(tag->track());
	albumNameBox->setEditText(tag->album());
	
	if(genreList.findIndex(tag->genre()) >= 0)
	    genreBox->setCurrentItem(genreList.findIndex(tag->genre()) + 1);
	else {
	    genreBox->setCurrentItem(0);
	    genreBox->setEditText(tag->genre());
	}
	
	trackSpin->setValue(tag->trackNumber());
	yearSpin->setValue(tag->year());
	
	commentBox->setText(tag->comment());
	
	// Start at the second item, since we've already processed the first.

	QPtrListIterator<PlaylistItem> it(items);
	++it;

	// If there is more than one item in the items that we're dealing with...

	if(it.current()) {
	    fileNameBox->clear();
	    fileNameBox->setEnabled(false);

	    lengthBox->clear();
	    lengthBox->setEnabled(false);
	    
	    bitrateBox->clear();
	    bitrateBox->setEnabled(false);
	    
	    for(BoxMap::iterator boxIt = enableBoxes.begin(); boxIt != enableBoxes.end(); boxIt++) {
		(*boxIt)->setChecked(true);
		(*boxIt)->show();
	    }

	    // Yep, this is ugly.  Loop through all of the files checking to see
	    // if their fields are the same.  If so, by default, enable their 
	    // checkbox.
	    
	    // Also, if there are more than 50 items, don't scan all of them.
	    
	    if(items.count() > 50) {
		enableBoxes[artistNameBox]->setChecked(false);
		enableBoxes[trackNameBox]->setChecked(false);
		enableBoxes[albumNameBox]->setChecked(false);
		enableBoxes[genreBox]->setChecked(false);
		enableBoxes[trackSpin]->setChecked(false);
		enableBoxes[yearSpin]->setChecked(false);
		enableBoxes[commentBox]->setChecked(false);
	    }
	    else {
		for(; it.current(); ++it) {
		    tag = (*it)->tag();
		    if(artistNameBox->currentText() != tag->artist() && enableBoxes.contains(artistNameBox)) {
			artistNameBox->lineEdit()->clear();
			enableBoxes[artistNameBox]->setChecked(false);
		    }
		    if(trackNameBox->text() != tag->track() && enableBoxes.contains(trackNameBox)) {
			trackNameBox->clear();
			enableBoxes[trackNameBox]->setChecked(false);
		    }
		    if(albumNameBox->currentText() != tag->album() && enableBoxes.contains(albumNameBox)) {
			albumNameBox->lineEdit()->clear();
			enableBoxes[albumNameBox]->setChecked(false);
		    }
		    if(genreBox->currentText() != tag->genre() && enableBoxes.contains(genreBox)) {
			genreBox->lineEdit()->clear();
			enableBoxes[genreBox]->setChecked(false);
		    }		
		    
		    if(trackSpin->value() != tag->trackNumber() && enableBoxes.contains(trackSpin)) {
			trackSpin->setValue(0);
			enableBoxes[trackSpin]->setChecked(false);
		    }		
		    if(yearSpin->value() != tag->year() && enableBoxes.contains(yearSpin)) {
			yearSpin->setValue(0);
			enableBoxes[yearSpin]->setChecked(false);
		    }
		    
		    if(commentBox->text() != tag->comment() && enableBoxes.contains(commentBox)) {
			commentBox->clear();
			enableBoxes[commentBox]->setChecked(false);
		    }
		}
	    }
	}
	else {
	    // Clean up in the case that we are only handling one item.
	    
	    fileNameBox->setText(item->fileName());
	    fileNameBox->setEnabled(true);
	    
	    lengthBox->setText(tag->lengthString());
	    lengthBox->setEnabled(true);
	    
	    bitrateBox->setText(tag->bitrateString());
	    bitrateBox->setEnabled(true);	    

	    for(BoxMap::iterator boxIt = enableBoxes.begin(); boxIt != enableBoxes.end(); boxIt++) {
		(*boxIt)->setChecked(true);
		(*boxIt)->hide();
	    }
	}
	dataChanged = false;
    }
    else
	clear();
}

void TagEditor::clear()
{
    artistNameBox->lineEdit()->clear();
    trackNameBox->clear();
    albumNameBox->lineEdit()->clear();
    genreBox->setCurrentItem(0);
    fileNameBox->clear();
    trackSpin->setValue(0);
    yearSpin->setValue(0);
    lengthBox->clear();
    bitrateBox->clear();
    commentBox->clear();    
}

void TagEditor::save()
{
    save(items);
}

void TagEditor::updateCollection()
{
    CollectionList *list = CollectionList::instance();

    if(!list)
	return;
    
    if(artistNameBox->listBox()) {
        artistNameBox->listBox()->clear();
	
	// This is another case where a sorted value list would be useful.  It's
	// silly to build and maintain unsorted lists and have to call sort 
	// every time that you want to verify that a list is sorted.	

	QStringList artistList = list->artists();
	artistList.sort();

        artistNameBox->listBox()->insertStringList(artistList);
	artistNameBox->completionObject()->setItems(artistList);
    }

    if(albumNameBox->listBox()) {
        albumNameBox->listBox()->clear();

	QStringList albumList = list->albums();
	albumList.sort();

        albumNameBox->listBox()->insertStringList(albumList);
	albumNameBox->completionObject()->setItems(albumList);
    }    
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void TagEditor::readConfig()
{
    KConfig *config = KGlobal::config();
    { // combo box completion modes
        KConfigGroupSaver saver(config, "TagEditor");
        if(artistNameBox && albumNameBox) {
            KGlobalSettings::Completion artistNameBoxMode = 
		KGlobalSettings::Completion(config->readNumEntry("ArtistNameBoxMode", KGlobalSettings::CompletionAuto));
	    artistNameBox->setCompletionMode(artistNameBoxMode);
	    
            KGlobalSettings::Completion albumNameBoxMode = 
		KGlobalSettings::Completion(config->readNumEntry("AlbumNameBoxMode", KGlobalSettings::CompletionAuto));
	    albumNameBox->setCompletionMode(albumNameBoxMode);
        }
    }

    // Once the custom genre list editor is done, this is where we should read 
    // the genre list from the config file.

    genreList = GenreListList::ID3v1List();

    if(genreBox) {
        genreBox->clear();

        // Add values to the genre box

        genreBox->insertItem(QString::null);

        for(GenreList::Iterator it = genreList.begin(); it != genreList.end(); ++it)
            genreBox->insertItem((*it));
    }
}

void TagEditor::saveConfig()
{
    KConfig *config = KGlobal::config();
    { // combo box completion modes
        KConfigGroupSaver saver(config, "TagEditor");
        if(artistNameBox && albumNameBox) {
	    config->writeEntry("ArtistNameBoxMode", artistNameBox->completionMode());
	    config->writeEntry("AlbumNameBoxMode", albumNameBox->completionMode());
        }
    }

}

void TagEditor::setupLayout()
{
    static const int horizontalSpacing = 10;
    static const int verticalSpacing = 2;

    QHBoxLayout *layout = new QHBoxLayout(this, 2, horizontalSpacing);

    //////////////////////////////////////////////////////////////////////////////
    // define two columns of the bottem layout
    //////////////////////////////////////////////////////////////////////////////
    QVBoxLayout *leftColumnLayout = new QVBoxLayout(layout, verticalSpacing);
    QVBoxLayout *rightColumnLayout = new QVBoxLayout(layout, verticalSpacing);

    layout->setStretchFactor(leftColumnLayout, 2);
    layout->setStretchFactor(rightColumnLayout, 3);

    //////////////////////////////////////////////////////////////////////////////
    // put stuff in the left column -- all of the field names are class wide
    //////////////////////////////////////////////////////////////////////////////
    { // just for organization
        artistNameBox = new KComboBox(true, this, "artistNameBox");
	artistNameBox->setCompletionMode(KGlobalSettings::CompletionAuto);
	addItem(i18n("Artist name:"), artistNameBox, leftColumnLayout);

        trackNameBox = new KLineEdit(this, "trackNameBox");
	addItem(i18n("Track name:"), trackNameBox, leftColumnLayout);

        albumNameBox = new KComboBox(true, this, "albumNameBox");
	albumNameBox->setCompletionMode(KGlobalSettings::CompletionAuto);
	addItem(i18n("Album name:"), albumNameBox, leftColumnLayout);

        genreBox = new KComboBox(true, this, "genreBox");
	addItem(i18n("Genre:"), genreBox, leftColumnLayout);

        // this fills the space at the bottem of the left column
        leftColumnLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    }
    //////////////////////////////////////////////////////////////////////////////
    // put stuff in the right column
    //////////////////////////////////////////////////////////////////////////////
    { // just for organization
        rightColumnLayout->addWidget(new QLabel(i18n("File name:"), this));

        fileNameBox = new KLineEdit(this, "fileNameBox");
        rightColumnLayout->addWidget(fileNameBox);

        { // lay out the track row
            QHBoxLayout *trackRowLayout = new QHBoxLayout(rightColumnLayout, horizontalSpacing);

            trackSpin = new KIntSpinBox(0, 255, 1, 0, 10, this, "trackSpin");
	    addItem(i18n("Track:"), trackSpin, trackRowLayout);

            trackRowLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

            yearSpin = new KIntSpinBox(0, 9999, 1, 0, 10, this, "yearSpin");
	    addItem(i18n("Year:"), yearSpin, trackRowLayout);

            trackRowLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

            trackRowLayout->addWidget(new QLabel(i18n("Length:"), this));
            lengthBox = new KLineEdit(this, "lengthBox");
            lengthBox->setMaximumWidth(50);
            lengthBox->setReadOnly(true);
            trackRowLayout->addWidget(lengthBox);

            trackRowLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

            trackRowLayout->addWidget(new QLabel(i18n("Bitrate:"), this));
            bitrateBox = new KLineEdit(this, "bitrateBox");
            bitrateBox->setMaximumWidth(50);
            bitrateBox->setReadOnly(true);
            trackRowLayout->addWidget(bitrateBox);
        }

        commentBox = new KEdit(this, "commentBox");
	commentBox->setTextFormat(Qt::PlainText);
	addItem(i18n("Comment:"), commentBox, rightColumnLayout);
    }

    connect(artistNameBox, SIGNAL(textChanged(const QString&)), this, SIGNAL(changed()));
    connect(trackNameBox, SIGNAL(textChanged(const QString&)), this, SIGNAL(changed()));
    connect(albumNameBox, SIGNAL(textChanged(const QString&)), this, SIGNAL(changed()));
    connect(genreBox, SIGNAL(activated(int)), this, SIGNAL(changed()));
    connect(genreBox, SIGNAL(textChanged(const QString&)), this, SIGNAL(changed()));
    connect(fileNameBox, SIGNAL(textChanged(const QString&)), this, SIGNAL(changed()));
    connect(yearSpin, SIGNAL(valueChanged(int)), this, SIGNAL(changed()));
    connect(trackSpin, SIGNAL(valueChanged(int)), this, SIGNAL(changed()));
    connect(commentBox, SIGNAL(textChanged()), this, SIGNAL(changed()));

    connect(this, SIGNAL(changed()), this, SLOT(setDataChanged()));
}

void TagEditor::save(const PlaylistItemList &list)
{
    if(list.count() > 0 && dataChanged) {
	
	KApplication::setOverrideCursor(Qt::waitCursor);

	// To keep track of the files that don't cooperate...

	QStringList errorFiles;
	
	for(QPtrListIterator<PlaylistItem> it(list); it.current(); ++it) {
	    PlaylistItem *item = *it;
	    
	    QFileInfo newFile(item->dirPath() + QDir::separator() + fileNameBox->text());
	    QFileInfo directory(item->dirPath());
	    
	    // If (the new file is writable or the new file doesn't exist and
	    // it's directory is writable) and the old file is writable...  
	    // If not we'll append it to errorFiles to tell the user which
	    // files we couldn't write to.
	    
	    if((newFile.isWritable() || (!newFile.exists() && directory.isWritable())) && item->isWritable()) {
		
		// If the file name in the box doesn't match the current file
		// name...
		
		if(list.count() == 1 && item->fileName() != newFile.fileName()) {
		    
		    // Rename the file if it doesn't exist or the user says
		    // that it's ok.
		    
		    if(!newFile.exists() ||
		       KMessageBox::warningYesNo(this, i18n("This file already exists.\nDo you want to replace it?"),
						 i18n("File Exists")) == KMessageBox::Yes)
		    {
			QDir currentDir;
			currentDir.rename(item->filePath(), newFile.filePath());
			item->setFile(newFile.filePath());
		    }
		}
		
		// A bit more ugliness.  If there are multiple files that are
		// being modified, they each have a "enabled" checkbox that
		// says if that field is to be respected for the multiple 
		// files.  We have to check to see if that is enabled before
		// each field that we write.
		
		if(enableBoxes[artistNameBox]->isOn())
		    item->tag()->setArtist(artistNameBox->currentText());
		if(enableBoxes[trackNameBox]->isOn())
		    item->tag()->setTrack(trackNameBox->text());
		if(enableBoxes[albumNameBox]->isOn())
		    item->tag()->setAlbum(albumNameBox->currentText());
		if(enableBoxes[trackSpin]->isOn())
		    item->tag()->setTrackNumber(trackSpin->value());
		if(enableBoxes[yearSpin]->isOn())
		    item->tag()->setYear(yearSpin->value());
		if(enableBoxes[commentBox]->isOn())
		    item->tag()->setComment(commentBox->text());
		
		if(enableBoxes[genreBox]->isOn()) {
		    if(genreList.findIndex(genreBox->currentText()) >= 0)
			item->tag()->setGenre(genreList[genreList.findIndex(genreBox->currentText())]);
		    else
			item->tag()->setGenre(Genre(genreBox->currentText(), item->tag()->genre().getID3v1()));
		}
		
		item->tag()->save();
		
		item->refresh();
	    }
	    else
		errorFiles.append(item->fileName());
	}
	
	if(!errorFiles.isEmpty())
	    KMessageBox::detailedSorry(this,
				       i18n("Could not save to specified file(s)."), 
				       i18n("Could Not Write to:\n") + errorFiles.join("\n"));
	dataChanged = false;

	KApplication::restoreOverrideCursor();
    }
}

void TagEditor::saveChangesPrompt()
{
    if(isVisible() && dataChanged && !items.isEmpty()) {

	QStringList files;
        PlaylistItem *item = items.first();
        while(item) {
            files.append(item->fileName());
            item = items.next();
        }

        if(KMessageBox::questionYesNoList(this,
					  i18n("Do you want to save your changes to:\n"), 
					  files, 
					  i18n("Save Changes")) == KMessageBox::Yes) {
            save(items);
	}
    }
}

void TagEditor::addItem(const QString &text, QWidget *item, QBoxLayout *layout)
{
    if(!item || !layout)
	return;

    QLabel *label = new QLabel(text, this);

    QCheckBox *enableBox = new QCheckBox(i18n("Enable"), this);
    enableBox->setChecked(true);

    label->setMinimumHeight(enableBox->height());

    if(layout->direction() == QBoxLayout::LeftToRight) {
	layout->addWidget(label);
	layout->addWidget(item);
	layout->addWidget(enableBox);
    }
    else {
	QHBoxLayout *l = new QHBoxLayout(layout);

	l->addWidget(label);
	l->setStretchFactor(label, 1);

	l->insertStretch(-1, 1);

	l->addWidget(enableBox);
	l->setStretchFactor(enableBox, 0);

	layout->addWidget(item);
    }

    enableBox->hide();

    connect(enableBox, SIGNAL(toggled(bool)), item, SLOT(setEnabled(bool)));
    enableBoxes.insert(item, enableBox);
}

void TagEditor::showEvent(QShowEvent *e)
{
    refresh();
    QWidget::showEvent(e);
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void TagEditor::setDataChanged(bool c)
{
    dataChanged = c;
}

#include "tageditor.moc"
