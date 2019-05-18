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

#ifndef JUK_EXAMPLEOPTIONS_H
#define JUK_EXAMPLEOPTIONS_H

class QUrl;

#include <QDialog>
#include <QWidget>

#include "ui_exampleoptionsbase.h"

class ExampleOptions : public QWidget, public Ui::ExampleOptionsBase
{
    Q_OBJECT
public:
    explicit ExampleOptions(QWidget *parent);

protected slots:
    void exampleSelectionChanged();
    void exampleDataChanged();
    void exampleFileChanged();

signals:
    void dataChanged();
    void fileChanged();
};

class ExampleOptionsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ExampleOptionsDialog(QWidget *parent);

    const ExampleOptions *widget() const { return m_options; }

protected:
    virtual void hideEvent(QHideEvent *) override;
    virtual void showEvent(QShowEvent *) override;

protected slots:
    void fileModeSelected();
    void urlChanged(const QUrl &);

signals:
    void fileChanged(const QString &);
    void dataChanged();
    void signalHidden();
    void signalShown();

private:
    ExampleOptions *m_options;
};

#endif /* JUK_EXAMPLEOPTIONS_H */

// vim: set et sw=4 tw=0 sta:
