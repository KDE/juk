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

#include <kdialogbase.h>
#include <kglobal.h>
#include <kstdguiitem.h>
#include <klocale.h>
#include <kiconloader.h>

#include <qstringlist.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qvbox.h>
#include <qhbox.h>

#include "deletedialog.h"

DeleteDialog::DeleteDialog(QWidget *parent, const char *name)
    : DeleteDialogBase(parent, name)
{
    layout()->setSpacing(KDialog::spacingHint());
    ddWarningIcon->setPixmap(KGlobal::iconLoader()->loadIcon("messagebox_warning",
        KIcon::Desktop, KIcon::SizeLarge));
}

void DeleteDialog::setFiles(const QStringList &files)
{
    ddFileList->clear();
    ddFileList->insertStringList(files);
    ddNumFiles->setText(i18n("<b>1</b> file selected.", "<b>%n</b> files selected.", files.count()));
}

bool DeleteDialog::confirmDeleteList(QWidget *parent, const QStringList &condemnedFiles)
{
    KDialogBase base(KDialogBase::Plain, WStyle_DialogBorder, parent, "delete_dialog",
        true /* modal */, i18n("About to delete selected files"), KDialogBase::Ok |
        KDialogBase::Cancel, KDialogBase::Cancel /* Default */, true /* separator */);

    QWidget *page = base.plainPage();
    QVBoxLayout *layout = new QVBoxLayout(page);
    DeleteDialog *widget = new DeleteDialog(page);
    layout->addWidget(widget);

    widget->setFiles(condemnedFiles);
    base.setMinimumSize(410, 324);
    base.adjustSize();
    
    base.setButtonGuiItem(KDialogBase::Ok, KStdGuiItem::del());

    return base.exec() == QDialog::Accepted;
}

// vim: set et ts=4 sw=4:
