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

#include "tageditor.h"
#include "collectionlist.h"
#include "playlistitem.h"
#include "tag.h"
#include "actioncollection.h"
#include "tagtransactionmanager.h"

#include <kactioncollection.h>
#include <kconfiggroup.h>
#include <kcombobox.h>
#include <klineedit.h>
#include <knuminput.h>
#include <ktextedit.h>
#include <kapplication.h>
#include <kmessagebox.h>
#include <kconfig.h>
#include <klocale.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kicon.h>
#include <ktoggleaction.h>
#include <kshortcut.h>

#include <QLabel>
#include <QApplication>
#include <QCheckBox>
#include <QDir>
#include <QValidator>
#include <QEventLoop>
#include <QKeyEvent>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSizePolicy>

#include <id3v1genres.h>

#undef KeyRelease

class FileNameValidator : public QValidator
{
public:
    FileNameValidator(QObject *parent, const char *name = 0) :
        QValidator(parent)
    {
        setObjectName( QLatin1String( name ) );
    }

    virtual void fixup(QString &s) const
    {
        s.remove('/');
    }

    virtual State validate(QString &s, int &) const
    {
        if(s.contains('/'))
           return Invalid;
        return Acceptable;
    }
};

class FixedHLayout : public QHBoxLayout
{
public:
    FixedHLayout(QWidget *parent, int margin = 0, int spacing = -1, const char *name = 0) :
        QHBoxLayout(parent),
        m_width(-1)
    {
        setMargin(margin);
        setSpacing(spacing);
        setObjectName( name );
    }
    FixedHLayout(QLayout *parentLayout, int spacing = -1, const char *name = 0) :
        QHBoxLayout(),
        m_width(-1)
    {
        parentLayout->addItem(this);
        setSpacing(spacing);
        setObjectName(name);
    }
    void setWidth(int w = -1)
    {
        m_width = w == -1 ? QHBoxLayout::minimumSize().width() : w;
    }
    virtual QSize minimumSize() const
    {
        QSize s = QHBoxLayout::minimumSize();
        s.setWidth(m_width);
        return s;
    }
private:
    int m_width;
};

class CollectionObserver : public PlaylistObserver
{
public:
    CollectionObserver(TagEditor *parent) :
        PlaylistObserver(CollectionList::instance()),
        m_parent(parent)
    {
    }

    virtual void updateData()
    {
        if(m_parent && m_parent->m_currentPlaylist && m_parent->isVisible())
            m_parent->slotSetItems(m_parent->m_currentPlaylist->selectedItems());
    }

    virtual void updateCurrent() {}

private:
    TagEditor *m_parent;
};

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

TagEditor::TagEditor(QWidget *parent) :
    QWidget(parent),
    m_currentPlaylist(0),
    m_observer(0),
    m_performingSave(false)
{
    setupActions();
    setupLayout();
    readConfig();
    m_dataChanged = false;
    m_collectionChanged = false;

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}

TagEditor::~TagEditor()
{
    delete m_observer;
    saveConfig();
}

void TagEditor::setupObservers()
{
    m_observer = new CollectionObserver(this);
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void TagEditor::slotSetItems(const PlaylistItemList &list)
{
    if(m_performingSave)
        return;

    // Store the playlist that we're setting because saveChangesPrompt
    // can delete the PlaylistItems in list.

    Playlist *itemPlaylist = 0;
    if(!list.isEmpty())
        itemPlaylist = list.first()->playlist();

    bool hadPlaylist = m_currentPlaylist != 0;

    saveChangesPrompt();

    if(m_currentPlaylist) {
        disconnect(m_currentPlaylist, SIGNAL(signalAboutToRemove(PlaylistItem *)),
                   this, SLOT(slotItemRemoved(PlaylistItem *)));
    }

    if((hadPlaylist && !m_currentPlaylist) || !itemPlaylist) {
        m_currentPlaylist = 0;
        m_items.clear();
    }
    else {
        m_currentPlaylist = itemPlaylist;

        // We can't use list here, it may not be valid

        m_items = itemPlaylist->selectedItems();
    }

    if(m_currentPlaylist) {
        connect(m_currentPlaylist, SIGNAL(signalAboutToRemove(PlaylistItem *)),
                this, SLOT(slotItemRemoved(PlaylistItem *)));
        connect(m_currentPlaylist, SIGNAL(destroyed()), this, SLOT(slotPlaylistRemoved()));
    }

    if(isVisible())
        slotRefresh();
    else
        m_collectionChanged = true;
}

void TagEditor::slotRefresh()
{
    // This method takes the list of currently selected m_items and tries to
    // figure out how to show that in the tag editor.  The current strategy --
    // the most common case -- is to just process the first item.  Then we
    // check after that to see if there are other m_items and adjust accordingly.

    if(m_items.isEmpty() || !m_items.first()->file().tag()) {
        slotClear();
        setEnabled(false);
        return;
    }

    setEnabled(true);

    PlaylistItem *item = m_items.first();

    Q_ASSERT(item);

    Tag *tag = item->file().tag();

    QFileInfo fi(item->file().absFilePath());
    if(!fi.isWritable() && m_items.count() == 1)
        setEnabled(false);

    artistNameBox->setEditText(tag->artist());
    trackNameBox->setText(tag->title());
    albumNameBox->setEditText(tag->album());

    fileNameBox->setText(item->file().fileInfo().fileName());
    fileNameBox->setToolTip(item->file().absFilePath());

    bitrateBox->setText(QString::number(tag->bitrate()));
    lengthBox->setText(tag->lengthString());

    if(m_genreList.indexOf(tag->genre()) >= 0)
        genreBox->setCurrentIndex(m_genreList.indexOf(tag->genre()) + 1);
    else {
        genreBox->setCurrentIndex(0);
        genreBox->setEditText(tag->genre());
    }

    trackSpin->setValue(tag->track());
    yearSpin->setValue(tag->year());

    commentBox->setPlainText(tag->comment());

    // Start at the second item, since we've already processed the first.

    PlaylistItemList::Iterator it = m_items.begin();
    ++it;

    // If there is more than one item in the m_items that we're dealing with...


    QList<QWidget *> disabledForMulti;

    disabledForMulti << fileNameLabel << fileNameBox << lengthLabel << lengthBox
                     << bitrateLabel << bitrateBox;

    foreach(QWidget *w, disabledForMulti) {
        w->setDisabled(m_items.size() > 1);
        if(m_items.size() > 1 && !w->inherits("QLabel"))
            QMetaObject::invokeMethod(w, "clear");
    }

    if(it != m_items.end()) {

        foreach(QCheckBox *box, m_enableBoxes) {
            box->setChecked(true);
            box->show();
        }

        // Yep, this is ugly.  Loop through all of the files checking to see
        // if their fields are the same.  If so, by default, enable their
        // checkbox.

        // Also, if there are more than 50 m_items, don't scan all of them.

        if(m_items.count() > 50) {
            m_enableBoxes[artistNameBox]->setChecked(false);
            m_enableBoxes[trackNameBox]->setChecked(false);
            m_enableBoxes[albumNameBox]->setChecked(false);
            m_enableBoxes[genreBox]->setChecked(false);
            m_enableBoxes[trackSpin]->setChecked(false);
            m_enableBoxes[yearSpin]->setChecked(false);
            m_enableBoxes[commentBox]->setChecked(false);
        }
        else {
            for(; it != m_items.end(); ++it) {
                tag = (*it)->file().tag();

                if(tag) {

                    if(artistNameBox->currentText() != tag->artist() &&
                       m_enableBoxes.contains(artistNameBox))
                    {
                        artistNameBox->lineEdit()->clear();
                        m_enableBoxes[artistNameBox]->setChecked(false);
                    }
                    if(trackNameBox->text() != tag->title() &&
                       m_enableBoxes.contains(trackNameBox))
                    {
                        trackNameBox->clear();
                        m_enableBoxes[trackNameBox]->setChecked(false);
                    }
                    if(albumNameBox->currentText() != tag->album() &&
                       m_enableBoxes.contains(albumNameBox))
                    {
                        albumNameBox->lineEdit()->clear();
                        m_enableBoxes[albumNameBox]->setChecked(false);
                    }
                    if(genreBox->currentText() != tag->genre() &&
                       m_enableBoxes.contains(genreBox))
                    {
                        genreBox->lineEdit()->clear();
                        m_enableBoxes[genreBox]->setChecked(false);
                    }
                    if(trackSpin->value() != tag->track() &&
                       m_enableBoxes.contains(trackSpin))
                    {
                        trackSpin->setValue(0);
                        m_enableBoxes[trackSpin]->setChecked(false);
                    }
                    if(yearSpin->value() != tag->year() &&
                       m_enableBoxes.contains(yearSpin))
                    {
                        yearSpin->setValue(0);
                        m_enableBoxes[yearSpin]->setChecked(false);
                    }
                    if(commentBox->toPlainText() != tag->comment() &&
                       m_enableBoxes.contains(commentBox))
                    {
                        commentBox->clear();
                        m_enableBoxes[commentBox]->setChecked(false);
                    }
                }
            }
        }
    }
    else {
        foreach(QCheckBox *box, m_enableBoxes) {
            box->setChecked(true);
            box->hide();
        }
    }
    m_dataChanged = false;
}

void TagEditor::slotClear()
{
    artistNameBox->lineEdit()->clear();
    trackNameBox->clear();
    albumNameBox->lineEdit()->clear();
    genreBox->setCurrentIndex(0);
    fileNameBox->clear();
    fileNameBox->setToolTip(QString());
    trackSpin->setValue(0);
    yearSpin->setValue(0);
    lengthBox->clear();
    bitrateBox->clear();
    commentBox->clear();
}

void TagEditor::slotUpdateCollection()
{
    if(isVisible())
        updateCollection();
    else
        m_collectionChanged = true;
}

void TagEditor::updateCollection()
{
    m_collectionChanged = false;

    CollectionList *list = CollectionList::instance();

    if(!list)
        return;

    QStringList artistList = list->uniqueSet(CollectionList::Artists);
    artistList.sort();
    artistNameBox->clear();
    artistNameBox->addItems(artistList);
    artistNameBox->completionObject()->setItems(artistList);

    QStringList albumList = list->uniqueSet(CollectionList::Albums);
    albumList.sort();
    albumNameBox->clear();
    albumNameBox->addItems(albumList);
    albumNameBox->completionObject()->setItems(albumList);

    // Merge the list of genres found in tags with the standard ID3v1 set.

    StringHash genreHash;

    m_genreList = list->uniqueSet(CollectionList::Genres);

    foreach(const QString &genre, m_genreList)
        genreHash.insert(genre);

    TagLib::StringList genres = TagLib::ID3v1::genreList();

    for(TagLib::StringList::Iterator it = genres.begin(); it != genres.end(); ++it)
        genreHash.insert(TStringToQString((*it)));

    m_genreList = genreHash.values();
    m_genreList.sort();

    genreBox->clear();
    genreBox->addItem(QString());
    genreBox->addItems(m_genreList);
    genreBox->completionObject()->setItems(m_genreList);

    // We've cleared out the original entries of these list boxes, re-read
    // the current item if one is selected.
    slotRefresh();
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void TagEditor::readConfig()
{
    // combo box completion modes

    KConfigGroup config(KGlobal::config(), "TagEditor");
    if(artistNameBox && albumNameBox) {
        readCompletionMode(config, artistNameBox, "ArtistNameBoxMode");
        readCompletionMode(config, albumNameBox, "AlbumNameBoxMode");
        readCompletionMode(config, genreBox, "GenreBoxMode");
    }

    bool show = config.readEntry("Show", false);
    ActionCollection::action<KToggleAction>("showEditor")->setChecked(show);
    setVisible(show);

    TagLib::StringList genres = TagLib::ID3v1::genreList();

    for(TagLib::StringList::ConstIterator it = genres.begin(); it != genres.end(); ++it)
        m_genreList.append(TStringToQString((*it)));
    m_genreList.sort();

    genreBox->clear();
    genreBox->addItem(QString());
    genreBox->addItems(m_genreList);
    genreBox->completionObject()->setItems(m_genreList);
}

void TagEditor::readCompletionMode(const KConfigGroup &config, KComboBox *box, const QString &key)
{
    KGlobalSettings::Completion mode =
        KGlobalSettings::Completion(config.readEntry(key, (int)KGlobalSettings::CompletionAuto));

    box->setCompletionMode(mode);
}

void TagEditor::saveConfig()
{
    // combo box completion modes

    KConfigGroup config(KGlobal::config(), "TagEditor");

    if(artistNameBox && albumNameBox) {
        config.writeEntry("ArtistNameBoxMode", (int)artistNameBox->completionMode());
        config.writeEntry("AlbumNameBoxMode", (int)albumNameBox->completionMode());
        config.writeEntry("GenreBoxMode", (int)genreBox->completionMode());
    }
    config.writeEntry("Show", ActionCollection::action<KToggleAction>("showEditor")->isChecked());
}

void TagEditor::setupActions()
{
    KToggleAction *show = new KToggleAction(KIcon(QLatin1String("document-properties")),
                                            i18n("Show &Tag Editor"), this);
    ActionCollection::actions()->addAction("showEditor", show);
    connect(show, SIGNAL(toggled(bool)), this, SLOT(setShown(bool)));

    KAction *act = new KAction(KIcon(QLatin1String( "document-save")), i18n("&Save"), this);
    ActionCollection::actions()->addAction("saveItem", act);
    act->setShortcut(Qt::CTRL + Qt::Key_T);
    connect(act, SIGNAL(triggered(bool)), SLOT(slotSave()));
}

void TagEditor::setupLayout()
{
    setupUi(this);

    foreach(QWidget *input, findChildren<QWidget *>()) {
        if(input->inherits("QLineEdit") || input->inherits("QComboBox"))
            connect(input, SIGNAL(textChanged(const QString &)), this, SLOT(slotDataChanged()));
        if(input->inherits("QComboxBox"))
            connect(input, SIGNAL(activated(int)), this, SLOT(slotDataChanged()));
        if(input->inherits("QSpinBox"))
            connect(input, SIGNAL(valueChanged(int)), this, SLOT(slotDataChanged()));
        if(input->inherits("QTextEdit"))
            connect(input, SIGNAL(textChanged()), this, SLOT(slotDataChanged()));
    }

    // Do some meta-programming to find the matching enable boxes

    foreach(QCheckBox *enable, findChildren<QCheckBox *>(QRegExp("Enable"))) {
        enable->hide();
        QRegExp re("^" + enable->objectName().replace("Enable", "") + "(Box|Spin)$");
        QList<QWidget *> targets = findChildren<QWidget *>(re);
        Q_ASSERT(!targets.isEmpty());
        m_enableBoxes[targets.front()] = enable;
    }
}

void TagEditor::save(const PlaylistItemList &list)
{
    if(!list.isEmpty() && m_dataChanged) {

        KApplication::setOverrideCursor(Qt::WaitCursor);
        m_dataChanged = false;
        m_performingSave = true;

        // The list variable can become corrupted if the playlist holding its
        // items dies, which is possible as we edit tags.  So we need to copy
        // the end marker.

        PlaylistItemList::ConstIterator end = list.end();

        for(PlaylistItemList::ConstIterator it = list.begin(); it != end; /* Deliberately missing */ ) {

            // Process items before we being modifying tags, as the dynamic
            // playlists will try to modify the file we edit if the tag changes
            // due to our alterations here.

            qApp->processEvents(QEventLoop::ExcludeUserInput);

            PlaylistItem *item = *it;

            // The playlist can be deleted from under us if this is the last
            // item and we edit it so that it doesn't match the search, which
            // means we can't increment the iterator, so let's do it now.

            ++it;

            QString fileName = item->file().fileInfo().path() + QDir::separator() +
                               fileNameBox->text();
            if(list.count() > 1)
                fileName = item->file().fileInfo().absoluteFilePath();

            Tag *tag = TagTransactionManager::duplicateTag(item->file().tag(), fileName);

            // A bit more ugliness.  If there are multiple files that are
            // being modified, they each have a "enabled" checkbox that
            // says if that field is to be respected for the multiple
            // files.  We have to check to see if that is enabled before
            // each field that we write.

            if(m_enableBoxes[artistNameBox]->isChecked())
                tag->setArtist(artistNameBox->currentText());
            if(m_enableBoxes[trackNameBox]->isChecked())
                tag->setTitle(trackNameBox->text());
            if(m_enableBoxes[albumNameBox]->isChecked())
                tag->setAlbum(albumNameBox->currentText());
            if(m_enableBoxes[trackSpin]->isChecked()) {
                if(trackSpin->text().isEmpty())
                    trackSpin->setValue(0);
                tag->setTrack(trackSpin->value());
            }
            if(m_enableBoxes[yearSpin]->isChecked()) {
                if(yearSpin->text().isEmpty())
                    yearSpin->setValue(0);
                tag->setYear(yearSpin->value());
            }
            if(m_enableBoxes[commentBox]->isChecked())
                tag->setComment(commentBox->toPlainText());

            if(m_enableBoxes[genreBox]->isChecked())
                tag->setGenre(genreBox->currentText());

            TagTransactionManager::instance()->changeTagOnItem(item, tag);
        }

        TagTransactionManager::instance()->commit();
        CollectionList::instance()->dataChanged();
        m_performingSave = false;
        KApplication::restoreOverrideCursor();
    }
}

void TagEditor::saveChangesPrompt()
{
    if(!isVisible() || !m_dataChanged || m_items.isEmpty())
        return;

    QStringList files;

    foreach(const PlaylistItem *item, m_items)
        files.append(item->file().absFilePath());

    if(KMessageBox::questionYesNoList(this,
                                      i18n("Do you want to save your changes to:\n"),
                                      files,
                                      i18n("Save Changes"),
                                      KStandardGuiItem::save(),
                                      KStandardGuiItem::discard(),
                                      "tagEditor_showSaveChangesBox") == KMessageBox::Yes)
    {
        save(m_items);
    }
}

void TagEditor::showEvent(QShowEvent *e)
{
    if(m_collectionChanged) {
        updateCollection();
    }

    QWidget::showEvent(e);
}

bool TagEditor::eventFilter(QObject *watched, QEvent *e)
{
    QKeyEvent *ke = static_cast<QKeyEvent*>(e);
    if(watched->inherits("QSpinBox") && e->type() == QEvent::KeyRelease && ke->modifiers() == 0)
        slotDataChanged();

    return false;
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void TagEditor::slotDataChanged(bool c)
{
    m_dataChanged = c;
}

void TagEditor::slotItemRemoved(PlaylistItem *item)
{
    m_items.removeAll(item);
    if(m_items.isEmpty())
        slotRefresh();
}

void TagEditor::slotPlaylistDestroyed(Playlist *p)
{
    if(m_currentPlaylist == p) {
        m_currentPlaylist = 0;
        slotSetItems(PlaylistItemList());
    }
}

#include "tageditor.moc"

// vim: set et sw=4 tw=0 sta:
