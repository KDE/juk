/***************************************************************************
    begin                : Thu Oct 28 2004
    copyright            : (C) 2004 by Michael Pyne
                         : (c) 2003 Frerich Raabe <raabe@kde.org>
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

#include <taglib/tstring.h>
#include <taglib/fileref.h>
#include <taglib/tag.h>

#include <algorithm>

#include <kdebug.h>
#include <kcombobox.h>
#include <kurl.h>
#include <kurlrequester.h>
#include <kiconloader.h>
#include <knuminput.h>
#include <kstandarddirs.h>
#include <kio/netaccess.h>
#include <kconfigbase.h>
#include <kconfig.h>
#include <kglobal.h>
#include <klineedit.h>
#include <klocale.h>
#include <kpushbutton.h>
#include <kapplication.h>
#include <kmessagebox.h>

#include <qfile.h>
#include <qhbox.h>
#include <qvbox.h>
#include <qscrollview.h>
#include <qobjectlist.h>
#include <qtimer.h>
#include <qregexp.h>
#include <qcheckbox.h>
#include <qdir.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qsignalmapper.h>
#include <qheader.h>

#include "tag.h"
#include "filehandle.h"
#include "filerenamer.h"
#include "exampleoptions.h"
#include "playlistitem.h"
#include "playlist.h"

class ConfirmationDialog : public KDialogBase
{
public:
    ConfirmationDialog(const QMap<QString, QString> &files,
                       QWidget *parent = 0, const char *name = 0)
        : KDialogBase(parent, name, true, i18n("Warning"), Ok | Cancel)
    {
        QVBox *vbox = makeVBoxMainWidget();
        QHBox *hbox = new QHBox(vbox);

        QLabel *l = new QLabel(hbox);
        l->setPixmap(SmallIcon("messagebox_warning", 32));

        l = new QLabel(i18n("You are about to rename the following files. "
                            "Are you sure you want to continue?"), hbox);
        hbox->setStretchFactor(l, 1);

        KListView *lv = new KListView(vbox);

        lv->addColumn(i18n("Original Name"));
        lv->addColumn(i18n("New Name"));

        int lvHeight = 0;

        QMap<QString, QString>::ConstIterator it = files.begin();
        for(; it != files.end(); ++it) {
            KListViewItem *i = it.key() != it.data()
                ? new KListViewItem(lv, it.key(), it.data())
                : new KListViewItem(lv, it.key(), i18n("No Change"));
            lvHeight += i->height();
        }

        lvHeight += lv->horizontalScrollBar()->height() + lv->header()->height();
        lv->setMinimumHeight(QMIN(lvHeight, 400));
        resize(QMIN(width(), 500), QMIN(minimumHeight(), 400));
    }
};

//
// Implementation of ConfigCategoryReader
//

ConfigCategoryReader::ConfigCategoryReader() : CategoryReaderInterface(),
    m_currentItem(0)
{
    KConfigGroup config(KGlobal::config(), "FileRenamer");

    for(unsigned i = StartTag; i < NumTypes; ++i)
        m_options[i] = TagRenamerOptions(static_cast<TagType>(i));

    for(unsigned i = 0; i < (NumTypes - 1); ++i)
        m_folderSeparators[i] = false;

    QValueList<int> checkedSeparators = config.readIntListEntry("CheckedDirSeparators");
    QValueList<int>::ConstIterator it = checkedSeparators.begin();
    for(; it != checkedSeparators.end(); ++it)
        if(*it > 0 && *it < (NumTypes - 1))
            m_folderSeparators[*it] = true;

    m_musicFolder = config.readPathEntry("MusicFolder", "${HOME}/music");
    m_separator = config.readEntry("Separator", " - ");

    checkedSeparators = config.readIntListEntry("CategoryOrder");
    for(it = checkedSeparators.begin(); it != checkedSeparators.end(); ++it)
        m_categoryOrder.append(static_cast<TagType>(*it));
}

QString ConfigCategoryReader::categoryValue(TagType type) const
{
    if(!m_currentItem)
        return "";

    Tag *tag = m_currentItem->file().tag();

    switch(type) {
    case Track:
        return FileRenamer::fixupTrack(QString::number(tag->track()), *this);

    case Year:
        return QString::number(tag->year());

    case Title:
        return tag->title();

    case Artist:
        return tag->artist();

    case Album:
        return tag->album();

    case Genre:
        return tag->genre();

    default:
        return "";
    }
}

QString ConfigCategoryReader::prefix(TagType category) const
{
    return m_options[category].prefix();
}

QString ConfigCategoryReader::suffix(TagType category) const
{
    return m_options[category].suffix();
}

TagRenamerOptions::EmptyActions ConfigCategoryReader::emptyAction(TagType category) const
{
    return m_options[category].emptyAction();
}

QString ConfigCategoryReader::emptyText(TagType category) const
{
    return m_options[category].emptyText();
}

QValueList<TagType> ConfigCategoryReader::categoryOrder() const
{
    return m_categoryOrder;
}

QString ConfigCategoryReader::separator() const
{
    return m_separator;
}

QString ConfigCategoryReader::musicFolder() const
{
    return m_musicFolder;
}

int ConfigCategoryReader::trackWidth() const
{
    return m_options[Track].trackWidth();
}

bool ConfigCategoryReader::hasFolderSeparator(int index) const
{
    return m_folderSeparators[index];
}

bool ConfigCategoryReader::isDisabled(TagType category) const
{
    return m_options[category].disabled();
}

FileRenamerWidget::FileRenamerWidget(QWidget *parent) :
    FileRenamerBase(parent), CategoryReaderInterface(),
    m_exampleFromFile(false), m_exampleFile(0)
{
    QLabel *temp = new QLabel(0);
    m_exampleText->setPaletteBackgroundColor(temp->paletteBackgroundColor());
    delete temp;

    layout()->setMargin(0); // We'll be wrapped by KDialogBase
    
    // This must be created before createTagRows() is called.

    m_exampleDialog = new ExampleOptionsDialog(this);

    createTagRows();
    loadConfig();

    for(unsigned i = 0; i < NumTypes; ++i) {
        setCategoryEnabled(m_rows[i].category, !m_rows[i].options.disabled());
        m_rows[i].enableButton->setChecked(!m_rows[i].options.disabled());
    }

    connect(m_exampleDialog, SIGNAL(signalShown()), SLOT(exampleDialogShown()));
    connect(m_exampleDialog, SIGNAL(signalHidden()), SLOT(exampleDialogHidden()));
    connect(m_exampleDialog, SIGNAL(dataChanged()), SLOT(dataSelected()));
    connect(m_exampleDialog, SIGNAL(fileChanged(const QString &)),
            this,            SLOT(fileSelected(const QString &)));

    exampleTextChanged();
}

void FileRenamerWidget::loadConfig()
{
    QValueList<int> checkedSeparators;
    KConfigGroup config(KGlobal::config(), "FileRenamer");

    for(unsigned i = StartTag; i < NumTypes; ++i)
        m_rows[i].options = TagRenamerOptions(m_rows[i].category);

    checkedSeparators = config.readIntListEntry("CheckedDirSeparators");

    QValueList<int>::ConstIterator it = checkedSeparators.begin();
    for(; it != checkedSeparators.end(); ++it) {
        if(*it < (NumTypes - 1) && *it >= 0)
            m_folderSwitches[*it]->setChecked(true);
    }

    QString url = config.readPathEntry("MusicDirectory", "${HOME}/music");
    m_musicFolder->setURL(url);

    m_separator->setCurrentText(config.readEntry("Separator", " - "));
}

void FileRenamerWidget::saveConfig()
{
    KConfigGroup config(KGlobal::config(), "FileRenamer");
    QValueList<int> checkedSeparators;
    QValueList<int> categoryOrder;

    for(unsigned i = StartTag; i < NumTypes; ++i)
        m_rows[i].options.saveConfig();

    for(int i = 0; i < NumTypes - 1; ++i)
        if(m_folderSwitches[i]->isChecked() == true)
            checkedSeparators += i;

    config.writeEntry("CheckedDirSeparators", checkedSeparators);

    for(int i = 0; i < NumTypes; ++i)
        categoryOrder += m_rows[i].category;

    config.writeEntry("CategoryOrder", categoryOrder);
    config.writePathEntry("MusicDirectory", m_musicFolder->url());
    config.writeEntry("Separator", m_separator->currentText());

    config.sync();
}

FileRenamerWidget::~FileRenamerWidget()
{
    delete m_exampleFile;
}

void FileRenamerWidget::createTagRows()
{
    KConfigGroup config(KGlobal::config(), "FileRenamer");
    QValueList<int> categoryOrder = config.readIntListEntry("CategoryOrder");
    if(categoryOrder.isEmpty())
        for(int i = 0; i < NumTypes; ++i)
            categoryOrder += i;
    
    QPixmap up   = SmallIcon("up");
    QPixmap down = SmallIcon("down");

    QSignalMapper *mapper       = new QSignalMapper(this, "signal mapper");
    QSignalMapper *toggleMapper = new QSignalMapper(this, "toggle mapper");
    QSignalMapper *upMapper     = new QSignalMapper(this, "up button mapper");
    QSignalMapper *downMapper   = new QSignalMapper(this, "down button mapper");

    connect(mapper,       SIGNAL(mapped(int)), SLOT(showCategoryOption(int)));
    connect(toggleMapper, SIGNAL(mapped(int)), SLOT(toggleCategory(int)));
    connect(upMapper,     SIGNAL(mapped(int)), SLOT(moveItemUp(int)));
    connect(downMapper,   SIGNAL(mapped(int)), SLOT(moveItemDown(int)));

    m_mainFrame = new QVBox(m_mainView->viewport());
    m_mainView->addChild(m_mainFrame);
    m_mainView->setResizePolicy(QScrollView::AutoOneFit);

    // OK, the deal with the categoryOrder variable is that we need to create
    // the rows in the order that they were saved in.  Or at least, this is
    // the easiest way to handle it.  The signal mappers operate according to
    // the category value, whereas most of the other variables (including
    // m_rows) operate on the current position, where 0 is the top.  To get
    // the category value, use m_rows[i].category

    for(TagType i = StartTag; i < NumTypes; /* Empty */) {
        m_rows[i].category = static_cast<TagType>(categoryOrder.front());
        categoryOrder.pop_front();

        QHBox *frame = new QHBox(m_mainFrame);
        m_rows[i].widget = frame;
        frame->setFrameShape(QFrame::Box);
        frame->setLineWidth(1);
        frame->setMargin(3);

        m_mainFrame->setStretchFactor(frame, 1);
        m_mainFrame->setMargin(2);
        m_mainFrame->setSpacing(5);

        QVBox *buttons = new QVBox(frame);

        m_rows[i].upButton = new KPushButton(buttons);
        m_rows[i].downButton = new KPushButton(buttons);

        m_rows[i].upButton->setPixmap(up);
        m_rows[i].downButton->setPixmap(down);
        m_rows[i].upButton->setFlat(true);
        m_rows[i].downButton->setFlat(true);

        upMapper->connect(m_rows[i].upButton, SIGNAL(clicked()), SLOT(map()));
        upMapper->setMapping(m_rows[i].upButton, m_rows[i].category);
        downMapper->connect(m_rows[i].downButton, SIGNAL(clicked()), SLOT(map()));
        downMapper->setMapping(m_rows[i].downButton, m_rows[i].category);

        QString labelText = QString("<b>%1</b>").arg(TagRenamerOptions::getTagTypeText(m_rows[i].category));
        QLabel *label = new QLabel(labelText, frame);
        frame->setStretchFactor(label, 1);
        label->setAlignment(AlignCenter);

        QVBox *options = new QVBox(frame);
        m_rows[i].enableButton = new QCheckBox(i18n("Enabled"), options);
        m_rows[i].enableButton->setChecked(true);
        toggleMapper->connect(m_rows[i].enableButton, SIGNAL(toggled(bool)), SLOT(map()));
        toggleMapper->setMapping(m_rows[i].enableButton, m_rows[i].category);

        KPushButton *optionsButton = new KPushButton(i18n("Options..."), options);
        mapper->connect(optionsButton, SIGNAL(clicked()), SLOT(map()));
        mapper->setMapping(optionsButton, m_rows[i].category);

        // Insert the directory separator checkbox if this isn't the last
        // item.

        if(i < (NumTypes - 1)) {
            QWidget *temp = new QWidget(m_mainFrame);
            QHBoxLayout *l = new QHBoxLayout(temp);

            m_folderSwitches[i] = new QCheckBox(i18n("Insert Folder Separator"), temp);
            l->addWidget(m_folderSwitches[i], 0, AlignCenter);

            connect(m_folderSwitches[i], SIGNAL(toggled(bool)),
                    SLOT(exampleTextChanged()));
        }

        // Yes, this is a hack

        int destroyerOfWorlds = i;
        i = static_cast<TagType>(++destroyerOfWorlds);
    }

    m_rows[0].upButton->setEnabled(false);
    m_rows[NumTypes - 1].downButton->setEnabled(false);
}

void FileRenamerWidget::exampleTextChanged()
{
    // Just use .mp3 as an example

    if(m_exampleFromFile && (!m_exampleFile || !m_exampleFile->file())) {
        m_exampleText->setText(i18n("No file selected, or selected file has no tags."));
        return;
    }

    m_exampleText->setText(FileRenamer::fileName(*this) + ".mp3");
}

QString FileRenamerWidget::fileCategoryValue(TagType category) const
{
    TagLib::Tag *tag = m_exampleFile->tag();

    switch(category) {
    case Track:
        return FileRenamer::fixupTrack(QString::number(tag->track()), *this);

    case Year:
        return QString::number(tag->year());

    case Title:
        return TStringToQString(tag->title());

    case Artist:
        return TStringToQString(tag->artist());

    case Album:
        return TStringToQString(tag->album());

    case Genre:
        return TStringToQString(tag->genre());

    default:
        return "";
    }
}

QString FileRenamerWidget::categoryValue(TagType category) const
{
    if(m_exampleFromFile)
        return fileCategoryValue(category);

    const ExampleOptions *example = m_exampleDialog->widget();

    switch (category) {
    case Track:
        return FileRenamer::fixupTrack(example->m_exampleTrack->text(), *this);

    case Year:
        return example->m_exampleYear->text();

    case Title:
        return example->m_exampleTitle->text();

    case Artist:
        return example->m_exampleArtist->text();

    case Album:
        return example->m_exampleAlbum->text();

    case Genre:
        return example->m_exampleGenre->text();

    default:
        return "";
    }
}

