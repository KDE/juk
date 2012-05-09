#ifndef LYRICSWIDGET_H
#define LYRICSWIDGET_H

#include <QTextBrowser>
#include "filehandle.h"

class QNetworkAccessManager;
class QNetworkReply;


class LyricsWidget : public QTextBrowser
{
    Q_OBJECT

public:
    explicit LyricsWidget(QWidget *parent);
    
    virtual ~LyricsWidget();
    
    QSize minimumSize() const { return QSize(100, 0); }
    
public Q_SLOTS:
    void playing(const FileHandle &file);

private Q_SLOTS:
    void receiveListReply(QNetworkReply*);
    void receiveLyricsReply(QNetworkReply*);
    
private:
    QNetworkAccessManager *m_networkAccessManager;
    QString m_title;
};
    

#endif//LYRICSWIDGET_H
