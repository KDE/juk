/**
 * Copyright (C) 2004, 2007, 2009 Michael Pyne <mpyne@kde.org>
 * Copyright (C) 2003 Frerich Raabe <raabe@kde.org>
 * Copyright (C) 2014 Arnold Dumas <contact@arnolddumas.fr>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "filerenamer.h"

#include <algorithm>
#include <utility>

#include <kiconloader.h>
#include <KLocalizedString>
#include <KIO/FileCopyJob>
#include <kdesktopfile.h>
#include <kconfiggroup.h>
#include <KSharedConfig>
#include <klineedit.h>
#include <kmessagebox.h>

#include <QFile>
#include <QTimer>
#include <QCheckBox>
#include <QDir>
#include <QDialog>
#include <QDialogButtonBox>
#include <QUrl>
#include <QLabel>
#include <QPixmap>
#include <QFrame>
#include <QStandardPaths>
#include <QTreeWidget>
#include <QScrollBar>
#include <QPushButton>
#include <QHeaderView>

#include "coverinfo.h"
#include "exampleoptions.h"
#include "filehandle.h"
#include "filerenameroptions.h"
#include "iconsupport.h"
#include "juk_debug.h"
#include "juktag.h"
#include "playlist.h" // processEvents()
#include "playlistitem.h"

using namespace Qt::Literals::StringLiterals;
using std::as_const;

static QString defaultMusicDir()
{
    const QString defaultDir(QStandardPaths::writableLocation(QStandardPaths::MusicLocation));
    const QString home(QDir::homePath());

    // replace home dir path with ~, if present

    if(defaultDir.startsWith(home)) {
        return u"~"_s + defaultDir.sliced(home.length());
    }
    else {
        return defaultDir;
    }
}

class ConfirmationDialog : public QDialog
{
public:
    ConfirmationDialog(const QMap<QString, QString> &files,
                       QWidget *parent = nullptr)
        : QDialog(parent)
    {
        using namespace IconSupport; // ""_icon

        setModal(true);
        setWindowTitle(i18nc("@title:window, warning about mass file rename", "Warning"));

        auto vboxLayout = new QVBoxLayout(this);
        auto hbox = new QWidget(this);
        auto hboxVLayout = new QVBoxLayout(hbox);
        vboxLayout->addWidget(hbox);

        QLabel *l = new QLabel(hbox);
        l->setPixmap(("dialog-warning"_icon).pixmap(KIconLoader::SizeLarge));
        hboxVLayout->addWidget(l);

        l = new QLabel(i18n("You are about to rename the following files. "
                            "Are you sure you want to continue?"), hbox);
        hboxVLayout->addWidget(l);

        QTreeWidget *lv = new QTreeWidget(this);

        QStringList headers;
        headers << i18n("Original Name");
        headers << i18n("New Name");

        lv->setHeaderLabels(headers);
        lv->setRootIsDecorated(false);
        vboxLayout->addWidget(lv, 1);

        auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Yes | QDialogButtonBox::Cancel, this);
        vboxLayout->addWidget(buttonBox);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

        int lvHeight = 0;

        for(const auto [oldName, newName] : files.asKeyValueRange()) {
            QTreeWidgetItem *item = new QTreeWidgetItem(lv);
            item->setText(0, oldName);

            if (oldName != newName) {
                item->setText(1, newName);
            }
            else {
                item->setText(1, i18n("No Change"));
            }

            lvHeight += lv->visualItemRect(item).height();
        }

        lvHeight += lv->horizontalScrollBar()->height() + lv->header()->height();
        lv->setMinimumHeight(qMin(lvHeight, 400));

        resize(qMin(width(), 500), qMin(minimumHeight(), 400));

        show();
    }
};

//
// Implementation of ConfigCategoryReader
//

ConfigCategoryReader::ConfigCategoryReader()
  : CategoryReaderInterface()
{
    KConfigGroup config(KSharedConfig::openConfig(), u"FileRenamer"_s);

    const QList<int> defaultCategories = { Artist, Album, Title, Track };
    const QList<int> categoryOrder = config.readEntry("CategoryOrder", defaultCategories);
    int categoryCount[NumTypes] = { 0 }; // Keep track of each category encountered.

    for(int catIdx : categoryOrder) {
        int catCount = categoryCount[catIdx]++;
        TagType category = static_cast<TagType>(catIdx);
        CategoryID catId(category, catCount);

        m_options[catId] = TagRenamerOptions(catId);
        m_categoryOrder << catId;
    }

    m_folderSeparators.fill(false, m_categoryOrder.count() - 1);

    const auto checkedSeparators = config.readEntry("CheckedDirSeparators", QList<int>());

    for(int sepIdx : checkedSeparators) {
        if(sepIdx < m_folderSeparators.count())
            m_folderSeparators[sepIdx] = true;
    }

    m_musicFolder = config.readPathEntry("MusicFolder", defaultMusicDir());
    m_separator = config.readEntry("Separator", " - ");
}

QString ConfigCategoryReader::categoryValue(TagType type) const
{
    if(!m_currentItem)
        return QString();

    Tag *tag = m_currentItem->file().tag();

    switch(type) {
    case Track:
        return QString::number(tag->track());

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
        return QString();
    }
}

QString ConfigCategoryReader::prefix(const CategoryID &category) const
{
    return m_options[category].prefix();
}

QString ConfigCategoryReader::suffix(const CategoryID &category) const
{
    return m_options[category].suffix();
}

TagRenamerOptions::EmptyActions ConfigCategoryReader::emptyAction(const CategoryID &category) const
{
    return m_options[category].emptyAction();
}

QString ConfigCategoryReader::emptyText(const CategoryID &category) const
{
    return m_options[category].emptyText();
}

QList<CategoryID> ConfigCategoryReader::categoryOrder() const
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

int ConfigCategoryReader::trackWidth(int categoryNum) const
{
    return m_options[CategoryID(Track, categoryNum)].trackWidth();
}

bool ConfigCategoryReader::hasFolderSeparator(int index) const
{
    if(index >= m_folderSeparators.count())
        return false;
    return m_folderSeparators[index];
}

bool ConfigCategoryReader::isDisabled(const CategoryID &category) const
{
    return m_options[category].disabled();
}

//
// Implementation of FileRenamerWidget
//

FileRenamerWidget::FileRenamerWidget(QWidget *parent) :
    QWidget(parent),
    CategoryReaderInterface(),
    m_ui(new Ui::FileRenamerBase),
    m_exampleFromFile(false)
{
    m_ui->setupUi(this);

    // This must be created before createTagRows() is called.

    m_exampleDialog = new ExampleOptionsDialog(this);

    createTagRows();
    loadConfig();

    // Add correct text to combo box.
    m_ui->m_category->clear();
    for(int i = StartTag; i < NumTypes; ++i) {
        QString category = TagRenamerOptions::tagTypeText(static_cast<TagType>(i));
        m_ui->m_category->addItem(category);
    }

    connect(m_exampleDialog, &ExampleOptionsDialog::signalShown,
            this,            &FileRenamerWidget::exampleDialogShown);
    connect(m_exampleDialog, &ExampleOptionsDialog::signalHidden,
            this,            &FileRenamerWidget::exampleDialogHidden);
    connect(m_exampleDialog, &ExampleOptionsDialog::dataChanged,
            this,            &FileRenamerWidget::dataSelected);
    connect(m_exampleDialog, &ExampleOptionsDialog::fileChanged,
            this,            &FileRenamerWidget::fileSelected);
    connect(m_ui->dlgButtonBox, &QDialogButtonBox::accepted, this, [this]() {
                emit accepted();
            });
    connect(m_ui->dlgButtonBox, &QDialogButtonBox::rejected, this, [this]() {
                emit rejected();
            });

    exampleTextChanged();
}

void FileRenamerWidget::loadConfig()
{
    QList<int> checkedSeparators;
    KConfigGroup config(KSharedConfig::openConfig(), u"FileRenamer"_s);

    for(int i = 0; i < m_rows.count(); ++i)
        m_rows[i].options = TagRenamerOptions(m_rows[i].category);

    checkedSeparators = config.readEntry("CheckedDirSeparators", QList<int>());

    for(int separator : as_const(checkedSeparators)) {
        if(separator < m_folderSwitches.count())
            m_folderSwitches[separator]->setChecked(true);
    }

    QString path = config.readEntry("MusicFolder", defaultMusicDir());
    m_ui->m_musicFolder->setUrl(QUrl::fromLocalFile(path));
    m_ui->m_musicFolder->setMode(KFile::Directory |
                                 KFile::ExistingOnly |
                                 KFile::LocalOnly);

    m_ui->m_separator->setEditText(config.readEntry("Separator", " - "));
}

void FileRenamerWidget::saveConfig()
{
    KConfigGroup config(KSharedConfig::openConfig(), u"FileRenamer"_s);
    QList<int> checkedSeparators;
    QList<int> categoryOrder;

    for(int i = 0; i < m_rows.count(); ++i) {
        int rowId = idOfPosition(i); // Write out in GUI order, not m_rows order
        m_rows[rowId].options.saveConfig(m_rows[rowId].category.categoryNumber);
        categoryOrder += m_rows[rowId].category.category;
    }

    for(int i = 0; i < m_folderSwitches.count(); ++i)
        if(m_folderSwitches[i]->isChecked() == true)
            checkedSeparators += i;

    config.writeEntry("CheckedDirSeparators", checkedSeparators);
    config.writeEntry("CategoryOrder", categoryOrder);
    config.writePathEntry("MusicFolder", m_ui->m_musicFolder->url().path());
    config.writeEntry("Separator", m_ui->m_separator->currentText());

    config.sync();
}

FileRenamerWidget::~FileRenamerWidget()
{
}

int FileRenamerWidget::addRowCategory(TagType category)
{
    using namespace IconSupport; // ""_icon

    static QIcon up   = "go-up"_icon;
    static QIcon down = "go-down"_icon;

    // Find number of categories already of this type.
    int categoryCount = std::count_if(m_rows.cbegin(), m_rows.cend(),
            [category](const Row &r) { return r.category.category == category; }
            );

    Row row;

    row.category = CategoryID(category, categoryCount);
    row.position = m_rows.count();

    QFrame *frame = new QFrame(m_mainFrame);
    QHBoxLayout *frameLayout = new QHBoxLayout(frame);
    frameLayout->setContentsMargins(3, 3, 3, 3);

    row.widget = frame;
    frame->setFrameShape(QFrame::Box);
    frame->setLineWidth(1);

    QBoxLayout *mainFrameLayout = static_cast<QBoxLayout *>(m_mainFrame->layout());
    mainFrameLayout->addWidget(frame, 1);

    QFrame *buttons = new QFrame(frame);
    QVBoxLayout *buttonLayout = new QVBoxLayout(buttons);
    frameLayout->addWidget(buttons);
    buttons->setFrameStyle(QFrame::Plain | QFrame::Box);
    buttons->setLineWidth(1);

    row.upButton = new QPushButton(buttons);
    row.downButton = new QPushButton(buttons);

    row.upButton->setIcon(up);
    row.downButton->setIcon(down);
    row.upButton->setFlat(true);
    row.downButton->setFlat(true);

    buttonLayout->addWidget(row.upButton);
    buttonLayout->addWidget(row.downButton);

    QString labelText = QString("<b>%1</b>").arg(TagRenamerOptions::tagTypeText(category));
    QLabel *label = new QLabel(labelText, frame);
    frameLayout->addWidget(label, 1);
    label->setAlignment(Qt::AlignCenter);

    QVBoxLayout *optionLayout = new QVBoxLayout;
    frameLayout->addLayout(optionLayout);

    row.enableButton = new QPushButton(i18nc("@action:button, remove music genre from file renamer", "Remove"), frame);
    optionLayout->addWidget(row.enableButton);

    row.optionsButton = new QPushButton(i18nc("@action:button, file renamer genre options", "Options"), frame);
    optionLayout->addWidget(row.optionsButton);

    row.widget->show();
    m_rows.append(row);

    assignPositionHandlerForRow(row);

    // Disable add button if there's too many rows.
    if(m_rows.count() == MAX_CATEGORIES)
        m_ui->m_insertCategory->setEnabled(false);

    return row.position;
}

void FileRenamerWidget::assignPositionHandlerForRow(Row &row)
{
    const auto id = row.position;

    disconnect(row.upButton);
    disconnect(row.downButton);
    disconnect(row.enableButton);
    disconnect(row.optionsButton);

    connect(row.upButton, &QPushButton::clicked, this, [this, id]() {
            this->moveItemUp(id);
        });
    connect(row.downButton, &QPushButton::clicked, this, [this, id]() {
            this->moveItemDown(id);
        });
    connect(row.enableButton, &QPushButton::clicked, this, [this, id]() {
            this->slotRemoveRow(id);
        });
    connect(row.optionsButton, &QPushButton::clicked, this, [this, id]() {
            this->showCategoryOption(id);
        });
}

bool FileRenamerWidget::removeRow(int id)
{
    if(id >= m_rows.count()) {
        qCWarning(JUK_LOG) << "Trying to remove row, but " << id << " is out-of-range.\n";
        return false;
    }

    if(m_rows.count() == 1) {
        qCCritical(JUK_LOG) << "Can't remove last row of File Renamer.\n";
        return false;
    }

    delete m_rows[id].widget;
    m_rows[id].widget        = nullptr;
    m_rows[id].enableButton  = nullptr;
    m_rows[id].upButton      = nullptr;
    m_rows[id].optionsButton = nullptr;
    m_rows[id].downButton    = nullptr;

    int checkboxPosition = 0; // Remove first checkbox.

    // If not the first row, remove the checkbox before it.
    if(m_rows[id].position > 0)
        checkboxPosition = m_rows[id].position - 1;

    // The checkbox is contained within a layout widget, so the layout
    // widget is the one the needs to die.
    delete m_folderSwitches[checkboxPosition]->parent();
    m_folderSwitches.removeAll(m_folderSwitches[checkboxPosition]);

    // Go through all the rows and if they have the same category and a
    // higher categoryNumber, decrement the number.  Also update the
    // position identifier.
    for(int i = 0; i < m_rows.count(); ++i) {
        if(i == id)
            continue; // Don't mess with ourself.

        if((m_rows[id].category.category == m_rows[i].category.category) &&
           (m_rows[id].category.categoryNumber < m_rows[i].category.categoryNumber))
        {
            --m_rows[i].category.categoryNumber;
        }

        // Items are moving up.
        if(m_rows[id].position < m_rows[i].position)
            --m_rows[i].position;
    }

    // Every row after the one we delete will have a different identifier, since
    // the identifier is simply its index into m_rows.  So we need to re-do the
    // signal mappings for the affected rows after updating its position.
    for(int i = id + 1; i < m_rows.count(); ++i)
        assignPositionHandlerForRow(m_rows[i]);

    m_rows.remove(id);

    // Make sure we update the buttons of affected rows.
    m_rows[idOfPosition(0)].upButton->setEnabled(false);
    m_rows[idOfPosition(m_rows.count() - 1)].downButton->setEnabled(false);

    // We can insert another row now, make sure GUI is updated to match.
    m_ui->m_insertCategory->setEnabled(true);

    QTimer::singleShot(0, this, &FileRenamerWidget::exampleTextChanged);
    return true;
}

void FileRenamerWidget::addFolderSeparatorCheckbox()
{
    QWidget *temp = new QWidget(m_mainFrame);
    m_mainFrame->layout()->addWidget(temp);

    QHBoxLayout *l = new QHBoxLayout(temp);

    QCheckBox *cb = new QCheckBox(i18nc("@option:check", "Insert folder separator"), temp);
    m_folderSwitches.append(cb);
    l->addWidget(cb, 0, Qt::AlignCenter);
    cb->setChecked(false);

    connect(cb, &QCheckBox::toggled, this, &FileRenamerWidget::exampleTextChanged);

    temp->show();
}

void FileRenamerWidget::createTagRows()
{
    KConfigGroup config(KSharedConfig::openConfig(), u"FileRenamer"_s);
    QList<int> categoryOrder = config.readEntry("CategoryOrder", QList<int>());

    if(categoryOrder.isEmpty())
        categoryOrder << Artist << Album << Title << Track;

    // Setup arrays.
    m_rows.reserve(categoryOrder.count());
    m_folderSwitches.reserve(categoryOrder.count() - 1);

    m_mainFrame = new QFrame(m_ui->m_mainView);
    m_ui->m_mainView->setWidget(m_mainFrame);
    m_ui->m_mainView->setWidgetResizable(true);

    QVBoxLayout *frameLayout = new QVBoxLayout(m_mainFrame);
    frameLayout->setContentsMargins(10, 10, 10, 10);
    frameLayout->setSpacing(5);

    // OK, the deal with the categoryOrder variable is that we need to create
    // the rows in the order that they were saved in (the order given by categoryOrder).
    // The signal mappers operate according to the row identifier.  To find the position of
    // a row given the identifier, use m_rows[id].position.  To find the id of a given
    // position, use idOfPosition(position).

    for(auto it = categoryOrder.cbegin(); it != categoryOrder.cend(); ++it) {
        if(*it < StartTag || *it >= NumTypes) {
            qCCritical(JUK_LOG) << "Invalid category encountered in file renamer configuration.\n";
            continue;
        }

        if(m_rows.count() == MAX_CATEGORIES) {
            qCCritical(JUK_LOG) << "Maximum number of File Renamer tags reached, bailing.\n";
            break;
        }

        addRowCategory(static_cast<TagType>(*it));

        // Insert the directory separator checkbox if this isn't the last
        // item.

        if((it + 1) != categoryOrder.constEnd())
            addFolderSeparatorCheckbox();
    }

    m_rows.first().upButton->setEnabled(false);
    m_rows.last().downButton->setEnabled(false);

    // If we have maximum number of categories already, don't let the user
    // add more.
    if(m_rows.count() >= MAX_CATEGORIES)
        m_ui->m_insertCategory->setEnabled(false);
}

void FileRenamerWidget::exampleTextChanged()
{
    // Just use .mp3 as an example
    if(m_exampleFromFile && (m_exampleFile.isEmpty() ||
                             !FileHandle(m_exampleFile).tag()->isValid()))
    {
        m_ui->m_exampleText->setText(i18n("No file selected, or selected file has no tags."));
        return;
    }

    m_ui->m_exampleText->setText(FileRenamer::fileName(*this) + ".mp3");
}

QString FileRenamerWidget::fileCategoryValue(TagType category) const
{
    FileHandle file(m_exampleFile);
    Tag *tag = file.tag();

    switch(category) {
    case Track:
        return QString::number(tag->track());

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
        return QString();
    }
}

QString FileRenamerWidget::categoryValue(TagType category) const
{
    if(m_exampleFromFile)
        return fileCategoryValue(category);

    const ExampleOptions *example = m_exampleDialog->widget();

    switch (category) {
    case Track:
        return example->m_exampleTrack->text();

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
        return QString();
    }
}

QList<CategoryID> FileRenamerWidget::categoryOrder() const
{
    QList<CategoryID> list;

    // Iterate in GUI row order.
    for(int i = 0; i < m_rows.count(); ++i) {
        int rowId = idOfPosition(i);
        list += m_rows[rowId].category;
    }

    return list;
}

bool FileRenamerWidget::hasFolderSeparator(int index) const
{
    if(index >= m_folderSwitches.count())
        return false;
    return m_folderSwitches[index]->isChecked();
}

void FileRenamerWidget::moveItem(int id, MovementDirection direction)
{
    QWidget *l = m_rows[id].widget;
    int bottom = m_rows.count() - 1;
    int pos = m_rows[id].position;
    int newPos = (direction == MoveUp) ? pos - 1 : pos + 1;

    // Item we're moving can't go further down after this.

    if((pos == (bottom - 1) && direction == MoveDown) ||
       (pos == bottom && direction == MoveUp))
    {
        int idBottomRow = idOfPosition(bottom);
        int idAboveBottomRow = idOfPosition(bottom - 1);

        m_rows[idBottomRow].downButton->setEnabled(true);
        m_rows[idAboveBottomRow].downButton->setEnabled(false);
    }

    // We're moving the top item, do some button switching.

    if((pos == 0 && direction == MoveDown) || (pos == 1 && direction == MoveUp)) {
        int idTopItem = idOfPosition(0);
        int idBelowTopItem = idOfPosition(1);

        m_rows[idTopItem].upButton->setEnabled(true);
        m_rows[idBelowTopItem].upButton->setEnabled(false);
    }

    // This is the item we're swapping with.

    int idSwitchWith = idOfPosition(newPos);
    QWidget *w = m_rows[idSwitchWith].widget;

    // Update the table of widget rows.

    std::swap(m_rows[id].position, m_rows[idSwitchWith].position);

    // Move the item two spaces above/below its previous position.  It has to
    // be 2 spaces because of the checkbox.

    QBoxLayout *layout = dynamic_cast<QBoxLayout *>(m_mainFrame->layout());
    if ( !layout )
        return;

    layout->removeWidget(l);
    layout->insertWidget(2 * newPos, l);

    // Move the top item two spaces in the opposite direction, for a similar
    // reason.

    layout->removeWidget(w);
    layout->insertWidget(2 * pos, w);
    layout->invalidate();

    QTimer::singleShot(0, this, &FileRenamerWidget::exampleTextChanged);
}

int FileRenamerWidget::idOfPosition(int position) const
{
    if(position >= m_rows.count()) {
        qCCritical(JUK_LOG) << "Search for position " << position << " out-of-range.\n";
        return -1;
    }

    for(int i = 0; i < m_rows.count(); ++i)
        if(m_rows[i].position == position)
            return i;

    qCCritical(JUK_LOG) << "Unable to find identifier for position " << position;
    return -1;
}

int FileRenamerWidget::findIdentifier(const CategoryID &category) const
{
    for(int index = 0; index < m_rows.count(); ++index)
        if(m_rows[index].category == category)
            return index;

    qCCritical(JUK_LOG) << "Unable to find match for category " <<
        TagRenamerOptions::tagTypeText(category.category) <<
        ", number " << category.categoryNumber;

    return MAX_CATEGORIES;
}

void FileRenamerWidget::showCategoryOption(int id)
{
    TagOptionsDialog *dialog = new TagOptionsDialog(this, m_rows[id].options, m_rows[id].category.categoryNumber);

    if(dialog->exec() == QDialog::Accepted) {
        m_rows[id].options = dialog->options();
        exampleTextChanged();
    }

    delete dialog;
}

void FileRenamerWidget::moveItemUp(int id)
{
    moveItem(id, MoveUp);
}

void FileRenamerWidget::moveItemDown(int id)
{
    moveItem(id, MoveDown);
}

void FileRenamerWidget::toggleExampleDialog()
{
    m_exampleDialog->setHidden(!m_exampleDialog->isHidden());
}

void FileRenamerWidget::insertCategory()
{
    TagType category = static_cast<TagType>(m_ui->m_category->currentIndex());
    if(m_ui->m_category->currentIndex() < 0 || category >= NumTypes) {
        qCCritical(JUK_LOG) << "Trying to add unknown category somehow.\n";
        return;
    }

    // We need to enable the down button of the current bottom row since it
    // can now move down.
    int idBottom = idOfPosition(m_rows.count() - 1);
    m_rows[idBottom].downButton->setEnabled(true);

    addFolderSeparatorCheckbox();

    // Identifier of new row.
    int id = addRowCategory(category);

    // Set its down button to be disabled.
    m_rows[id].downButton->setEnabled(false);

    m_mainFrame->layout()->invalidate();
    m_ui->m_mainView->update();

    // Now update according to the code in loadConfig().
    m_rows[id].options = TagRenamerOptions(m_rows[id].category);
    exampleTextChanged();
}

void FileRenamerWidget::exampleDialogShown()
{
    m_ui->m_showExample->setText(i18nc("@action:button", "Hide Renamer Test Dialog"));
}

void FileRenamerWidget::exampleDialogHidden()
{
    m_ui->m_showExample->setText(i18nc("@action:button", "Show Renamer Test Dialog"));
}

void FileRenamerWidget::fileSelected(const QString &file)
{
    m_exampleFromFile = true;
    m_exampleFile = file;
    exampleTextChanged();
}

void FileRenamerWidget::dataSelected()
{
    m_exampleFromFile = false;
    exampleTextChanged();
}

QString FileRenamerWidget::separator() const
{
    return m_ui->m_separator->currentText();
}

QString FileRenamerWidget::musicFolder() const
{
    return m_ui->m_musicFolder->url().path();
}

void FileRenamerWidget::slotRemoveRow(int id)
{
    // Remove the given identified row.
    if(!removeRow(id))
        qCCritical(JUK_LOG) << "Unable to remove row " << id;
}

//
// Implementation of FileRenamer
//

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

    for(PlaylistItem *item : as_const(items)) {
        reader.setPlaylistItem(item);
        QString oldFile = item->file().absFilePath();
        QString extension = item->file().fileInfo().suffix();
        QString newFile = fileName(reader) + '.' + extension;

        if(oldFile != newFile) {
            map[oldFile] = newFile;
            itemMap[oldFile] = item;
        }
    }

    if(itemMap.isEmpty() || ConfirmationDialog(map).exec() != QDialog::Accepted)
        return;

    QApplication::setOverrideCursor(Qt::WaitCursor);
    for(const auto &[oldFile, newFile] : as_const(map).asKeyValueRange()) {
        if(moveFile(oldFile, newFile)) {
            itemMap[oldFile]->setFile(newFile);
            itemMap[oldFile]->refresh();

            setFolderIcon(QUrl::fromLocalFile(newFile), itemMap[oldFile]);
        }
        else
            errorFiles << i18nc("Rename %1 file to %2", "%1 to %2", oldFile, newFile);
    }
    QApplication::restoreOverrideCursor();

    if(!errorFiles.isEmpty())
        KMessageBox::errorList(nullptr, i18n("The following rename operations failed:\n"), errorFiles);
}

bool FileRenamer::moveFile(const QString &src, const QString &dest)
{
    qCDebug(JUK_LOG) << "Moving file " << src << " to " << dest;

    QUrl srcURL = QUrl::fromLocalFile(src);
    QUrl dstURL = QUrl::fromLocalFile(dest);

    if(!srcURL.isValid() || !dstURL.isValid() || srcURL == dstURL)
        return false;

    QUrl dir = dstURL.adjusted(QUrl::RemoveFilename); // resolves to path w/out filename
    if(!QDir().mkpath(dir.path())) {
        qCCritical(JUK_LOG) << "Unable to create directory " << dir.path();
        return false;
    }

    // Move the file.
    KIO::Job *job = KIO::file_move(srcURL, dstURL);
    return job->exec();
}

void FileRenamer::setFolderIcon(const QUrl &dstURL, const PlaylistItem *item)
{
    if(item->file().tag()->album().isEmpty() ||
       !item->file().coverInfo()->hasCover())
    {
        return;
    }

    // Split path, and go through each path element.  If a path element has
    // the album information, set its folder icon.
    const QStringList elements = dstURL.path().split('/', Qt::SkipEmptyParts);
    QString path;

    for(const QString &pathPart : elements) {
        path.append('/' + pathPart);

        qCDebug(JUK_LOG) << "Checking path: " << path;
        if(pathPart.contains(item->file().tag()->album()) &&
           QDir(path).exists() &&
           !QFile::exists(path + "/.directory"))
        {
            // Seems to be a match, let's set the folder icon for the current
            // path.  First we should write out the file.

            QPixmap thumb = item->file().coverInfo()->pixmap(CoverInfo::Thumbnail);
            thumb.save(path + "/.juk-thumbnail.png", "PNG");

            KDesktopFile dirFile(path + "/.directory");
            KConfigGroup desktopGroup(dirFile.desktopGroup());

            if(!desktopGroup.hasKey("Icon")) {
                desktopGroup.writePathEntry("Icon", QString("%1/.juk-thumbnail.png").arg(path));
                dirFile.sync();
            }

            return;
        }
    }
}

/**
 * Returns iterator pointing to the last item enabled in the given list with
 * a non-empty value (or is required to be included).
 */
QList<CategoryID>::ConstIterator lastEnabledItem(const QList<CategoryID> &list,
                                                 const CategoryReaderInterface &interface)
{
    auto last = list.end();

    for(auto it = list.begin(); it != list.end(); ++it) {
        if(interface.isRequired(*it) || (!interface.isDisabled(*it) &&
              !interface.categoryValue((*it).category).isEmpty()))
        {
            last = it;
        }
    }

    return last;
}

QString FileRenamer::fileName(const CategoryReaderInterface &interface)
{
    const QList<CategoryID> categoryOrder = interface.categoryOrder();
    const QString separator = interface.separator();
    const QString folder = interface.musicFolder();
    QList<CategoryID>::ConstIterator lastEnabled;
    int i = 0;
    QStringList list;
    QChar dirSeparator (QDir::separator());

    // Use lastEnabled to properly handle folder separators.
    lastEnabled = lastEnabledItem(categoryOrder, interface);
    bool pastLast = false; // Toggles to true once we've passed lastEnabled.

    for(auto it = categoryOrder.begin(); it != categoryOrder.end(); ++it, ++i)
    {
        if(it == lastEnabled)
            pastLast = true;

        if(interface.isDisabled(*it))
            continue;

        QString value = interface.value(*it);

        // The user can use the folder separator checkbox to add folders, so don't allow
        // slashes that slip in to accidentally create new folders.  Should we filter this
        // back out when showing it in the GUI?
        value.replace('/', "%2f");

        if(!pastLast && interface.hasFolderSeparator(i))
            value.append(dirSeparator);

        if(interface.isRequired(*it) || !value.isEmpty())
            list.append(value);
    }

    // Construct a single string representation, handling strings ending in
    // '/' specially

    QString result;

    for(auto it = list.cbegin(); it != list.cend(); /* Empty */) {
        result += *it;

        ++it; // Manually advance iterator to check for end-of-list.

        // Add separator unless at a directory boundary
        if(it != list.cend() &&
           !(*it).startsWith(dirSeparator) && // Check beginning of next item.
           !result.endsWith(dirSeparator))
        {
            result += separator;
        }
    }

    return QString(folder + dirSeparator + result);
}

// vim: set et sw=4 tw=0 sta:
