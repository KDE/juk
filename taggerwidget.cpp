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

#include <klocale.h>
#include <kdebug.h>

#include <qnamespace.h>
#include <qsplitter.h>
#include <qvbox.h>
#include <qhbox.h>
#include <qlayout.h>
#include <qlabel.h>

#include "taggerwidget.h"
#include "genrelistlist.h"

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

TaggerWidget::TaggerWidget(QWidget *parent) : QWidget(parent)
{
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
  connect(taggerList, SIGNAL(selectionChanged(QListViewItem *)), this, SLOT(updateBoxes(FileListItem *)));

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

void TaggerWidget::updateBoxes(FileListItem *item)
{
  //  kdDebug() << "updateBoxes(): " << item->getFileInfo()->filePath() << endl;

  Tag *tag = item->getTag();
  QFileInfo *fileInfo = item->getFileInfo();
  MPEGHeader *header = item->getHeader();

  artistNameBox->setEditText(tag->getArtist());
  trackNameBox->setText(tag->getTrack());
  albumNameBox->setEditText(tag->getAlbum());
  genreBox->setCurrentItem(tag->getGenreNumber() + 1);
  fileNameBox->setText(fileInfo->fileName());
  trackSpin->setValue(tag->getTrackNumber());
  yearSpin->setValue(tag->getYear());

  if(header->getResult()) {
    lengthBox->setText(header->getLengthChar());
    bitrateBox->setText(QString::number(header->getBitrate()));
  }

  commentBox->setText(tag->getComment());
}
