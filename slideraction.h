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
//Added by qt3to4:
#include <QWheelEvent>
#include <QFocusEvent>
#include <QBoxLayout>
#include <ktoolbar.h>
#include <phonon/volumeslider.h>
#include <phonon/seekslider.h>

class KActionCollection;
class QBoxLayout;
class Q3DockWindow;

class VolumeSlider : public Phonon::VolumeSlider
{
    Q_OBJECT
public:
    VolumeSlider(Qt::Orientation o, QWidget *parent);

protected:
    virtual void focusInEvent(QFocusEvent *);
};

class SliderAction : public KAction
{
    Q_OBJECT

public:
    SliderAction(const QString &text, QObject* parent);
    virtual ~SliderAction();

    VolumeSlider *volumeSlider() const { return m_volumeSlider; }
    Phonon::SeekSlider *trackPositionSlider() const { return m_trackPositionSlider; }

    virtual int plug(QWidget *parent, int index = -1);
    virtual void unplug(QWidget *widget);

    virtual QWidget* createToolBarWidget(QToolBar* parent);

public slots:
    void slotUpdateOrientation();

private:
    QWidget *createWidget(QWidget *parent);

private slots:
    void slotUpdateSize();
    void slotToolbarDestroyed();

private:
    KToolBar *m_toolBar;
    QWidget *m_widget;
    QBoxLayout *m_layout;
    Phonon::SeekSlider *m_trackPositionSlider;
    VolumeSlider *m_volumeSlider;

    static const int volumeMax = 50;
};

#endif

// vim: set et sw=4 tw=0 sta:
