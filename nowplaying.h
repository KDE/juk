/***************************************************************************
    copyright            : (C) 2004 Nathan Toone
    email                : nathan@toonetown.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef NOWPLAYING_H
#define NOWPLAYING_H

#include <qwidget.h>

class QPushButton;
class QSplitter;

class NowPlaying : public QWidget
{
    Q_OBJECT

public:
    NowPlaying(QSplitter *parent, const char *name = 0);
    virtual ~NowPlaying();

private:
    void setupActions();
    void setupLayout();
    void readConfig();
    void saveConfig();

public slots:
    void slotRefresh();
    void slotClear();

private slots:
    void slotButtonPress();

protected:
    virtual void resizeEvent(QResizeEvent *ev);
    virtual void mousePressEvent(QMouseEvent *e);

private:
    QPushButton *m_button;
};

#endif
