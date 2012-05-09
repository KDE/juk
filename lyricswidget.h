#ifndef LYRICSWIDGET_H
#define LYRICSWIDGET_H

#include <QTextEdit>
#include "filehandle.h"

class QNetworkAccessManager;
class QNetworkReply;


class LyricsWidget : public QTextEdit
{
    Q_OBJECT

public:
    explicit LyricsWidget(QWidget *parent);
    
    virtual ~LyricsWidget();
    
public Q_SLOTS:
    void playing(const FileHandle &file);

private Q_SLOTS:
    void receiveListReply(QNetworkReply*);
    void receiveLyricsReply(QNetworkReply*);
    
private:
    QNetworkAccessManager *m_networkAccessManager;
};
    

#endif//LYRICSWIDGET_H
