/***************************************************************************
                          juk.h  -  description
                             -------------------
    begin                : Mon Feb  4 23:40:41 EST 2002
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

#ifndef JUK_H
#define JUK_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <kapp.h>
#include <kaction.h>
#include <kstdaction.h>
#include <kmainwindow.h>

#include <qtimer.h>

#include "taggerwidget.h"
#include "playlistwidget.h"
#include "slideraction.h"
#include "player.h"

class JuK : public KMainWindow
{
  Q_OBJECT 
public:
  JuK(QWidget* parent=0, const char *name=0);
  ~JuK();

private: 
  // private methods
  void setupActions();
  void setupLayout();
  void setupPlayer();
  void readConfig();
  void saveConfig();

  // layout objects
  TaggerWidget *tagger;
  PlaylistWidget *playlist;

  // actions
  SliderAction *sliderAction;  
  KAction *playAction, *pauseAction, *stopAction;


  QTimer *playTimer;
  Player player;
  bool trackPositionDragging;
  bool noSeek;

private slots:
  // file menu
  void openFile();
  void openDirectory();
  void saveFile();
  void quit();

  // function menu
  void showTagger();
  void showPlaylist();

  // player menu
  void addToPlaylist();
  void removeFromPlaylist();

  void playFile();
  void pauseFile();
  void stopFile();

  // additional player slots
  void trackPositionSliderClick();
  void trackPositionSliderRelease();
  void trackPositionSliderUpdate(int position);
  void pollPlay();
  void setVolume(int volume);
};

#endif
