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
#include <qslider.h>

class QBoxLayout;
class QDockWindow;

class VolumeSlider : public QSlider
{
    Q_OBJECT

public:
    VolumeSlider(Orientation o, QWidget *parent, const char *name);

    int volume() const;
    void setVolume(int value);

    void setOrientation(Orientation o);

signals:
    void signalVolumeChanged(int value);

protected:
    virtual void wheelEvent(QWheelEvent *e);
    virtual void focusInEvent(QFocusEvent *);

private slots:
    void slotValueChanged(int value);
};

class SliderAction : public KAction
{
    Q_OBJECT

public:
    SliderAction(const QString &text, QObject *parent, const char *name);
    virtual ~SliderAction();

    VolumeSlider *volumeSlider() const { return m_volumeSlider; }
    QSlider *trackPositionSlider() const { return m_trackPositionSlider; }

    bool dragging() const { return m_dragging; }
    bool volumeDragging() const { return m_volumeDragging; }

    virtual int plug(QWidget *parent, int index = -1);
    virtual void unplug(QWidget *widget);

    static const int minPosition;
    static const int maxPosition;

public slots:
    void slotUpdateOrientation();

signals:
    void signalPositionChanged(int position);
    void signalVolumeChanged(int volume);

private:
    QWidget *createWidget(QWidget *parent);

private slots:
    void slotUpdateSize();
    void slotVolumeSliderPressed();
    void slotVolumeSliderReleased();
    void slotSliderPressed();
    void slotSliderReleased();
    void slotToolbarDestroyed();

private:
    KToolBar *m_toolBar;
    QBoxLayout *m_layout;
    QSlider *m_trackPositionSlider;
    VolumeSlider *m_volumeSlider;
    bool m_dragging;
    bool m_volumeDragging;

    static const int volumeMax = 50;
};

#endif
