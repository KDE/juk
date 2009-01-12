/***************************************************************************
    copyright            : (C) 2002 by Daniel Molkentin
    email                : molkentin@kde.org

    copyright            : (C) 2002 - 2004 by Scott Wheeler
    email                : wheeler@kde.org

    copyright            : (C) 2007, 2008 by Michael Pyne
    email                : michael.pyne@kdemail.net
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

#include <ksystemtrayicon.h>

#include <QVector>
#include <QColor>
#include <QPixmap>
#include <QIcon>
#include <QFrame>

class QMouseEvent;
class QWheelEvent;
class QLabel;
class QTimer;
class KVBox;
class FileHandle;
class QVBoxLayout;

/**
 * Workalike of KPassivePopup intended to more easily support JuK's particular
 * usage pattern, including things like staying open while under the mouse.
 *
 * @author Michael Pyne <michael.pyne@kdemail.net>
 */
class PassiveInfo : public QFrame
{
    Q_OBJECT
public:
    PassiveInfo(QSystemTrayIcon *parent = 0);

    // Sets view as a child widget to show in the popup window.
    // This widget does not take ownership of the widget.  If you want it auto-deleted,
    // either re-parent it or create it using this widget as its parent.
    void setView(QWidget *view);

    QWidget *view() const;

public slots:
    // Starts a timer to show the popup.  The popup will not automatically delete itself
    // once hidden.
    void startTimer(int delay);
    virtual void show();

signals:
    void mouseEntered();
    void timeExpired();

protected:
    virtual void enterEvent(QEvent *);
    virtual void leaveEvent(QEvent *);
    virtual void hideEvent(QHideEvent *);

private:
    // Move us near the system tray icon.
    void positionSelf();

private slots:
    void timerExpired();

private:
    QSystemTrayIcon *m_icon;
    QTimer *m_timer;
    QVBoxLayout *m_layout;
    QWidget *m_view;
    bool m_justDie;
};

class SystemTray : public KSystemTrayIcon
{
    Q_OBJECT

public:
    SystemTray(QWidget *parent = 0);
    virtual ~SystemTray();

signals:
    // Emitted when the fade process is complete.
    void fadeDone();

private:
    static const int STEPS = 20; ///< Number of intermediate steps for fading.

    virtual void wheelEvent(QWheelEvent *e);
    void createPopup();
    void setToolTip(const QString &tip = QString(), const QPixmap &cover = QPixmap());
    void slotActivated(QSystemTrayIcon::ActivationReason reason);
    void ousePressEvent(QMouseEvent *e);
    QPixmap createPixmap(const QString &pixName);

    // Returns true if the popup will need to have its buttons on the left
    // (because the JuK icon is on the left side of the screen.
    bool buttonsToLeft() const;

    void createButtonBox(QWidget *parent);

    // Creates the widget layout for the popup, returning the QVBox that
    // holds the text labels.  Uses buttonsToLeft() to figure out which
    // order to create them in.  @p file is used to grab the cover.
    KVBox *createPopupLayout(QWidget *parent, const FileHandle &file);

    void addSeparatorLine(QWidget *parent);
    void addCoverButton(QWidget *parent, const QPixmap &cover);

    // Interpolates from start color to end color.  If @p step == 0, then
    // m_startColor is returned, while @p step == @steps returns
    // m_endColor.
    QColor interpolateColor(int step, int steps = STEPS);

private slots:
    void slotPlay();
    void slotTogglePopup();
    void slotPause() { setIcon(m_pausePix); }
    void slotStop();
    void slotPopupDestroyed();
    void slotNextStep(); ///< This is the fading routine.
    void slotPopupLargeCover();
    void slotForward();
    void slotBack();
    void slotFadeOut(); ///< Fades out the text
    void slotMouseInPopup(); ///< Forces the text back to its normal color.

private:
    QPixmap m_playPix;
    QPixmap m_pausePix;
    QPixmap m_currentPix;
    QPixmap m_backPix;
    QPixmap m_forwardPix;
    QIcon   m_appPix;
    QColor m_startColor, m_endColor;

    PassiveInfo *m_popup;
    QVector<QLabel *> m_labels;
    QTimer *m_fadeTimer;
    int m_step;
    bool m_fade;

    /// Used to choose between manual fade and windowOpacity
    bool m_hasCompositionManager;
};

#endif // SYSTEMTRAY_H

// vim: set et sw=4 tw=0 sta:
