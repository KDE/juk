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

#include <qnamespace.h>
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
  changed = false;
  setupLayout();
  readConfig();
}

TaggerWidget::~TaggerWidget() {}

void TaggerWidget::add(QString item)
{
  taggerList->append(item);
}

void TaggerWidget::add(QStringList *items)
{
  taggerList->append(items);
}

FileListItem *TaggerWidget::getSelectedItem()
{
  if(taggerList && taggerList->getSelectedItem())
    return(taggerList->getSelectedItem());
  else
    return(0);
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void TaggerWidget::save()
{
  kdDebug() << "TaggerWidget::save()" << endl;
  QPtrList<QListViewItem> items = taggerList->selectedItems();

  if(items.count() > 0) {

    FileListItem *item = dynamic_cast<FileListItem *>(items.first());
    
    if(item && changed) {
      QFileInfo newFile(item->getFileInfo()->dirPath() + QDir::separator() + fileNameBox->text());
      QFileInfo directory(item->getFileInfo()->dirPath());

      // if (the new file is writable or the new file doesn't exist and it's directory is writable)
      // and the old file is writable...
      if((newFile.isWritable() || (!newFile.exists() && directory.isWritable())) && item->getFileInfo()->isWritable()) {
	// if the file name in the box doesn't match the current file name
	if(item->getFileInfo()->fileName()!=newFile.fileName()) {
	  // rename the file if it doesn't exist or we say it's ok
	  if(!newFile.exists() ||
	     KMessageBox::warningYesNo(this, i18n("This file already exists.\nDo you want to replace it?"), 
				  i18n("File Exists")) == KMessageBox::Yes) 
	    {
	    QDir currentDir;
	    currentDir.rename(item->getFileInfo()->filePath(), newFile.filePath());
	    item->setFile(newFile.filePath());
	  }
	}

	item->getTag()->setArtist(artistNameBox->currentText());
	item->getTag()->setTrack(trackNameBox->text());
	item->getTag()->setAlbum(albumNameBox->currentText());
	//  item->getTag()->setGenre(genreBox->currentText());
	//  item->getTag()->setGenre(genreBox->currentItem() - 1);
	item->getTag()->setTrackNumber(trackSpin->value());
	item->getTag()->setYear(yearSpin->value());
	item->getTag()->setComment(commentBox->text());

	item->getTag()->save();
	
	item->refresh();

	changed = false;
      }
      else {
	KMessageBox::sorry(this, i18n("Could not save to specified file."));
      }

      changed = false;
    }
  }
}

void TaggerWidget::setChanged()
{
  changed = true;
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void TaggerWidget::setupLayout()
{
  const int horizontalSpacing = 10;
  const int verticalSpacing = 2;

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
  taggerList = new FileList(split, "taggerList");

  // this connection is a little ugly -- it does a cast
  connect(taggerList, SIGNAL(selectionChanged()), this, SLOT(updateBoxes()));

  //////////////////////////////////////////////////////////////////////////////
  // now set up a bottom widget of the splitter and make it stay small at 
  // start up.  also define a bottem layout to organize things in
  //////////////////////////////////////////////////////////////////////////////
  QWidget *bottem = new QWidget(split);
  split->setResizeMode(bottem, QSplitter::FollowSizeHint);
  QHBoxLayout *bottemLayout = new QHBoxLayout(bottem, 0, horizontalSpacing);

  //////////////////////////////////////////////////////////////////////////////
  // define two columns of the bottem layout
  //////////////////////////////////////////////////////////////////////////////
  QVBoxLayout *leftColumnLayout = new QVBoxLayout(bottemLayout, verticalSpacing);
  QVBoxLayout *rightColumnLayout = new QVBoxLayout(bottemLayout, verticalSpacing);

  bottemLayout->setStretchFactor(leftColumnLayout, 2);
  bottemLayout->setStretchFactor(rightColumnLayout, 3);

  //////////////////////////////////////////////////////////////////////////////
  // put stuff in the left column -- all of the field names are class wide
  //////////////////////////////////////////////////////////////////////////////
  { // just for organization
    leftColumnLayout->addWidget(new QLabel(i18n("Artist Name"), bottem));
    
    artistNameBox = new KComboBox(true, bottem, "artistNameBox");
    leftColumnLayout->addWidget(artistNameBox);
    
    leftColumnLayout->addWidget(new QLabel(i18n("Track Name"), bottem));
    
    trackNameBox = new KLineEdit(bottem, "trackNameBox");
    leftColumnLayout->addWidget(trackNameBox);
    
    leftColumnLayout->addWidget(new QLabel(i18n("Album Name"), bottem));
    
    albumNameBox = new KComboBox(true, bottem, "albumNameBox");
    leftColumnLayout->addWidget(albumNameBox);
    
    leftColumnLayout->addWidget(new QLabel(i18n("Genre"), bottem));
    
    genreBox = new KComboBox(false, bottem, "genreBox");
    
    leftColumnLayout->addWidget(genreBox);
    
    // this fills the space at the bottem of the left column
    leftColumnLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
  }
  //////////////////////////////////////////////////////////////////////////////
  // put stuff in the right column
  //////////////////////////////////////////////////////////////////////////////
  { // just for organization
    rightColumnLayout->addWidget(new QLabel(i18n("File Name"), bottem));

    fileNameBox = new KLineEdit(bottem, "fileNameBox");
    rightColumnLayout->addWidget(fileNameBox);
    { // lay out the track row
      QHBoxLayout *trackRowLayout = new QHBoxLayout(rightColumnLayout, horizontalSpacing);

      trackRowLayout->addWidget(new QLabel(i18n("Track"), bottem));
      
      trackSpin = new KIntSpinBox(0, 255, 1, 0, 10, bottem, "trackSpin");
      trackRowLayout->addWidget(trackSpin);

      trackRowLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

      trackRowLayout->addWidget(new QLabel(i18n("Year"), bottem));
      
      yearSpin = new KIntSpinBox(0, 9999, 1, 0, 10, bottem, "yearSpin");
      trackRowLayout->addWidget(yearSpin);

      trackRowLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

      trackRowLayout->addWidget(new QLabel(i18n("Length"), bottem));

      lengthBox = new KLineEdit(bottem, "lengthBox");
      lengthBox->setMaximumWidth(50);
      trackRowLayout->addWidget(lengthBox);

      trackRowLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

      trackRowLayout->addWidget(new QLabel(i18n("Bitrate"), bottem));

      bitrateBox = new KLineEdit(bottem, "bitrateBox");
      bitrateBox->setMaximumWidth(50);
      trackRowLayout->addWidget(bitrateBox);
    }
    rightColumnLayout->addWidget(new QLabel(i18n("Comment"), bottem));
    
    commentBox = new KEdit(bottem, "commentBox");
    rightColumnLayout->addWidget(commentBox);
  }
  
  connect(artistNameBox, SIGNAL(textChanged(const QString&)), this, SLOT(setChanged()));
  connect(trackNameBox, SIGNAL(textChanged(const QString&)), this, SLOT(setChanged()));
  connect(albumNameBox, SIGNAL(textChanged(const QString&)), this, SLOT(setChanged()));
  connect(genreBox, SIGNAL(activated(const QString&)), this, SLOT(setChanged()));
  connect(fileNameBox, SIGNAL(textChanged(const QString&)), this, SLOT(setChanged()));
  connect(yearSpin, SIGNAL(valueChanged(int)), this, SLOT(setChanged()));
  connect(trackSpin, SIGNAL(valueChanged(int)), this, SLOT(setChanged()));
  connect(commentBox, SIGNAL(textChanged()), this, SLOT(setChanged()));
}

void TaggerWidget::readConfig()
{
  genreList = GenreListList::id3v1List(); // this should later be read from a config file
  if(genreList && genreBox) {
    genreBox->clear();
    // add values to the genre box
    genreBox->insertItem(QString::null);
    for(GenreList::Iterator it = genreList->begin(); it != genreList->end(); ++it)
      genreBox->insertItem((*it));     
  }
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void TaggerWidget::updateBoxes() // this needs to be updated to properly work with multiple selections
{
  //  kdDebug() << "updateBoxes(): " << item->getFileInfo()->filePath() << endl;

  QPtrList<QListViewItem> items = taggerList->selectedItems();

  if(items.count() > 0) {

    FileListItem *item = dynamic_cast<FileListItem *>(items.first());

    if(item) {
      Tag *tag = item->getTag();
      QFileInfo *fileInfo = item->getFileInfo();
      MPEGHeader *header = item->getHeader();
      
      artistNameBox->setEditText(tag->getArtist());
      trackNameBox->setText(tag->getTrack());
      albumNameBox->setEditText(tag->getAlbum());
      
      if(genreList && genreList->findIndex(tag->getGenre()) >= 0)
	genreBox->setCurrentItem(genreList->findIndex(tag->getGenre()) + 1);
      else 
	genreBox->setCurrentItem(0);
      
      fileNameBox->setText(fileInfo->fileName());
      trackSpin->setValue(tag->getTrackNumber());
      yearSpin->setValue(tag->getYear());
      
      if(header->getResult()) {
	lengthBox->setText(header->getLengthChar());
	bitrateBox->setText(QString::number(header->getBitrate()));
      }
      
      commentBox->setText(tag->getComment());
      
      changed = false;
    }
  }
}
