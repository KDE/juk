#include <qheader.h>
#include <qlistview.h>

#include <kglobalsettings.h>
#include <klocale.h>

#include "juk_applet.h"
#include "playlistitem.h"


JukApplet::JukApplet (QWidget * parent, const char * name)
    : JukAppletBase (parent, name)
{
    PlayListItem *pli[5];

    // adjust - use a (very ...) small font:
    QFont std = KGlobalSettings::generalFont();
    std.setPointSize ( (int)  ( 0.75 * std.pointSize ()) );
    setFont (std);

    // dummies for testing:
    for (int i = 0; i < 5; ++i)
    {
	pli[i]= new PlayListItem ( m_playList, 
				    i18n ("Track Name"), 
				    i18n ("Artist"), i18n ("Album"), 
				    i18n ("Track"), i18n ("Genre"), 
				    i18n ("5:13"), i18n ("1989"), 
				    i18n("Comment Comment Comment"), 
				    i18n ("filename-filename.ogg") );

    }
}

JukApplet::~JukApplet ()
{
}

void JukApplet::refreshItems ( int noOfItems )
{
    const int ItemRange = 20; // how many items up and down do we query?
    // playingItem is supposed to be up to date!

    // find the interval of items we need, make sure the interval is larger
    // than or equal to zero : 
    int min = QMAX ( 0, playingItem - ItemRange);
    int max = QMIN (playingItem + ItemRange, noOfItems);
    
    items.clear();

    for (int current = min; current < max; ++current)
    {
	TrackInfo ti; 
	if (queryItem (current, ti ) )
	{
	    items.insert (current, ti);
	}
    }
}

void JukApplet::setPlayingItem (int)
{
}

bool JukApplet::queryItem (int /* index */, TrackInfo&)
{
    // query the information from JuK...
    return false;
}

void JukApplet::invalidateItems( int noOfItems )
{
    refreshItems ( noOfItems );
}

#include "juk_applet.moc"
