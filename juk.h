/***************************************************************************
    begin                : Mon Feb  4 23:40:41 EST 2002
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

#ifndef JUK_H
#define JUK_H

#include <kxmlguiwindow.h>

#include <QKeyEvent>


class KToggleAction;
class KGlobalAccel;

class SliderAction;
class StatusLabel;
class SystemTray;
class PlaylistSplitter;

class JuK : public KXmlGuiWindow
{
    Q_OBJECT

public:
    JuK(QWidget* parent = 0);
    virtual ~JuK();

    static JuK* JuKInstance();


    // Use a null cover for failure
    void coverDownloaded(const QPixmap &cover);

private:
    void setupLayout();
    void setupActions();
    void setupGlobalAccels();

    void keyPressEvent(QKeyEvent *);

    /**
     * readSettings() is separate from readConfig() in that it contains settings
     * that need to be read before the GUI is setup.
     */
    void readSettings();
    void readConfig();
    void saveConfig();

    virtual bool queryExit();
    virtual bool queryClose();

private slots:
    void slotSetupSystemTray();
    void slotShowHide();
    void slotAboutToQuit();
    void slotQuit();
    void slotToggleSystemTray(bool enabled);
    void slotEditKeys();
    void slotConfigureTagGuesser();
    void slotConfigureFileRenamer();
    void slotConfigureScrobbling();
    void slotUndo();
    void slotCheckAlbumNextAction(bool albumRandomEnabled);
    void slotProcessArgs();

private:
    PlaylistSplitter *m_splitter;
    StatusLabel *m_statusLabel;
    SystemTray *m_systemTray;

    KToggleAction *m_randomPlayAction;
    KToggleAction *m_toggleSystemTrayAction;
    KToggleAction *m_toggleDockOnCloseAction;
    KToggleAction *m_togglePopupsAction;
    KToggleAction *m_toggleSplashAction;
    KToggleAction *m_loopPlaylistAction;

    KGlobalAccel *m_accel;

    bool m_startDocked;
    bool m_showSplash;
    bool m_shuttingDown;

    static JuK* m_instance;
};

#endif

// vim: set et sw=4 tw=0 sta:
