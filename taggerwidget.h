/***************************************************************************
                     taggerwidget.h  -  description
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

#ifndef TAGGERWIDGET_H
#define TAGGERWIDGET_H

#include <klistview.h>
#include <kcombobox.h>
#include <klineedit.h>
#include <knuminput.h>
#include <keditcl.h>

#include <qptrlist.h>

#include "filelist.h"
#include "filelistitem.h"
#include "genrelist.h"

class TaggerWidget : public QWidget
{
  Q_OBJECT

public: 
  TaggerWidget(QWidget *parent);
  ~TaggerWidget();

  void add(QString item);
  void add(QStringList &items);

  FileList *getTaggerList();
  
  QPtrList<FileListItem> getSelectedItems();

public slots:
  void save();
  void save(QPtrList<FileListItem> items);
  void setChanged();

private:
  void setupLayout();
  void readConfig();

  // main visual objects
  FileList *taggerList;

  KComboBox *artistNameBox;
  KLineEdit *trackNameBox;
  KComboBox *albumNameBox;
  KComboBox *genreBox;
  KLineEdit *fileNameBox;
  KIntSpinBox *trackSpin;
  KIntSpinBox *yearSpin;
  KLineEdit *lengthBox;
  KLineEdit *bitrateBox;
  KEdit *commentBox;

  // internally useful things
  QPtrList<FileListItem> previousSelection;
  GenreList *genreList;
  bool changed;

private slots:
  void saveChangesPrompt();
  void updateBoxes();
  void updateCombos();
};

#endif
