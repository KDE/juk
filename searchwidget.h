/***************************************************************************
                          searchwidget.h
                             -------------------
    begin                : Sun Mar 6 2003
    copyright            : (C) 2003 by Scott Wheeler <wheeler@kde.org>
                           (C) 2003 by Richard Lärkäng <nouseforaname@home.se>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef SEARCHWIDGET_H
#define SEARCHWIDGET_H

#include <qwidget.h>

class KLineEdit;
class QCheckBox;

class SearchWidget : public QWidget
{
    Q_OBJECT

public:
    SearchWidget(QWidget *parent, const char *name);
    virtual ~SearchWidget();

public slots:
    void clear();

signals:
    void signalQueryChanged(const QString &query, bool caseSensitive);

private slots:
    void slotQueryChanged();

private:
    KLineEdit *m_lineEdit;
    QCheckBox *m_caseSensitive;
} ;

#endif