QValueList<TagType> FileRenamerWidget::categoryOrder() const
{
    QValueList<TagType> list;

    for(unsigned i = 0; i < NumTypes; ++i)
        list.append(m_rows[i].category);

    return list;
}

bool FileRenamerWidget::hasFolderSeparator(int index) const
{
    return m_folderSwitches[index]->isChecked();
}

void FileRenamerWidget::moveItem(QWidget *l, MovementDirection direction)
{
    int pos = findIndex(l);

    if(pos < 0) {
        kdError() << "Unable to find index for " << l << endl;
        return;
    }

    int delta = 1;

    // This is used to make the following code more or less
    // direction-independant.
    
    if(direction == MoveUp)
        delta = -1;

    // Item we're moving can't go further down after this.

    if((pos == (NumTypes - 2) && direction == MoveDown) ||
       (pos == (NumTypes - 1) && direction == MoveUp))
    {
        m_rows[NumTypes - 1].downButton->setEnabled(true);
        m_rows[NumTypes - 2].downButton->setEnabled(false);
    }

    // We're moving the top item, do some button switching.

    if((pos == 0 && direction == MoveDown) || (pos == 1 && direction == MoveUp)) {
        m_rows[0].upButton->setEnabled(true);
        m_rows[1].upButton->setEnabled(false);
    }

    // This is the item we're swapping with.

    QWidget *w = m_rows[pos + delta].widget;

    // Update the table of widget rows.

    std::swap(m_rows[pos], m_rows[pos + delta]);

    // Move the item two spaces above/below its previous position.  It has to
    // be 2 spaces because of the checkbox.

    QBoxLayout *layout = dynamic_cast<QBoxLayout *>(m_mainFrame->layout());

    layout->remove(l);
    layout->insertWidget(2 * (pos + delta), l);

    // Move the top item two spaces in the opposite direction, for a similar
    // reason.

    layout->remove(w);
    layout->insertWidget(pos * 2, w);
    layout->invalidate();

    setCategoryEnabled(pos + delta, !m_rows[pos + delta].options.disabled());
    setCategoryEnabled(pos, !m_rows[pos].options.disabled());

    QTimer::singleShot(0, this, SLOT(exampleTextChanged()));
}

