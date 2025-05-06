/**
 * Copyright (C) 2004, 2008 Michael Pyne <mpyne@kde.org>
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

#include "deletedialog.h"
#include "ui_deletedialogbase.h"

#include <KStandardGuiItem>
#include <KLocalizedString>
#include <KIconLoader>
#include <KConfig>
#include <KConfigGroup>
#include <KSharedConfig>

#include <QDialogButtonBox>
#include <QStringList>
#include <QCheckBox>
#include <QVBoxLayout>

#include "iconsupport.h"

//////////////////////////////////////////////////////////////////////////////
// DeleteWidget implementation
//////////////////////////////////////////////////////////////////////////////

using namespace Qt::Literals::StringLiterals;

DeleteWidget::DeleteWidget(QWidget *parent)
  : QWidget(parent)
  , m_ui(new Ui::DeleteDialogBase)
{
    m_ui->setupUi(this);

    setObjectName(QLatin1String("delete_dialog_widget"));

    KConfigGroup messageGroup(KSharedConfig::openConfig(), u"FileRemover"_s);

    bool deleteInstead = messageGroup.readEntry("deleteInsteadOfTrash", false);
    slotShouldDelete(deleteInstead);
    m_ui->ddShouldDelete->setChecked(deleteInstead);

    // Forward on signals
    connect(m_ui->ddShouldDelete, &QCheckBox::toggled, this, &DeleteWidget::slotShouldDelete);
    connect(m_ui->ddButtonBox, &QDialogButtonBox::accepted, this, &DeleteWidget::accepted);
    connect(m_ui->ddButtonBox, &QDialogButtonBox::rejected, this, &DeleteWidget::rejected);

}

void DeleteWidget::setFiles(const QStringList &files)
{
    m_ui->ddFileList->clear();
    m_ui->ddFileList->insertItems(0, files);
    m_ui->ddNumFiles->setText(i18np("<b>1</b> file selected.", "<b>%1</b> files selected.", files.count()));
}

bool DeleteWidget::shouldDelete() const
{
    return m_ui->ddShouldDelete->isChecked();
}

void DeleteWidget::slotShouldDelete(bool shouldDelete)
{
    using namespace IconSupport;

    if(shouldDelete) {
        m_ui->ddDeleteText->setText(i18n("<qt>These items will be <b>permanently "
            "deleted</b> from your hard disk.</qt>"));
        m_ui->ddWarningIcon->setPixmap(
                ("dialog-warning"_icon).pixmap(KIconLoader::SizeLarge));
    }
    else {
        m_ui->ddDeleteText->setText(i18n("<qt>These items will be moved to the Trash Bin.</qt>"));
        m_ui->ddWarningIcon->setPixmap(
                ("user-trash-full"_icon).pixmap(KIconLoader::SizeLarge));
    }
}

//////////////////////////////////////////////////////////////////////////////
// DeleteDialog implementation
//////////////////////////////////////////////////////////////////////////////

DeleteDialog::DeleteDialog(QWidget *parent) :
    QDialog(parent),
    m_trashGuiItem(i18nc("@action:button", "&Send to Trash"), "user-trash-full")
{
    setObjectName(QLatin1String("juk_delete_dialog"));
    setModal(true);
    setWindowTitle(i18nc("@title:window", "Delete Selected Files"));

    auto layout = new QVBoxLayout(this);

    m_widget = new DeleteWidget(this);
    m_widget->setMinimumSize(400, 300);
    layout->addWidget(m_widget);

    // Trying to adjust for Qt bug with rich text where the layout is ignored
    // (something about not being able to get height-for-width on X11?)
    setMinimumSize(410, 326);
    adjustSize();

    slotShouldDelete(shouldDelete());

    connect(m_widget, SIGNAL(accepted()), SLOT(accept()));
    connect(m_widget, SIGNAL(rejected()), SLOT(reject()));
    connect(m_widget, SIGNAL(signalShouldDelete(bool)), SLOT(slotShouldDelete(bool)));
}

bool DeleteDialog::confirmDeleteList(const QStringList &condemnedFiles)
{
    m_widget->setFiles(condemnedFiles);

    return exec() == QDialog::Accepted;
}

void DeleteDialog::setFiles(const QStringList &files)
{
    m_widget->setFiles(files);
}

void DeleteDialog::accept()
{
    KConfigGroup messageGroup(KSharedConfig::openConfig(), u"FileRemover"_s);

    // Save user's preference

    messageGroup.writeEntry("deleteInsteadOfTrash", shouldDelete());
    messageGroup.sync();

    QDialog::accept();
}

void DeleteDialog::slotShouldDelete(bool shouldDelete)
{
    KGuiItem::assign(m_widget->m_ui->ddButtonBox->button(QDialogButtonBox::Ok),
                     shouldDelete ? KStandardGuiItem::del() : m_trashGuiItem);
}

// vim: set et sw=4 tw=0 sta:
