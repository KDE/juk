#ifndef PLAYLISTITEM_H
#define PLAYLISTITEM_H

#include <qlistbox.h>

class PlayListItem : public QListBoxItem
{
public:
    PlayListItem (QListBox *parent, QString trackname, QString artist, QString album, 
		  QString track, QString genre, QString length, QString year, 
		  QString comment, QString filename);
    virtual ~PlayListItem() {}
    int height (const QListBox * ) const ;
    int width (const QListBox *) const ;
protected:
    void paint (QPainter * p);
private:
    QString trackname, artist, album, track, genre, year, length, comment, filename;
    int m_height;
};

#endif // PLAYLISTITEM_H

