/***************************************************************************
    copyright            : (C) 2002 by Daniel Molkentin
    email                : molkentin@kde.org

    copyright            : (C) 2002 - 2004 by Scott Wheeler
    email                : wheeler@kde.org

    copyright            : (C) 2007, 2008, 2009 by Michael Pyne
    email                : mpyne@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef JUK_SYSTEMTRAY_H
#define JUK_SYSTEMTRAY_H

#include <kstatusnotifieritem.h>

#include <QVector>
#include <QColor>
#include <QPixmap>
#include <QIcon>
#include <QFrame>

class SystemTray;
class PlayerManager;
class QLabel;
class QTimer;
class KVBox;
class FileHandle;
class QVBoxLayout;

/**
 * Workalike of KPassivePopup intended to more easily support JuK's particular
 * usage pattern, including things like staying open while under the mouse.
 *
 * @author Michael Pyne <mpyne@kde.org>
 */
class PassiveInfo : public QFrame
{
    Q_OBJECT
public:
    PassiveInfo(SystemTray *parent = 0);

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
    void previousSong();
    void nextSong();

protected:
    virtual void enterEvent(QEvent *);
    virtual void leaveEvent(QEvent *);
    virtual void hideEvent(QHideEvent *);
    virtual void wheelEvent(QWheelEvent *);

private:
    // Move us near the required position.
    void positionSelf();

private slots:
    void timerExpired();

private:
    SystemTray *m_icon;
    QTimer *m_timer;
    QVBoxLayout *m_layout;
    QWidget *m_view;
    bool m_justDie;
};

class SystemTray : public KStatusNotifierItem
{
    Q_OBJECT

public:
    SystemTray(PlayerManager *player, QWidget *parent = 0);

signals:
    // Emitted when the fade process is complete.
    void fadeDone();

private:
    static const int STEPS = 20; ///< Number of intermediate steps for fading.

    void createPopup();
    void setToolTip(const QString &tip = QString(), const QPixmap &cover = QPixmap());

    void createButtonBox(QWidget *parent);

    // Creates the widget layout for the popup, returning the QVBox that
    // holds the text labels.

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
    void slotPause();
    void slotStop();
    void slotPopupDestroyed();
    void slotNextStep(); ///< This is the fading routine.
    void slotPopupLargeCover();
    void slotForward();
    void slotBack();
    void slotFadeOut(); ///< Fades out the text
    void slotMouseInPopup(); ///< Forces the text back to its normal color.
    void scrollEvent(int delta, Qt::Orientation orientation);

private:
    QPixmap m_backPix;
    QPixmap m_forwardPix;
    QColor m_startColor, m_endColor;

    PassiveInfo *m_popup;
    PlayerManager *m_player;
    QVector<QLabel *> m_labels;
    QTimer *m_fadeTimer;
    int m_step;
    bool m_fade;

    /// Used to choose between manual fade and windowOpacity
    bool m_hasCompositionManager;
};

#endif // JUK_SYSTEMTRAY_H

// vim: set et sw=4 tw=0 sta:
