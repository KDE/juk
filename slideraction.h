/***************************************************************************
                          slideraction.h  -  description
                             -------------------
    begin                : Wed Feb 6 2002
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

#ifndef SLIDERACTION_H
#define SLIDERACTION_H

#include "customaction.h"

class QSlider;
class QBoxLayout;
class QDockWindow;

class SliderAction : public CustomAction
{
    Q_OBJECT

public:
    SliderAction(const QString &text, QObject *parent, const char *name);
    virtual ~SliderAction();

    QSlider *volumeSlider() const { return m_volumeSlider; }
    QSlider *trackPositionSlider() const { return m_trackPositionSlider; }

    bool dragging() const { return m_dragging; }

public slots:
    void slotUpdateOrientation(QDockWindow *dockWindow = 0);

signals:
    void signalPositionChanged(int position);

private:
    QWidget *createWidget(QWidget *parent);

private slots:
    void slotUpdateSize();
    void slotSliderPressed();
    void slotSliderReleased();

private:
    QBoxLayout *m_layout;
    QSlider *m_trackPositionSlider;
    QSlider *m_volumeSlider;
    bool m_dragging;

    static const int volumeMax = 50;
};

#endif