int FileRenamerWidget::findIndex(TagType category) const
{
    for(int index = 0; index < NumTypes; ++index)
        if(m_rows[index].category == category)
            return index;

    return -1;
}

int FileRenamerWidget::findIndex(QWidget *item) const
{
    for(int index = 0; index < NumTypes; ++index)
        if(m_rows[index].widget == item)
            return index;

    return -1;
}

void FileRenamerWidget::enableAllUpButtons()
{
    for(unsigned i = 0; i < NumTypes; ++i)
        m_rows[i].upButton->setEnabled(true);
}

void FileRenamerWidget::enableAllDownButtons()
{
    for(unsigned i = 0; i < NumTypes; ++i)
        m_rows[i].downButton->setEnabled(true);
}

void FileRenamerWidget::showCategoryOption(int category)
{
    showCategoryOptions(static_cast<TagType>(category));
}

void FileRenamerWidget::showCategoryOptions(TagType category)
{
    TagOptionsDialog *dialog = new TagOptionsDialog(this, m_rows[findIndex(category)].options);

    if(dialog->exec() == QDialog::Accepted) {
        m_rows[findIndex(category)].options = dialog->options();
        exampleTextChanged();
    }

    delete dialog;
}

void FileRenamerWidget::setCategoryEnabled(int index, bool enable)
{
    bool changed = m_rows[index].options.disabled() == enable;

    m_rows[index].options.setDisabled(!enable);

    if(index < (NumTypes - 1)) {
        changed = changed || m_folderSwitches[index]->isChecked() != enable;
        m_folderSwitches[index]->setEnabled(enable);
    }

    // Only call this if we actually changed something, since it sorta
    // takes some time to process, and we want to avoid flicker if possible.

    if(changed)
        exampleTextChanged();
}

