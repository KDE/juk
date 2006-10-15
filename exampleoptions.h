/***************************************************************************
    begin                : Thu Oct 28 2004
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

#ifndef EXAMPLEOPTIONS_H
#define EXAMPLEOPTIONS_H

#include <qdialog.h>

#include <QHideEvent>
#include <QShowEvent>
#include <QWidget>
#include "ui_exampleoptionsbase.h"

class ExampleOptions : public QWidget, public Ui::ExampleOptionsBase
{
    Q_OBJECT
public:
    ExampleOptions(QWidget *parent);

protected slots:
    virtual void exampleSelectionChanged();
    virtual void exampleDataChanged();
    virtual void exampleFileChanged();

signals:
    void dataChanged();
    void fileChanged();
};

// We're not using KDialog(Base) because this dialog won't have any push
// buttons to close it.  It's just a little floating dialog.
class ExampleOptionsDialog : public QDialog
{
    Q_OBJECT
public:
    ExampleOptionsDialog(QWidget *parent);

    const ExampleOptions *widget() const { return m_options; }

    protected:
    virtual void hideEvent(QHideEvent *);
    virtual void showEvent(QShowEvent *);

protected slots:
    void fileModeSelected();
    void fileChanged(const KUrl &);
signals:
    void fileChanged(const QString &);
    void dataChanged();
    void signalHidden();
    void signalShown();

private:
    ExampleOptions *m_options;
};

#endif /* EXAMPLEOPTIONS_H */

// vim: set et sw=4 tw=0 sta:
