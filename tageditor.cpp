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
#include <kpushbutton.h>

#include <qlabel.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qdir.h>

#include "tageditor.h"
#include "tag.h"
#include "tagguesser.h"
#include "collectionlist.h"
#include "genrelistlist.h"

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

TagEditor::TagEditor(QWidget *parent, const char *name) : QWidget(parent, name) 
{
    setupLayout();
    readConfig();
    m_dataChanged = false;
}

TagEditor::~TagEditor()
{
    saveConfig();
}

void TagEditor::setGenreList(const GenreList &list)
{
    m_genreList = list;
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void TagEditor::slotSetItems(const PlaylistItemList &list)
{
    saveChangesPrompt();
    m_items = list;
    if(isVisible())
	slotRefresh();
}

void TagEditor::slotRefresh()
{
    // This method takes the list of currently selected m_items and tries to 
    // figure out how to show that in the tag editor.  The current strategy --
    // the most common case -- is to just process the first item.  Then we
    // check after that to see if there are other m_items and adjust accordingly.

    PlaylistItem *item = m_items.getFirst();

    if(item) {
	Tag *tag = item->tag();
	
	m_artistNameBox->setEditText(tag->artist());
	m_trackNameBox->setText(tag->track());
	m_albumNameBox->setEditText(tag->album());
	
	if(m_genreList.findIndex(tag->genre()) >= 0)
	    m_genreBox->setCurrentItem(m_genreList.findIndex(tag->genre()) + 1);
	else {
	    m_genreBox->setCurrentItem(0);
	    m_genreBox->setEditText(tag->genre());
	}
	
	m_trackSpin->setValue(tag->trackNumber());
	m_yearSpin->setValue(tag->year());
	
	m_commentBox->setText(tag->comment());
	
	// Start at the second item, since we've already processed the first.

	QPtrListIterator<PlaylistItem> it(m_items);
	++it;

	// If there is more than one item in the m_items that we're dealing with...

	if(it.current()) {
	    m_fileNameBox->clear();
	    m_fileNameBox->setEnabled(false);

	    m_lengthBox->clear();
	    m_lengthBox->setEnabled(false);
	    
	    m_bitrateBox->clear();
	    m_bitrateBox->setEnabled(false);
	    
	    for(BoxMap::iterator boxIt = m_enableBoxes.begin(); boxIt != m_enableBoxes.end(); boxIt++) {
		(*boxIt)->setChecked(true);
		(*boxIt)->show();
	    }

	    // Yep, this is ugly.  Loop through all of the files checking to see
	    // if their fields are the same.  If so, by default, enable their 
	    // checkbox.
	    
	    // Also, if there are more than 50 m_items, don't scan all of them.
	    
	    if(m_items.count() > 50) {
		m_enableBoxes[m_artistNameBox]->setChecked(false);
		m_enableBoxes[m_trackNameBox]->setChecked(false);
		m_enableBoxes[m_albumNameBox]->setChecked(false);
		m_enableBoxes[m_genreBox]->setChecked(false);
		m_enableBoxes[m_trackSpin]->setChecked(false);
		m_enableBoxes[m_yearSpin]->setChecked(false);
		m_enableBoxes[m_commentBox]->setChecked(false);
	    }
	    else {
		for(; it.current(); ++it) {
		    tag = (*it)->tag();
		    if(m_artistNameBox->currentText() != tag->artist() && m_enableBoxes.contains(m_artistNameBox)) {
			m_artistNameBox->lineEdit()->clear();
			m_enableBoxes[m_artistNameBox]->setChecked(false);
		    }
		    if(m_trackNameBox->text() != tag->track() && m_enableBoxes.contains(m_trackNameBox)) {
			m_trackNameBox->clear();
			m_enableBoxes[m_trackNameBox]->setChecked(false);
		    }
		    if(m_albumNameBox->currentText() != tag->album() && m_enableBoxes.contains(m_albumNameBox)) {
			m_albumNameBox->lineEdit()->clear();
			m_enableBoxes[m_albumNameBox]->setChecked(false);
		    }
		    if(m_genreBox->currentText() != tag->genre() && m_enableBoxes.contains(m_genreBox)) {
			m_genreBox->lineEdit()->clear();
			m_enableBoxes[m_genreBox]->setChecked(false);
		    }		
		    
		    if(m_trackSpin->value() != tag->trackNumber() && m_enableBoxes.contains(m_trackSpin)) {
			m_trackSpin->setValue(0);
			m_enableBoxes[m_trackSpin]->setChecked(false);
		    }		
		    if(m_yearSpin->value() != tag->year() && m_enableBoxes.contains(m_yearSpin)) {
			m_yearSpin->setValue(0);
			m_enableBoxes[m_yearSpin]->setChecked(false);
		    }
		    
		    if(m_commentBox->text() != tag->comment() && m_enableBoxes.contains(m_commentBox)) {
			m_commentBox->clear();
			m_enableBoxes[m_commentBox]->setChecked(false);
		    }
		}
	    }
	}
	else {
	    // Clean up in the case that we are only handling one item.
	    
	    m_fileNameBox->setText(item->fileName());
	    m_fileNameBox->setEnabled(true);
	    
	    m_lengthBox->setText(tag->lengthString());
	    m_lengthBox->setEnabled(true);
	    
	    m_bitrateBox->setText(tag->bitrateString());
	    m_bitrateBox->setEnabled(true);	    

	    for(BoxMap::iterator boxIt = m_enableBoxes.begin(); boxIt != m_enableBoxes.end(); boxIt++) {
		(*boxIt)->setChecked(true);
		(*boxIt)->hide();
	    }
	}
	m_dataChanged = false;
    }
    else
	slotClear();
}

void TagEditor::slotClear()
{
    m_artistNameBox->lineEdit()->clear();
    m_trackNameBox->clear();
    m_albumNameBox->lineEdit()->clear();
    m_genreBox->setCurrentItem(0);
    m_fileNameBox->clear();
    m_trackSpin->setValue(0);
    m_yearSpin->setValue(0);
    m_lengthBox->clear();
    m_bitrateBox->clear();
    m_commentBox->clear();    
}

void TagEditor::slotUpdateCollection()
{
    CollectionList *list = CollectionList::instance();

    if(!list)
	return;
    
    if(m_artistNameBox->listBox()) {
        m_artistNameBox->listBox()->clear();
	
	// This is another case where a sorted value list would be useful.  It's
	// silly to build and maintain unsorted lists and have to call sort 
	// every time that you want to verify that a list is sorted.	

	QStringList artistList = list->artists();
	artistList.sort();

        m_artistNameBox->listBox()->insertStringList(artistList);
	m_artistNameBox->completionObject()->setItems(artistList);
    }

    if(m_albumNameBox->listBox()) {
        m_albumNameBox->listBox()->clear();

	QStringList albumList = list->albums();
	albumList.sort();

        m_albumNameBox->listBox()->insertStringList(albumList);
	m_albumNameBox->completionObject()->setItems(albumList);
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
        if(m_artistNameBox && m_albumNameBox) {
            KGlobalSettings::Completion artistNameBoxMode = 
		KGlobalSettings::Completion(config->readNumEntry("ArtistNameBoxMode", KGlobalSettings::CompletionAuto));
	    m_artistNameBox->setCompletionMode(artistNameBoxMode);
	    
            KGlobalSettings::Completion albumNameBoxMode = 
		KGlobalSettings::Completion(config->readNumEntry("AlbumNameBoxMode", KGlobalSettings::CompletionAuto));
	    m_albumNameBox->setCompletionMode(albumNameBoxMode);
        }
    }

    // Once the custom genre list editor is done, this is where we should read 
    // the genre list from the config file.

    m_genreList = GenreListList::ID3v1List();

    if(m_genreBox) {
        m_genreBox->clear();

        // Add values to the genre box

        m_genreBox->insertItem(QString::null);

        for(GenreList::Iterator it = m_genreList.begin(); it != m_genreList.end(); ++it)
            m_genreBox->insertItem((*it));
    }
}

void TagEditor::saveConfig()
{
    KConfig *config = KGlobal::config();
    { // combo box completion modes
        KConfigGroupSaver saver(config, "TagEditor");
        if(m_artistNameBox && m_albumNameBox) {
	    config->writeEntry("ArtistNameBoxMode", m_artistNameBox->completionMode());
	    config->writeEntry("AlbumNameBoxMode", m_albumNameBox->completionMode());
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
        m_artistNameBox = new KComboBox(true, this, "artistNameBox");
	m_artistNameBox->setCompletionMode(KGlobalSettings::CompletionAuto);
	addItem(i18n("Artist name:"), m_artistNameBox, leftColumnLayout);

        m_trackNameBox = new KLineEdit(this, "trackNameBox");
	addItem(i18n("Track name:"), m_trackNameBox, leftColumnLayout);

        m_albumNameBox = new KComboBox(true, this, "albumNameBox");
	m_albumNameBox->setCompletionMode(KGlobalSettings::CompletionAuto);
	addItem(i18n("Album name:"), m_albumNameBox, leftColumnLayout);

        m_genreBox = new KComboBox(true, this, "genreBox");
	addItem(i18n("Genre:"), m_genreBox, leftColumnLayout);

        // this fills the space at the bottem of the left column
        leftColumnLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    }
    //////////////////////////////////////////////////////////////////////////////
    // put stuff in the right column
    //////////////////////////////////////////////////////////////////////////////
    { // just for organization
        rightColumnLayout->addWidget(new QLabel(i18n("File name:"), this));

        m_fileNameBox = new KLineEdit(this, "fileNameBox");
        rightColumnLayout->addWidget(m_fileNameBox);

        { // lay out the track row
            QHBoxLayout *trackRowLayout = new QHBoxLayout(rightColumnLayout, horizontalSpacing);

            m_trackSpin = new KIntSpinBox(0, 255, 1, 0, 10, this, "trackSpin");
	    addItem(i18n("Track:"), m_trackSpin, trackRowLayout);

            trackRowLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

            m_yearSpin = new KIntSpinBox(0, 9999, 1, 0, 10, this, "yearSpin");
	    addItem(i18n("Year:"), m_yearSpin, trackRowLayout);

            trackRowLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

            trackRowLayout->addWidget(new QLabel(i18n("Length:"), this));
            m_lengthBox = new KLineEdit(this, "lengthBox");
            m_lengthBox->setMaximumWidth(50);
            m_lengthBox->setReadOnly(true);
            trackRowLayout->addWidget(m_lengthBox);

            trackRowLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

            trackRowLayout->addWidget(new QLabel(i18n("Bitrate:"), this));
            m_bitrateBox = new KLineEdit(this, "bitrateBox");
            m_bitrateBox->setMaximumWidth(50);
            m_bitrateBox->setReadOnly(true);
            trackRowLayout->addWidget(m_bitrateBox);
        }

        m_commentBox = new KEdit(this, "commentBox");
	m_commentBox->setTextFormat(Qt::PlainText);
	addItem(i18n("Comment:"), m_commentBox, rightColumnLayout);

        m_suggestButton = new KPushButton(i18n("S&uggest"), this, "suggestButton");
        rightColumnLayout->addWidget(m_suggestButton, 0 /*no stretching */, Qt::AlignRight);
    }

    connect(m_artistNameBox, SIGNAL(textChanged(const QString&)), this, SLOT(slotDataChanged()));
    connect(m_trackNameBox, SIGNAL(textChanged(const QString&)), this, SLOT(slotDataChanged()));
    connect(m_albumNameBox, SIGNAL(textChanged(const QString&)), this, SLOT(slotDataChanged()));
    connect(m_genreBox, SIGNAL(activated(int)), this, SLOT(slotDataChanged()));
    connect(m_genreBox, SIGNAL(textChanged(const QString&)), this, SLOT(slotDataChanged()));
    connect(m_fileNameBox, SIGNAL(textChanged(const QString&)), this, SLOT(slotDataChanged()));
    connect(m_yearSpin, SIGNAL(valueChanged(int)), this, SLOT(slotDataChanged()));
    connect(m_trackSpin, SIGNAL(valueChanged(int)), this, SLOT(slotDataChanged()));
    connect(m_commentBox, SIGNAL(textChanged()), this, SLOT(slotDataChanged()));
    connect(m_suggestButton, SIGNAL(clicked()), this, SLOT(slotSuggestClicked()));
}

void TagEditor::save(const PlaylistItemList &list)
{
    if(list.count() > 0 && m_dataChanged) {
	
	KApplication::setOverrideCursor(Qt::waitCursor);

	// To keep track of the files that don't cooperate...

	QStringList errorFiles;
	
	for(QPtrListIterator<PlaylistItem> it(list); it.current(); ++it) {
	    PlaylistItem *item = *it;
	    
	    QFileInfo newFile(item->dirPath() + QDir::separator() + m_fileNameBox->text());
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
		
		if(m_enableBoxes[m_artistNameBox]->isOn())
		    item->tag()->setArtist(m_artistNameBox->currentText());
		if(m_enableBoxes[m_trackNameBox]->isOn())
		    item->tag()->setTrack(m_trackNameBox->text());
		if(m_enableBoxes[m_albumNameBox]->isOn())
		    item->tag()->setAlbum(m_albumNameBox->currentText());
		if(m_enableBoxes[m_trackSpin]->isOn())
		    item->tag()->setTrackNumber(m_trackSpin->value());
		if(m_enableBoxes[m_yearSpin]->isOn())
		    item->tag()->setYear(m_yearSpin->value());
		if(m_enableBoxes[m_commentBox]->isOn())
		    item->tag()->setComment(m_commentBox->text());
		
		if(m_enableBoxes[m_genreBox]->isOn()) {
		    if(m_genreList.findIndex(m_genreBox->currentText()) >= 0)
			item->tag()->setGenre(m_genreList[m_genreList.findIndex(m_genreBox->currentText())]);
		    else
			item->tag()->setGenre(Genre(m_genreBox->currentText(), item->tag()->genre().getID3v1()));
		}
		
		item->tag()->save();
		
		item->slotRefresh();
	    }
	    else
		errorFiles.append(item->fileName());
	}
	
	if(!errorFiles.isEmpty())
	    KMessageBox::detailedSorry(this,
				       i18n("Could not save to specified file(s)."), 
				       i18n("Could Not Write to:\n") + errorFiles.join("\n"));
	m_dataChanged = false;

	KApplication::restoreOverrideCursor();
    }
}

void TagEditor::saveChangesPrompt()
{
    if(isVisible() && m_dataChanged && !m_items.isEmpty()) {

	QStringList files;
        PlaylistItem *item = m_items.first();
        while(item) {
            files.append(item->fileName());
            item = m_items.next();
        }

        if(KMessageBox::questionYesNoList(this,
					  i18n("Do you want to save your changes to:\n"), 
					  files, 
					  i18n("Save Changes")) == KMessageBox::Yes) {
            save(m_items);
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
    m_enableBoxes.insert(item, enableBox);
}

void TagEditor::showEvent(QShowEvent *e)
{
    slotRefresh();
    QWidget::showEvent(e);
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void TagEditor::slotDataChanged(bool c)
{
    m_dataChanged = c;
}

void TagEditor::slotSuggestClicked()
{
    PlaylistItem *item = m_items.getFirst();
    if(!item)
        return;

    Tag *tag = item->tag();
    Q_ASSERT(tag);
    TagGuesser guesser(tag->absFilePath());

    if(!guesser.title().isNull())
        m_trackNameBox->setText(guesser.title());
    if(!guesser.artist().isNull())
        m_artistNameBox->setEditText(guesser.artist());
    if(!guesser.album().isNull())
        m_albumNameBox->setEditText(guesser.album());
    if(!guesser.track().isNull())
        m_trackSpin->setValue(guesser.track().toInt());
    if(!guesser.comment().isNull())
        m_commentBox->setText(guesser.comment());
}

#include "tageditor.moc"
