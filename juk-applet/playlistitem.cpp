#include <qpainter.h>
#include <qstring.h>
#include <qtextstream.h>
#include <qfontmetrics.h>

#include <kglobalsettings.h>
#include <kdebug.h>

#include "playlistitem.h"

PlayListItem::PlayListItem ( QListBox *parent, QString trackname_, 
			     QString artist_, QString album_, QString track_,
			     QString genre_, QString length_, QString year_, 
			     QString comment_, QString filename_ ) 
    : QListBoxItem (parent), trackname (trackname_),
      artist (artist_), album (album_), track (track_), 
      genre (genre_), year (year_), 
      length (length_), comment (comment_),
      filename (filename_)
{
    QFont std = KGlobalSettings::generalFont();
    std.setPointSize ( (int)  ( 0.75 * std.pointSizeFloat ()) );
    QFontMetrics metrics (std);
    m_height = 2 * metrics.height() + 4;
}

int PlayListItem::height( const QListBox* ) const
{
    return m_height;
}

int PlayListItem::width ( const QListBox *lb) const
{
    return lb->width();
}
    
void PlayListItem::paint ( QPainter * p )
{
    QFont std = KGlobalSettings::generalFont();
    QColor bgColor = Qt::darkBlue;
    QColor separatorColor = Qt::blue;
    QRect br, viewport;
    QString line1;
    QTextStream stream1 (line1, IO_WriteOnly);
    QString line2;
    QTextStream stream2 (line2, IO_WriteOnly);
    
    std.setPointSize ( (int)  ( 0.75 * std.pointSizeFloat ()) );
    QFontMetrics metrics (std);

    stream1 << artist << " - " << track;
    stream2 << year << " - "  << length;
    
    viewport = p->viewport ();
    int width = viewport.width ();
    int height = viewport.height ();
    
    // paint the background:
    p->setBrush (bgColor);
    p->setPen (bgColor);
    p->drawRect (0, 0, width, height );

    p->setBrush (separatorColor);
    p->setPen (separatorColor);
    p->drawRect (0, height - 1, width, height );    
    
    // draw the title
    p->setPen (Qt::white);
    p->setFont (std);
    p->drawText ( 2, 2, width -4 , height -4 , Qt::AlignLeft, line1, -1, &br);
    
    p->drawText ( 2, 2 + metrics.height() , width - 4, height - metrics.height() - 4, Qt::AlignLeft, line2, -1, &br);
}
