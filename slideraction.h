/***************************************************************************
                          slideraction.h  -  description
                             -------------------
    begin                : Wed Feb 6 2002
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

#ifndef SLIDERACTION_H
#define SLIDERACTION_H

#include <kaction.h>
#include <ktoolbar.h>

#include <qstring.h>
#include <qslider.h>
#include <qobject.h>
#include <qlayout.h>

#include "customaction.h"

class SliderAction : public CustomAction
{
    Q_OBJECT
public:
    SliderAction(const QString &text, QObject *parent, const char *name);
    ~SliderAction();

    QSlider *getVolumeSlider();
    QSlider *getTrackPositionSlider();

public slots:
    void updateOrientation(QDockWindow *dockWindow = 0);

private:
    QWidget *createWidget(QWidget *parent);

    QBoxLayout *layout;
    QSlider *trackPositionSlider;
    QSlider *volumeSlider;

    static const int volumeMax = 50;

private slots:
    void updateLabels();
    void updateSize();
};

#endif
