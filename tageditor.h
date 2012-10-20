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

#include <QWidget>
#include <QList>
#include <QMap>

#include "ui_tageditor.h"
#include "filehandle.h"

class KComboBox;
class KLineEdit;
class KIntSpinBox;
class KTextEdit;
class KConfigGroup;

class QCheckBox;
class QBoxLayout;

class CollectionObserver;
class Playlist;

class TagEditor : public QWidget, public Ui::TagEditor
{
    Q_OBJECT

public:
    TagEditor(QWidget *parent = 0);
    virtual ~TagEditor();
    const FileHandleList &items() const { return m_items; }
    void setupObservers();

public slots:
    void slotSave() { save(m_items); }
    void slotSetItems(const QModelIndexList &list);
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
    void readCompletionMode(const KConfigGroup &config, KComboBox *box, const QString &key);
    void saveConfig();
    void save(const FileHandleList& list);
    void saveChangesPrompt();
    virtual void showEvent(QShowEvent *e);
    virtual bool eventFilter(QObject *watched, QEvent *e);

private slots:
    void slotDataChanged(bool c = true);
    void slotItemRemoved(const FileHandle &item);
    void slotPlaylistRemoved() { m_currentPlaylist = 0; }

private:
    typedef QMap<QWidget *, QCheckBox *> BoxMap;
    BoxMap m_enableBoxes;

    QStringList m_genreList;

    FileHandleList m_items;
    const Playlist *m_currentPlaylist;

    CollectionObserver *m_observer;

    bool m_dataChanged;
    bool m_collectionChanged;
    bool m_performingSave;

    friend class CollectionObserver;
};

#endif

// vim: set et sw=4 tw=0 sta:
