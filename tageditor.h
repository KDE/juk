/***************************************************************************
    begin                : Sat Sep 7 2002
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

#ifndef TAGEDITOR_H
#define TAGEDITOR_H

#include <qwidget.h>

class KComboBox;
class KLineEdit;
class KIntSpinBox;
class KEdit;
class KPushButton;
class KConfigBase;

class QCheckBox;
class QBoxLayout;

class Playlist;
class PlaylistItem;
typedef QValueList<PlaylistItem *> PlaylistItemList;

class CollectionObserver;

class TagEditor : public QWidget
{
    Q_OBJECT

public: 
    TagEditor(QWidget *parent = 0, const char *name = 0);
    virtual ~TagEditor();
    PlaylistItemList items() const { return m_items; }
    void setupObservers();

public slots:
    void slotSave() { save(m_items); }
    void slotSetItems(const PlaylistItemList &list);
    void slotRefresh();
    void slotClear();
    void slotPlaylistDestroyed(Playlist *p);
    /**
     * Update collection if we're visible, or defer otherwise
     */
    void slotUpdateCollection();

private:
    void updateCollection();

    void setupActions();
    void setupLayout();
    void readConfig();
    void readCompletionMode(KConfigBase *config, KComboBox *box, const QString &key);
    void saveConfig();
    void save(const PlaylistItemList &list);
    void saveChangesPrompt();
    /**
     * Adds an item to JuK's tagging layout.  This handles the creation and
     * placement of the "enable" box as well.
     */
    void addItem(const QString &text, QWidget *item, QBoxLayout *layout, const QString &iconName = QString::null);

    /**
     * Adds a widget to m_hideList and returns that widget.
     */
    QWidget *addHidden(QWidget *w) { m_hideList.append(w); return w; }

    virtual void showEvent(QShowEvent *e);
    virtual bool eventFilter(QObject *watched, QEvent *e);

private slots:
    void slotDataChanged(bool c = true);
    void slotItemRemoved(PlaylistItem *item);
    void slotPlaylistRemoved() { m_currentPlaylist = 0; }

private:
    typedef QMap<QWidget *, QCheckBox *> BoxMap;
    BoxMap m_enableBoxes;

    QStringList m_genreList;

    KComboBox *m_artistNameBox;
    KLineEdit *m_trackNameBox;
    KComboBox *m_albumNameBox;
    KComboBox *m_genreBox;
    KLineEdit *m_fileNameBox;
    KIntSpinBox *m_trackSpin;
    KIntSpinBox *m_yearSpin;
    KLineEdit *m_lengthBox;
    KLineEdit *m_bitrateBox;
    KEdit *m_commentBox;

    QValueList<QWidget *> m_hideList;

    PlaylistItemList m_items;
    Playlist *m_currentPlaylist;
    
    CollectionObserver *m_observer;

    bool m_dataChanged;
    bool m_collectionChanged;
    bool m_performingSave;

    friend class CollectionObserver;
};

#endif