void FileRenamerWidget::moveItemUp(int category)
{
    TagType tag = static_cast<TagType>(category);

    moveItem(widgetForCategory(tag), MoveUp);
}

void FileRenamerWidget::moveItemDown(int category)
{
    TagType tag = static_cast<TagType>(category);

    moveItem(widgetForCategory(tag), MoveDown);
}

void FileRenamerWidget::toggleExampleDialog()
{
    m_exampleDialog->setShown(!m_exampleDialog->isShown());
}

void FileRenamerWidget::exampleDialogShown()
{
    m_showExample->setText(i18n("Hide Renamer Test Dialog"));
}

void FileRenamerWidget::exampleDialogHidden()
{
    m_showExample->setText(i18n("Show Renamer Test Dialog"));
}

void FileRenamerWidget::fileSelected(const QString &file)
{
    m_exampleFromFile = true;
    delete m_exampleFile;
    m_exampleFile = new TagLib::FileRef(QFile::encodeName(file));

    exampleTextChanged();
}

void FileRenamerWidget::dataSelected()
{
    m_exampleFromFile = false;
    exampleTextChanged();
}

QString FileRenamerWidget::separator() const
{
    return m_separator->currentText();
}

QString FileRenamerWidget::musicFolder() const
{
    return m_musicFolder->url();
}

void FileRenamerWidget::toggleCategory(int category)
{
    QCheckBox *b = 0;

    // Find checkbox that matches this category

    for(unsigned i = 0; i < NumTypes; ++i) {
        if(m_rows[i].category == category) {
            b = m_rows[i].enableButton;
            break;
        }
    }

    if(!b) {
        kdError() << "Unable to match category " << category << " to a check box!\n";
        return;
    }

    setCategoryEnabled(findIndex(static_cast<TagType>(category)), b->isChecked());
}


FileRenamer::FileRenamer()
{
}

void FileRenamer::rename(PlaylistItem *item)
{
    PlaylistItemList list;
    list.append(item);

    rename(list);
}

