#ifndef JUK_APPLET_H
#define JUK_APPLET_H

#include <qvaluelist.h>
#include <qmap.h>

#include "juk_applet_base.h"

class TrackInfo
{
public:
    QString trackname;
    QString artist;
    QString album;
    QString track;
    QString genre;
    QString length;
    QString year;
    QString comment;
    QString filename;

    TrackInfo () {}
    TrackInfo (QString trackname_, QString artist_, QString album_, 
	       QString track_, QString genre_, QString length_, QString year_, 
	       QString comment_, QString filename_)
	: trackname (trackname_),
	  artist (artist_),
	  album (album_),
	  track (track_),
	  genre (genre_),
	  length (length_),
	  year (year_),
	  comment (comment_),
	  filename (filename_)
	{}
};

class JukApplet : public JukAppletBase
{
    Q_OBJECT
public:
    JukApplet (QWidget * parent = 0, const char * name = 0);
    ~JukApplet ();
signals:
    void itemSkipFirst ();
    void itemSkipBack ();
    void itemPlayPause ();
    void itemSkipNext ();
    void itemSkipLast ();    
public slots:
    /** This method is supposedly called by JuK to announce a new playing
	track. */
    void setPlayingItem (int index); 
    /** This method is supposedly called by JuK to announce  change in the
	playlist. This will be a DCOP slot. */
    void invalidateItems( int noOfItems );
protected:
    /** Refresh the information about tracks stored in currentItems. */
    void refreshItems ( int noOfItems );
    // basically, the following are stubs to be connected with DCOP:
    /** This method is supposed to query information about the track
	denominated by index from JuK. */
    bool queryItem (int index, TrackInfo&);
    /** Store a couple of items, preferably with the active item somewhere in
	the middle. */
    QMap<int, TrackInfo> items;
    int playingItem;
};

#endif // JUK_APPLET_H
