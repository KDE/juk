/**
 * Copyright (C) 2012 Martin Sandsmark <martin.sandsmark@kde.org>
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

#ifndef SCROBBLESETTINGS_H
#define SCROBBLESETTINGS_H

#include <KDialog>

class KLineEdit;
class KPushButton;
class QLabel;

class ScrobbleConfigDlg : public KDialog
{
    Q_OBJECT
public:
    explicit ScrobbleConfigDlg(QWidget* parent = 0, Qt::WindowFlags f = 0);
    
private slots:
    void testLogin();
    void validLogin();
    void invalidLogin();
    void save();
    void valuesChanged();
    
private:
    KLineEdit *m_usernameEdit;
    KLineEdit *m_passwordEdit;
    KPushButton *m_testButton;
    QLabel *m_testFeedbackLabel;
};

#endif//SCROBBLESETTINGS_H