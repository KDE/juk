/***************************************************************************
    begin                : Tue Aug 31 21:59:58 EST 2004
    copyright            : (C) 2004 by Michael Pyne
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

#include "deletedialog.h"

#include <kdialog.h>
#include <KApplication>
#include <KStandardGuiItem>
#include <klocale.h>
#include <kiconloader.h>
#include <kconfig.h>
#include <kconfiggroup.h>

#include <QStringList>
#include <QCheckBox>
#include <QLayout>
#include <QLabel>

//////////////////////////////////////////////////////////////////////////////
// DeleteWidget implementation
//////////////////////////////////////////////////////////////////////////////

DeleteWidget::DeleteWidget(QWidget *parent, const char *name)
    : QWidget(parent)
{
    setupUi(this);

    setObjectName(name);

    KConfigGroup messageGroup(KGlobal::config(), "FileRemover");

    bool deleteInstead = messageGroup.readEntry("deleteInsteadOfTrash", false);
    slotShouldDelete(deleteInstead);
    ddShouldDelete->setChecked(deleteInstead);
}

void DeleteWidget::setFiles(const QStringList &files)
{
    ddFileList->clear();
    ddFileList->insertStringList(files);
    ddNumFiles->setText(i18np("<b>1</b> file selected.", "<b>%1</b> files selected.", files.count()));
}

void DeleteWidget::slotShouldDelete(bool shouldDelete)
{
    if(shouldDelete) {
        ddDeleteText->setText(i18n("<qt>These items will be <b>permanently "
            "deleted</b> from your hard disk.</qt>"));
        ddWarningIcon->setPixmap(KIconLoader::global()->loadIcon("dialog-warning",
            KIconLoader::Desktop, KIconLoader::SizeLarge));
    }
    else {
        ddDeleteText->setText(i18n("<qt>These items will be moved to the Trash Bin.</qt>"));
        ddWarningIcon->setPixmap(KIconLoader::global()->loadIcon("user-trash-full",
            KIconLoader::Desktop, KIconLoader::SizeLarge));
    }
}

//////////////////////////////////////////////////////////////////////////////
// DeleteDialog implementation
//////////////////////////////////////////////////////////////////////////////

DeleteDialog::DeleteDialog(QWidget *parent, const char *name) :
    KDialog(parent, Qt::WStyle_DialogBorder),
    m_trashGuiItem(i18n("&Send to Trash"), "user-trash-full")
{
    setObjectName(name);
    setModal(true);
    setCaption(i18n("About to delete selected files"));
    setButtons(Ok | Cancel);
    setDefaultButton(Cancel);
    showButtonSeparator(true);

    m_widget = new DeleteWidget(this, "delete_dialog_widget");
    setMainWidget(m_widget);

    m_widget->setMinimumSize(400, 300);
    setMinimumSize(410, 326);
    adjustSize();

    slotShouldDelete(shouldDelete());

    connect(m_widget->ddShouldDelete, SIGNAL(toggled(bool)), SLOT(slotShouldDelete(bool)));
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
    KConfigGroup messageGroup(KGlobal::config(), "FileRemover");

    // Save user's preference

    messageGroup.writeEntry("deleteInsteadOfTrash", shouldDelete());
    messageGroup.sync();

    KDialog::accept();
}

void DeleteDialog::slotShouldDelete(bool shouldDelete)
{
    setButtonGuiItem(Ok, shouldDelete ? KStandardGuiItem::del() : m_trashGuiItem);
}

#include "deletedialog.moc"

// vim: set et sw=4 tw=0 sta:
