/***************************************************************************
    begin                : Wed Feb 6 2002
    copyright            : (C) 2002 - 2004 by Scott Wheeler
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

#ifndef SLIDERACTION_H
#define SLIDERACTION_H

#include <kaction.h>
#include <QBoxLayout>

#include "volumepopupbutton.h"

class Slider;

class TrackPositionAction : public KAction
{
    Q_OBJECT
public:
    TrackPositionAction(const QString &text, QObject *parent);
    Slider *slider() const;
protected:
    virtual QWidget *createWidget(QWidget *parent);
private slots:
    void totalTimeChanged(int ms);
};

class VolumeAction : public KAction
{
    Q_OBJECT
public:
    VolumeAction(const QString &text, QObject *parent);
    VolumePopupButton *button() const { return m_button; }
protected:
    virtual QWidget *createWidget(QWidget *parent);
private:
    VolumePopupButton *m_button;
};

#endif

// vim: set et sw=4 tw=0 sta:
