#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>

#include "juk_applet.h"

int main(int argc, char *argv[])
{
    KAboutData aboutData("juk-applet", I18N_NOOP("JuK Applet"),
                         "0.1", "JuK Kicker Applet - Draft", KAboutData::License_GPL,
                         "(c) 2003, Mirko Boehm", 0, "http://www.hackerbuero.org");
    KCmdLineArgs::init(argc, argv, &aboutData);
    KApplication a(argc, argv);

    JukApplet applet;

    a.setMainWidget (&applet); applet.show();

    return a.exec ();
}
