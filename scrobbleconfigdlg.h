/**
 * Copyright (C) 2012 Martin Sandsmark <martin.sandsmark@kde.org>
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

#ifndef JUK_SCROBBLESETTINGS_H
#define JUK_SCROBBLESETTINGS_H

#include <QDialog>
#include <KWallet/Wallet>

#include <memory>

using namespace KWallet;

class KLineEdit;
class QAbstractButton;
class QPushButton;
class QLabel;

class ScrobbleConfigDlg : public QDialog
{
    Q_OBJECT

public:
    explicit ScrobbleConfigDlg(QWidget* parent = nullptr);

private slots:
    void testLogin();
    void validLogin();
    void invalidLogin();
    void save();
    void valuesChanged();

private:
    KLineEdit *m_usernameEdit;
    KLineEdit *m_passwordEdit;
    QPushButton *m_testButton;
    QAbstractButton *m_saveButton;
    QLabel *m_testFeedbackLabel;

    std::unique_ptr<Wallet> m_wallet;
};

#endif //JUK_SCROBBLESETTINGS_H
