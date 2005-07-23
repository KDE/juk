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

#include <kcombobox.h>
#include <klineedit.h>
#include <knuminput.h>
#include <keditcl.h>
#include <kmessagebox.h>
#include <kconfig.h>
#include <klocale.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kactionclasses.h>

#include <qlabel.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qdir.h>
#include <qvalidator.h>
#include <qtooltip.h>
#include <qeventloop.h>
#include <qdict.h>

#include <id3v1genres.h>

#undef KeyRelease

using namespace ActionCollection;

class FileNameValidator : public QValidator
{
public:
    FileNameValidator(QObject *parent, const char *name = 0) :
	QValidator(parent, name) {}

    virtual void fixup(QString &s) const
    {
	s.remove('/');
    }

    virtual State validate(QString &s, int &) const
    {
	if(s.find('/') != -1)
	   return Invalid;
	return Acceptable;
    }
};

class FileBoxToolTip : public QToolTip
{
public:
    FileBoxToolTip(TagEditor *editor, QWidget *widget) :
	QToolTip(widget), m_editor(editor) {}
protected:
    virtual void maybeTip(const QPoint &)
    {
	tip(parentWidget()->rect(), m_editor->items().first()->file().absFilePath());
    }
private:
    TagEditor *m_editor;
};

class FixedHLayout : public QHBoxLayout
{
public:
    FixedHLayout(QWidget *parent, int margin = 0, int spacing = -1, const char *name = 0) :
	QHBoxLayout(parent, margin, spacing, name),
	m_width(-1) {}
    FixedHLayout(QLayout *parentLayout, int spacing = -1, const char *name = 0) :
	QHBoxLayout(parentLayout, spacing, name),
	m_width(-1) {}
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

TagEditor::TagEditor(QWidget *parent, const char *name) :
    QWidget(parent, name),
    m_currentPlaylist(0),
    m_observer(0),
    m_performingSave(false)
{
    setupActions();
    setupLayout();
    readConfig();
    m_dataChanged = false;
    m_collectionChanged = false;
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

    if(hadPlaylist && !m_currentPlaylist || !itemPlaylist) {
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

    m_artistNameBox->setEditText(tag->artist());
    m_trackNameBox->setText(tag->title());
    m_albumNameBox->setEditText(tag->album());

    m_fileNameBox->setText(item->file().fileInfo().fileName());
    new FileBoxToolTip(this, m_fileNameBox);
    m_bitrateBox->setText(QString::number(tag->bitrate()));
    m_lengthBox->setText(tag->lengthString());

    if(m_genreList.findIndex(tag->genre()) >= 0)
	m_genreBox->setCurrentItem(m_genreList.findIndex(tag->genre()) + 1);
    else {
	m_genreBox->setCurrentItem(0);
	m_genreBox->setEditText(tag->genre());
    }

    m_trackSpin->setValue(tag->track());
    m_yearSpin->setValue(tag->year());
    
    m_commentBox->setText(tag->comment());
    
    // Start at the second item, since we've already processed the first.
    
    PlaylistItemList::Iterator it = m_items.begin();
    ++it;

    // If there is more than one item in the m_items that we're dealing with...
    
    if(it != m_items.end()) {

	QValueListIterator<QWidget *> hideIt = m_hideList.begin();
	for(; hideIt != m_hideList.end(); ++hideIt)
	    (*hideIt)->hide();

	BoxMap::Iterator boxIt = m_enableBoxes.begin();
	for(; boxIt != m_enableBoxes.end(); boxIt++) {
	    (*boxIt)->setChecked(true);
	    (*boxIt)->show();
	}
	
	// Yep, this is ugly.  Loop through all of the files checking to see
	// if their fields are the same.  If so, by default, enable their 
	// checkbox.
	
	// Also, if there are more than 50 m_items, don't scan all of them.
	
	if(m_items.count() > 50) {
	    m_enableBoxes[m_artistNameBox]->setChecked(false);
	    m_enableBoxes[m_trackNameBox]->setChecked(false);
	    m_enableBoxes[m_albumNameBox]->setChecked(false);
	    m_enableBoxes[m_genreBox]->setChecked(false);
	    m_enableBoxes[m_trackSpin]->setChecked(false);
	    m_enableBoxes[m_yearSpin]->setChecked(false);
	    m_enableBoxes[m_commentBox]->setChecked(false);
	}
	else {
	    for(; it != m_items.end(); ++it) {
		tag = (*it)->file().tag();

		if(tag) {

		    if(m_artistNameBox->currentText() != tag->artist() &&
		       m_enableBoxes.contains(m_artistNameBox))
		    {
			m_artistNameBox->lineEdit()->clear();
			m_enableBoxes[m_artistNameBox]->setChecked(false);
		    }
		    if(m_trackNameBox->text() != tag->title() &&
		       m_enableBoxes.contains(m_trackNameBox))
		    {
			m_trackNameBox->clear();
			m_enableBoxes[m_trackNameBox]->setChecked(false);
		    }
		    if(m_albumNameBox->currentText() != tag->album() &&
		       m_enableBoxes.contains(m_albumNameBox))
		    {
			m_albumNameBox->lineEdit()->clear();
			m_enableBoxes[m_albumNameBox]->setChecked(false);
		    }
		    if(m_genreBox->currentText() != tag->genre() &&
		       m_enableBoxes.contains(m_genreBox))
		    {
			m_genreBox->lineEdit()->clear();
			m_enableBoxes[m_genreBox]->setChecked(false);
		    }		
		    if(m_trackSpin->value() != tag->track() &&
		       m_enableBoxes.contains(m_trackSpin))
		    {
			m_trackSpin->setValue(0);
			m_enableBoxes[m_trackSpin]->setChecked(false);
		    }		
		    if(m_yearSpin->value() != tag->year() &&
		       m_enableBoxes.contains(m_yearSpin))
		    {
			m_yearSpin->setValue(0);
			m_enableBoxes[m_yearSpin]->setChecked(false);
		    }
		    if(m_commentBox->text() != tag->comment() &&
		       m_enableBoxes.contains(m_commentBox))
		    {
			m_commentBox->clear();
			m_enableBoxes[m_commentBox]->setChecked(false);
		    }
		}
	    }
	}
    }
    else {
	// Clean up in the case that we are only handling one item.

	QValueListIterator<QWidget *> showIt = m_hideList.begin();
	for(; showIt != m_hideList.end(); ++showIt)
	    (*showIt)->show();

	BoxMap::iterator boxIt = m_enableBoxes.begin();
	for(; boxIt != m_enableBoxes.end(); boxIt++) {
	    (*boxIt)->setChecked(true);
	    (*boxIt)->hide();
	}
    }
    m_dataChanged = false;
}

void TagEditor::slotClear()
{
    m_artistNameBox->lineEdit()->clear();
    m_trackNameBox->clear();
    m_albumNameBox->lineEdit()->clear();
    m_genreBox->setCurrentItem(0);
    m_fileNameBox->clear();
    m_trackSpin->setValue(0);
    m_yearSpin->setValue(0);
    m_lengthBox->clear();
    m_bitrateBox->clear();
    m_commentBox->clear();    
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
    m_artistNameBox->listBox()->clear();
    m_artistNameBox->listBox()->insertStringList(artistList);
    m_artistNameBox->completionObject()->setItems(artistList);

    QStringList albumList = list->uniqueSet(CollectionList::Albums);
    albumList.sort();
    m_albumNameBox->listBox()->clear();
    m_albumNameBox->listBox()->insertStringList(albumList);
    m_albumNameBox->completionObject()->setItems(albumList);

    // Merge the list of genres found in tags with the standard ID3v1 set.

    StringHash genreHash;

    m_genreList = list->uniqueSet(CollectionList::Genres);

    for(QStringList::ConstIterator it = m_genreList.begin(); it != m_genreList.end(); ++it)
	genreHash.insert(*it);

    TagLib::StringList genres = TagLib::ID3v1::genreList();

    for(TagLib::StringList::ConstIterator it = genres.begin(); it != genres.end(); ++it)
	genreHash.insert(TStringToQString((*it)));

    m_genreList = genreHash.values();
    m_genreList.sort();

    m_genreBox->listBox()->clear();
    m_genreBox->listBox()->insertItem(QString::null);
    m_genreBox->listBox()->insertStringList(m_genreList);
    m_genreBox->completionObject()->setItems(m_genreList);
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void TagEditor::readConfig()
{
    // combo box completion modes
	
    KConfigGroup config(KGlobal::config(), "TagEditor");
    if(m_artistNameBox && m_albumNameBox) {
	readCompletionMode(&config, m_artistNameBox, "ArtistNameBoxMode");
	readCompletionMode(&config, m_albumNameBox, "AlbumNameBoxMode");
	readCompletionMode(&config, m_genreBox, "GenreBoxMode");
    }

    bool show = config.readBoolEntry("Show", false);
    action<KToggleAction>("showEditor")->setChecked(show);
    setShown(show);

    TagLib::StringList genres = TagLib::ID3v1::genreList();

    for(TagLib::StringList::ConstIterator it = genres.begin(); it != genres.end(); ++it)
	m_genreList.append(TStringToQString((*it)));
    m_genreList.sort();

    m_genreBox->clear();
    m_genreBox->insertItem(QString::null);
    m_genreBox->insertStringList(m_genreList);
    m_genreBox->completionObject()->setItems(m_genreList);
}

void TagEditor::readCompletionMode(KConfigBase *config, KComboBox *box, const QString &key)
{
    KGlobalSettings::Completion mode =
	KGlobalSettings::Completion(config->readNumEntry(key, KGlobalSettings::CompletionAuto));

    box->setCompletionMode(mode);
}

void TagEditor::saveConfig()
{
    // combo box completion modes

    KConfigGroup config(KGlobal::config(), "TagEditor");

    if(m_artistNameBox && m_albumNameBox) {
	config.writeEntry("ArtistNameBoxMode", m_artistNameBox->completionMode());
	config.writeEntry("AlbumNameBoxMode", m_albumNameBox->completionMode());
	config.writeEntry("GenreBoxMode", m_genreBox->completionMode());
    }
    config.writeEntry("Show", action<KToggleAction>("showEditor")->isChecked());
}

void TagEditor::setupActions()
{
    KToggleAction *show = new KToggleAction(i18n("Show &Tag Editor"), "edit", 0, actions(), "showEditor");
    show->setCheckedState(i18n("Hide &Tag Editor"));
    connect(show, SIGNAL(toggled(bool)), this, SLOT(setShown(bool)));

    new KAction(i18n("&Save"), "filesave", "CTRL+t", this, SLOT(slotSave()), actions(), "saveItem");
}

void TagEditor::setupLayout()
{
    static const int horizontalSpacing = 12;
    static const int verticalSpacing = 2;

    QHBoxLayout *layout = new QHBoxLayout(this, 6, horizontalSpacing);

    //////////////////////////////////////////////////////////////////////////////
    // define two columns of the bottem layout
    //////////////////////////////////////////////////////////////////////////////
    QVBoxLayout *leftColumnLayout = new QVBoxLayout(layout, verticalSpacing);
    QVBoxLayout *rightColumnLayout = new QVBoxLayout(layout, verticalSpacing);

    layout->setStretchFactor(leftColumnLayout, 2);
    layout->setStretchFactor(rightColumnLayout, 3);

    //////////////////////////////////////////////////////////////////////////////
    // put stuff in the left column -- all of the field names are class wide
    //////////////////////////////////////////////////////////////////////////////
    { // just for organization
    	
	m_artistNameBox = new KComboBox(true, this, "artistNameBox");
	m_artistNameBox->setCompletionMode(KGlobalSettings::CompletionAuto);
	addItem(i18n("&Artist name:"), m_artistNameBox, leftColumnLayout, "personal");

        m_trackNameBox = new KLineEdit(this, "trackNameBox");
	addItem(i18n("&Track name:"), m_trackNameBox, leftColumnLayout, "player_play");

	m_albumNameBox = new KComboBox(true, this, "albumNameBox");
	m_albumNameBox->setCompletionMode(KGlobalSettings::CompletionAuto);
	addItem(i18n("Album &name:"), m_albumNameBox, leftColumnLayout, "cdrom_unmount");

        m_genreBox = new KComboBox(true, this, "genreBox");
	addItem(i18n("&Genre:"), m_genreBox, leftColumnLayout, "knotify");

        // this fills the space at the bottem of the left column
        leftColumnLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum,
						  QSizePolicy::Expanding));
    }
    //////////////////////////////////////////////////////////////////////////////
    // put stuff in the right column
    //////////////////////////////////////////////////////////////////////////////
    { // just for organization
	
	QHBoxLayout *fileNameLayout = new QHBoxLayout(rightColumnLayout,
						      horizontalSpacing);

	m_fileNameBox = new KLineEdit(this, "fileNameBox");
	m_fileNameBox->setValidator(new FileNameValidator(m_fileNameBox));	

	QLabel *fileNameIcon = new QLabel(this);
	fileNameIcon->setPixmap(SmallIcon("sound"));
	QWidget *fileNameLabel = addHidden(new QLabel(m_fileNameBox, i18n("&File name:"), this));

	fileNameLayout->addWidget(addHidden(fileNameIcon));
	fileNameLayout->addWidget(fileNameLabel);
	fileNameLayout->setStretchFactor(fileNameIcon, 0);
	fileNameLayout->setStretchFactor(fileNameLabel, 0);
	fileNameLayout->insertStretch(-1, 1);
	rightColumnLayout->addWidget(addHidden(m_fileNameBox));

        { // lay out the track row
            FixedHLayout *trackRowLayout = new FixedHLayout(rightColumnLayout,
							    horizontalSpacing);

	    m_trackSpin = new KIntSpinBox(0, 9999, 1, 0, 10, this, "trackSpin");
	    addItem(i18n("T&rack:"), m_trackSpin, trackRowLayout);
	    m_trackSpin->installEventFilter(this);

	    trackRowLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding,
						    QSizePolicy::Minimum));

	    m_yearSpin = new KIntSpinBox(0, 9999, 1, 0, 10, this, "yearSpin");
	    addItem(i18n("&Year:"), m_yearSpin, trackRowLayout);
	    m_yearSpin->installEventFilter(this);

	    trackRowLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding,
						    QSizePolicy::Minimum));

	    trackRowLayout->addWidget(addHidden(new QLabel(i18n("Length:"), this)));
	    m_lengthBox = new KLineEdit(this, "lengthBox");
	    // addItem(i18n("Length:"), m_lengthBox, trackRowLayout);
	    m_lengthBox->setMinimumWidth(fontMetrics().width("00:00") + trackRowLayout->spacing());
	    m_lengthBox->setMaximumWidth(50);
	    m_lengthBox->setAlignment(Qt::AlignRight);
	    m_lengthBox->setReadOnly(true);
	    trackRowLayout->addWidget(addHidden(m_lengthBox));

	    trackRowLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding,
						    QSizePolicy::Minimum));

	    trackRowLayout->addWidget(addHidden(new QLabel(i18n("Bitrate:"), this)));
	    m_bitrateBox = new KLineEdit(this, "bitrateBox");
	    // addItem(i18n("Bitrate:"), m_bitrateBox, trackRowLayout);
	    m_bitrateBox->setMinimumWidth(fontMetrics().width("000") + trackRowLayout->spacing());
	    m_bitrateBox->setMaximumWidth(50);
	    m_bitrateBox->setAlignment(Qt::AlignRight);
	    m_bitrateBox->setReadOnly(true);
	    trackRowLayout->addWidget(addHidden(m_bitrateBox));

	    trackRowLayout->setWidth();
        }

        m_commentBox = new KEdit(this, "commentBox");
	m_commentBox->setTextFormat(Qt::PlainText);
	addItem(i18n("&Comment:"), m_commentBox, rightColumnLayout, "edit");
    	fileNameLabel->setMinimumHeight(m_trackSpin->height());

    }

    connect(m_artistNameBox, SIGNAL(textChanged(const QString&)),
	    this, SLOT(slotDataChanged()));

    connect(m_trackNameBox, SIGNAL(textChanged(const QString&)),
	    this, SLOT(slotDataChanged()));

    connect(m_albumNameBox, SIGNAL(textChanged(const QString&)),
	    this, SLOT(slotDataChanged()));

    connect(m_genreBox, SIGNAL(activated(int)),
	    this, SLOT(slotDataChanged()));

    connect(m_genreBox, SIGNAL(textChanged(const QString&)),
	    this, SLOT(slotDataChanged()));

    connect(m_fileNameBox, SIGNAL(textChanged(const QString&)),
	    this, SLOT(slotDataChanged()));

    connect(m_yearSpin, SIGNAL(valueChanged(int)),
	    this, SLOT(slotDataChanged()));

    connect(m_trackSpin, SIGNAL(valueChanged(int)),
	    this, SLOT(slotDataChanged()));

    connect(m_commentBox, SIGNAL(textChanged()),
	    this, SLOT(slotDataChanged()));
}

void TagEditor::save(const PlaylistItemList &list)
{
    if(!list.isEmpty() && m_dataChanged) {
	
	KApplication::setOverrideCursor(Qt::waitCursor);
	m_dataChanged = false;
	m_performingSave = true;

	// The list variable can become corrupted if the playlist holding its
	// items dies, which is possible as we edit tags.  So we need to copy
	// the end marker.

	PlaylistItemList::ConstIterator end = list.end();
	
	for(PlaylistItemList::ConstIterator it = list.begin(); it != end; /* Deliberatly missing */ ) {

	    // Process items before we being modifying tags, as the dynamic
	    // playlists will try to modify the file we edit if the tag changes
	    // due to our alterations here.

	    kapp->eventLoop()->processEvents(QEventLoop::ExcludeUserInput);

	    PlaylistItem *item = *it;
	    
	    // The playlist can be deleted from under us if this is the last
	    // item and we edit it so that it doesn't match the search, which
	    // means we can't increment the iterator, so let's do it now.

	    ++it;

	    QString fileName = item->file().fileInfo().dirPath() + QDir::separator() +
	                       m_fileNameBox->text();
	    if(list.count() > 1)
		fileName = item->file().fileInfo().absFilePath();

	    Tag *tag = TagTransactionManager::duplicateTag(item->file().tag(), fileName);
		
	    // A bit more ugliness.  If there are multiple files that are
	    // being modified, they each have a "enabled" checkbox that
	    // says if that field is to be respected for the multiple 
	    // files.  We have to check to see if that is enabled before
	    // each field that we write.
	    
	    if(m_enableBoxes[m_artistNameBox]->isOn())
		tag->setArtist(m_artistNameBox->currentText());
	    if(m_enableBoxes[m_trackNameBox]->isOn())
		tag->setTitle(m_trackNameBox->text());
	    if(m_enableBoxes[m_albumNameBox]->isOn())
		tag->setAlbum(m_albumNameBox->currentText());
	    if(m_enableBoxes[m_trackSpin]->isOn()) {
		if(m_trackSpin->text().isEmpty())
		    m_trackSpin->setValue(0);
		tag->setTrack(m_trackSpin->value());
	    }
	    if(m_enableBoxes[m_yearSpin]->isOn()) {
		if(m_yearSpin->text().isEmpty())
		    m_yearSpin->setValue(0);
		tag->setYear(m_yearSpin->value());
	    }
	    if(m_enableBoxes[m_commentBox]->isOn())
		tag->setComment(m_commentBox->text());
	    
	    if(m_enableBoxes[m_genreBox]->isOn())
		tag->setGenre(m_genreBox->currentText());

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

    for(PlaylistItemList::Iterator it = m_items.begin(); it != m_items.end(); ++it)
	files.append((*it)->file().absFilePath());

    if(KMessageBox::questionYesNoList(this,
				      i18n("Do you want to save your changes to:\n"), 
				      files, 
				      i18n("Save Changes"),
				      KStdGuiItem::save(),
				      KStdGuiItem::discard(),
				      "tagEditor_showSaveChangesBox") == KMessageBox::Yes)
    {
	save(m_items);
    }
}

void TagEditor::addItem(const QString &text, QWidget *item, QBoxLayout *layout, const QString &iconName)
{
    if(!item || !layout)
	return;

    QLabel *label = new QLabel(item, text, this);
    QLabel *iconLabel = new QLabel(item, 0, this);
    
    if(!iconName.isNull())
	iconLabel->setPixmap(SmallIcon(iconName));
    
    QCheckBox *enableBox = new QCheckBox(i18n("Enable"), this);
    enableBox->setChecked(true);

    label->setMinimumHeight(enableBox->height());

    if(layout->direction() == QBoxLayout::LeftToRight) {
    	layout->addWidget(iconLabel);
	layout->addWidget(label);
	layout->addWidget(item);
	layout->addWidget(enableBox);
    }
    else {
	QHBoxLayout *l = new QHBoxLayout(layout);
	
	l->addWidget(iconLabel);
	l->addWidget(label);
	l->setStretchFactor(label, 1);

	l->insertStretch(-1, 1);

	l->addWidget(enableBox);
	l->setStretchFactor(enableBox, 0);

	layout->addWidget(item);
    }

    enableBox->hide();

    connect(enableBox, SIGNAL(toggled(bool)), item, SLOT(setEnabled(bool)));
    m_enableBoxes.insert(item, enableBox);
}

void TagEditor::showEvent(QShowEvent *e)
{
    if(m_collectionChanged) {
	updateCollection();
	slotRefresh();
    }

    QWidget::showEvent(e);
}

bool TagEditor::eventFilter(QObject *watched, QEvent *e)
{
    QKeyEvent *ke = static_cast<QKeyEvent*>(e);
    if(watched->inherits("QSpinBox") && e->type() == QEvent::KeyRelease && ke->state() == 0)
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
    m_items.remove(item);
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
