#ifndef JUKIFACE_H
#define JUKIFACE_H

#include <dcopobject.h>

class JuKIface : virtual public DCOPObject
{
   K_DCOP

k_dcop:
   virtual void openFile( const QString &s )=0;

   virtual void play()=0;
   virtual void pause()=0;
   virtual void stop()=0;
   virtual void playPause()=0;

   virtual void back()=0;
   virtual void forward()=0;
   virtual void seekBack()=0;
   virtual void seekForward()=0;

   virtual void volumeUp()=0;
   virtual void volumeDown()=0;
   virtual void volumeMute()=0;
   virtual void setVolume(float volume)=0;

   virtual void startPlayingPlaylist()=0;
};
#endif
