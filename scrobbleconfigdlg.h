/***************************************************************************
    begin                : Tue Feb 23 2012
    copyright            : (C) 2012 by Martin Sandsmark
    email                : martin.sandsmark@kde.org
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

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