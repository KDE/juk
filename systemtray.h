/***************************************************************************
    copyright            : (C) 2002 by Daniel Molkentin
    email                : molkentin@kde.org

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

#ifndef SYSTEMTRAY_H
#define SYSTEMTRAY_H

#include <ksystemtray.h>

class QTimer;
class KPassivePopup;

class SystemTray : public KSystemTray
{
    Q_OBJECT

public:
    SystemTray(QWidget *parent = 0, const char *name = 0);
    virtual ~SystemTray();

private:
    virtual void wheelEvent(QWheelEvent *e);
    void createPopup();
    void setToolTip(const QString &tip = QString::null);
    void mousePressEvent(QMouseEvent *e);
    bool buttonsToLeft() const;
    QPixmap createPixmap(const QString &pixName);

private slots:
    void slotPlay();
    void slotPause() { setPixmap(m_pausePix); }
    void slotStop();

private:
    QPixmap m_playPix;
    QPixmap m_pausePix;
    QPixmap m_currentPix;
    QPixmap m_backPix;
    QPixmap m_forwardPix;
    QPixmap m_appPix;

    KPassivePopup *m_popup;
    QLabel *m_currentLabel;
};

#endif // SYSTEMTRAY_H