void FileRenamer::rename(const PlaylistItemList &items)
{
    ConfigCategoryReader reader;
    QStringList errorFiles;
    QMap<QString, QString> map;
    QMap<QString, PlaylistItem *> itemMap;

    for(PlaylistItemList::ConstIterator it = items.begin(); it != items.end(); ++it) {
        reader.setPlaylistItem(*it);
        QString oldFile = (*it)->file().absFilePath();
        QString extension = (*it)->file().fileInfo().extension(false);
        QString newFile = fileName(reader) + "." + extension;

        if(oldFile != newFile) {
            map[oldFile] = newFile;
            itemMap[oldFile] = *it;
        }
    }

    if(ConfirmationDialog(map).exec() != QDialog::Accepted)
        return;

    KApplication::setOverrideCursor(Qt::waitCursor);
    for(QMap<QString, QString>::ConstIterator it = map.begin();
        it != map.end(); ++it)
    {
        if(moveFile(it.key(), it.data()))
            itemMap[it.key()]->setFile(FileHandle(it.data()));
        else
            errorFiles << i18n("%1 to %2").arg(it.key()).arg(it.data());

        processEvents();
    }
    KApplication::restoreOverrideCursor();

    if(!errorFiles.isEmpty())
        KMessageBox::error(0, i18n("The following rename operations failed:\n") + errorFiles.join("\n"));
}

bool FileRenamer::moveFile(const QString &src, const QString &dest)
{
    kdDebug(65432) << "Moving file " << src << " to " << dest << endl;

    if(src == dest)
        return false;

    // Escape URL.
    KURL srcURL = KURL::fromPathOrURL(src);
    KURL dstURL = KURL::fromPathOrURL(dest);

    // Clean it.
    srcURL.cleanPath();
    dstURL.cleanPath();

    // Make sure it is valid.
    if(!srcURL.isValid() || !dstURL.isValid())
        return false;

    // Get just the directory.
    KURL dir = dstURL;
    dir.setFileName(QString::null);

    // Create the directory.
    if(!KStandardDirs::exists(dir.path()))
        if(!KStandardDirs::makeDir(dir.path())) {
            kdError() << "Unable to create directory " << dir.path() << endl;
            return false;
        }

    // Move the file.
    return KIO::NetAccess::file_move(srcURL, dstURL);
}

QString FileRenamer::fileName(const CategoryReaderInterface &interface)
{
    const QValueList<TagType> categoryOrder = interface.categoryOrder();
    const QString separator = interface.separator();
    const QString folder = interface.musicFolder();
    const QRegExp closeBracket("[])}]\\s*$");
    const QRegExp openBracket("^\\s*[[({]");

    TagType category;
    unsigned i = 0;
    QString value;
    QStringList list;

    for(QValueList<TagType>::ConstIterator it = categoryOrder.begin(); it != categoryOrder.end(); ++it) {
        category = static_cast<TagType>(*it);
        if(interface.isDisabled(category)) {
            ++i;
            continue;
        }
        
        value = interface.value(category);

        if(i < (NumTypes - 1) && interface.hasFolderSeparator(i))
            value.append("/");
        ++i;

        if(interface.isRequired(category) || !interface.isEmpty(category))
            list.append(value);
    }

    // Construct a single string representation, handling strings ending in
    // '/' specially

    QString result;
    for(QStringList::ConstIterator it = list.begin(); it != list.end(); /* Empty */) {
        result += *it;
        if((*it).find(closeBracket) != -1) {
            ++it;
            continue;
        }

        ++it;
        if(it != list.end() &&
           !result.endsWith(QChar(QDir::separator())) &&
           (*it).find(openBracket) == -1)
        {
            result += separator;
        }
    }
    
    return QString(folder + QDir::separator() + result).simplifyWhiteSpace();
}

QString FileRenamer::fixupTrack(const QString &track, const CategoryReaderInterface &interface)
{
    QString str(track);

    if(track == "0") {
        if(interface.emptyAction(Track) == TagRenamerOptions::UseReplacementValue)
            str = interface.emptyText(Track);
        else
            return "";
    }

    unsigned minimumWidth = interface.trackWidth();

    if(str.length() < minimumWidth) {
        QString prefix;
        prefix.fill('0', minimumWidth - str.length());
        return prefix + str;
    }

    return str;
}

#include "filerenamer.moc"

// vim: set et sw=4 ts=8:
