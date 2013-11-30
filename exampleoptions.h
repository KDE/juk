/**
 * Copyright (C) 2004 Michael Pyne <mpyne@kde.org>
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
