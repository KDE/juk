/***************************************************************************
                          tageditor.cpp  -  description
                             -------------------
    begin                : Sat Sep 7 2002
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

#include <qlabel.h>
#include <qlayout.h>
#include <qdir.h>

#include "tageditor.h"
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

void TagEditor::readConfig()
{
    genreList = GenreListList::ID3v1List(); // this should later be read from a config file 
    if(genreList && genreBox) {
        genreBox->clear();
        // add values to the genre box
        genreBox->insertItem(QString::null);
        for(GenreList::Iterator it = genreList->begin(); it != genreList->end(); ++it)
            genreBox->insertItem((*it));
    }
}

void TagEditor::saveConfig()
{

}

void TagEditor::setupLayout()
{
    const int horizontalSpacing = 10;
    const int verticalSpacing = 2;

    QHBoxLayout *layout = new QHBoxLayout(this, 0, horizontalSpacing);

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
        leftColumnLayout->addWidget(new QLabel(i18n("Artist Name"), this));

        artistNameBox = new KComboBox(true, this, "artistNameBox");
        leftColumnLayout->addWidget(artistNameBox);

        leftColumnLayout->addWidget(new QLabel(i18n("Track Name"), this));

        trackNameBox = new KLineEdit(this, "trackNameBox");
        leftColumnLayout->addWidget(trackNameBox);

        leftColumnLayout->addWidget(new QLabel(i18n("Album Name"), this));

        albumNameBox = new KComboBox(true, this, "albumNameBox");
        leftColumnLayout->addWidget(albumNameBox);

        leftColumnLayout->addWidget(new QLabel(i18n("Genre"), this));

        genreBox = new KComboBox(true, this, "genreBox");

        leftColumnLayout->addWidget(genreBox);

        // this fills the space at the bottem of the left column
        leftColumnLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    }
    //////////////////////////////////////////////////////////////////////////////
    // put stuff in the right column
    //////////////////////////////////////////////////////////////////////////////
    { // just for organization
        rightColumnLayout->addWidget(new QLabel(i18n("File Name"), this));

        fileNameBox = new KLineEdit(this, "fileNameBox");
        rightColumnLayout->addWidget(fileNameBox);
        { // lay out the track row
            QHBoxLayout *trackRowLayout = new QHBoxLayout(rightColumnLayout, horizontalSpacing);

            trackRowLayout->addWidget(new QLabel(i18n("Track"), this));

            trackSpin = new KIntSpinBox(0, 255, 1, 0, 10, this, "trackSpin");
            trackRowLayout->addWidget(trackSpin);

            trackRowLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

            trackRowLayout->addWidget(new QLabel(i18n("Year"), this));

            yearSpin = new KIntSpinBox(0, 9999, 1, 0, 10, this, "yearSpin");
            trackRowLayout->addWidget(yearSpin);

            trackRowLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

            trackRowLayout->addWidget(new QLabel(i18n("Length"), this));

            lengthBox = new KLineEdit(this, "lengthBox");
            lengthBox->setMaximumWidth(50);
            lengthBox->setReadOnly(true);
            trackRowLayout->addWidget(lengthBox);

            trackRowLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

            trackRowLayout->addWidget(new QLabel(i18n("Bitrate"), this));

            bitrateBox = new KLineEdit(this, "bitrateBox");
            bitrateBox->setMaximumWidth(50);
            bitrateBox->setReadOnly(true);
            trackRowLayout->addWidget(bitrateBox);
        }
        rightColumnLayout->addWidget(new QLabel(i18n("Comment"), this));

        commentBox = new KEdit(this, "commentBox");
        rightColumnLayout->addWidget(commentBox);
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

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void TagEditor::setItems(const QPtrList<PlaylistItem> &list)
{
    saveChangesPrompt();
    items = list;
    refresh();
}

void TagEditor::refresh()
{
    // currently this only works for one item

    PlaylistItem *item = items.getFirst();

    if(item) {
	Tag *tag = item->getTag();
	AudioData *audioData = item->getAudioData();
	
	artistNameBox->setEditText(tag->getArtist());
	trackNameBox->setText(tag->getTrack());
	albumNameBox->setEditText(tag->getAlbum());
	
	if(genreList && genreList->findIndex(tag->getGenre()) >= 0)
	    genreBox->setCurrentItem(genreList->findIndex(tag->getGenre()) + 1);
	else {
	    genreBox->setCurrentItem(0);
	    genreBox->setEditText(tag->getGenre());
	}
	
	fileNameBox->setText(item->fileName());
	trackSpin->setValue(tag->getTrackNumber());
	yearSpin->setValue(tag->getYear());
	
	if(audioData->getResult()) {
	    lengthBox->setText(audioData->getLengthChar());
	    bitrateBox->setText(QString::number(audioData->getBitrate()));
	}
	else {
	    lengthBox->clear();
	    bitrateBox->clear();
	}
	
	commentBox->setText(tag->getComment());
	
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
    kdDebug() << "TagEditor::updateCollection()" << endl;
    CollectionList *list = CollectionList::instance();

    if(!list)
	return;
    
    if(artistNameBox->listBox()) {
        artistNameBox->listBox()->clear();
	
	// This is another case where a sorted value list would be useful.  It's
	// silly to build and maintain unsorted lists and have to call sort 
	// every time that you want to verify that a list is sorted.	

	QStringList artistList = list->getArtistList();
	artistList.sort();

        artistNameBox->listBox()->insertStringList(artistList);
    }

    if(albumNameBox->listBox()) {
        albumNameBox->listBox()->clear();

	QStringList albumList = list->getAlbumList();
	albumList.sort();

        albumNameBox->listBox()->insertStringList(albumList);
    }    
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void TagEditor::save(const QPtrList<PlaylistItem> &list)
{
    if(list.count() > 0) {
	
	// While this accepts a list of items, it currently only works for the first item.
	
        PlaylistItem *item = list.getFirst();
	
        if(item && dataChanged) {
            QFileInfo newFile(item->dirPath() + QDir::separator() + fileNameBox->text());
            QFileInfo directory(item->dirPath());

            // if (the new file is writable or the new file doesn't exist and it's directory is writable)
            // and the old file is writable...
            if((newFile.isWritable() || (!newFile.exists() && directory.isWritable())) && item->isWritable()) {
                // if the file name in the box doesn't match the current file name
                if(item->fileName()!=newFile.fileName()) {
                    // rename the file if it doesn't exist or we say it's ok
                    if(!newFile.exists() ||
                       KMessageBox::warningYesNo(this, i18n("This file already exists.\nDo you want to replace it?"),
                                                 i18n("File Exists")) == KMessageBox::Yes)
                    {
                        QDir currentDir;
                        currentDir.rename(item->filePath(), newFile.filePath());
                        item->setFile(newFile.filePath());
                    }
                }

                item->getTag()->setArtist(artistNameBox->currentText());
                item->getTag()->setTrack(trackNameBox->text());
                item->getTag()->setAlbum(albumNameBox->currentText());
                item->getTag()->setTrackNumber(trackSpin->value());
                item->getTag()->setYear(yearSpin->value());
                item->getTag()->setComment(commentBox->text());

                //  item->getTag()->setGenre(genreBox->currentText());
                //  item->getTag()->setGenre(genreBox->currentItem() - 1);
                if(genreList->findIndex(genreBox->currentText()) >= 0)
                    item->getTag()->setGenre((*genreList)[genreList->findIndex(genreBox->currentText())]);
                else
                    item->getTag()->setGenre(Genre(genreBox->currentText(), item->getTag()->getGenre().getID3v1()));


                item->getTag()->save();

                item->refresh();

                dataChanged = false;
            }
            else
                KMessageBox::sorry(this, i18n("Could not save to specified file."));
        }
    }
}

void TagEditor::saveChangesPrompt()
{
    if(dataChanged && !items.isEmpty()) {

        QString message = i18n("Do you want to save your changes to:\n");

        PlaylistItem *item = items.first();

        while(item) {
            message.append(item->fileName() + "\n");
            item = items.next();
        }

        if(KMessageBox::warningYesNo(this, message, i18n("Save Changes")) == KMessageBox::Yes)
            save(items);
    }
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void TagEditor::setDataChanged(bool c)
{
    dataChanged = c;
}

#include "tageditor.moc"
