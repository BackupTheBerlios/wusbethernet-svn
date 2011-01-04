/**
 * Main function
 *
 * @author		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version		$Id$
 * @created		2010-06-16
 */

#include "config.h"
#include "mainframe.h"
#include "ConfigManager.h"
#include "utils/Logger.h"
#include <qapplication.h>
#include <QTranslator>
#include <QTextCodec>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/sysinfo.h>


/** Initializes the logging infrastructure */
void initLogger( ConfigManager & conf ) {
	Logger *l = Logger::getLogger();
	Logger::LogLevel loglevel = Logger::LOGLEVEL_INFO;
	int confLoglevel = conf.getIntValue("main.logging.loglevel", 1 );
	switch( confLoglevel ) {
	case 0:
		loglevel = Logger::LOGLEVEL_ERROR;	break;
	case 1:
		loglevel = Logger::LOGLEVEL_WARN;	break;
	case 2:
		loglevel = Logger::LOGLEVEL_INFO;	break;
	case 3:
		loglevel = Logger::LOGLEVEL_DEBUG;	break;
	case 4:
		loglevel = Logger::LOGLEVEL_TRACE;	break;
	}
	bool enableConsoleLogging = conf.getBoolValue("main.logging.enableConsole", true );
	bool enableFileLogging = conf.getBoolValue("main.logging.enableLogfiles", false );
	bool enableLogfileAppend = conf.getBoolValue("main.logging.enableFileAppend", false );

	l->setLogLevel( loglevel );
	l->addConsoleAppender();
	l->addFileAppender("general.log", enableLogfileAppend );
	Logger::enableFileLogging( enableFileLogging );
	Logger::enableConsoleLogging( enableConsoleLogging );

	l = Logger::getLogger(QString("DISCOVERY"));
	l->setLogLevel( loglevel );
	l->addConsoleAppender();
	l->addFileAppender("Discovery.log", enableLogfileAppend);

	l = Logger::getLogger(QString("XML"));
	l->setLogLevel( loglevel );
	l->addConsoleAppender();
	l->addFileAppender("XML.log", enableLogfileAppend);

	l = Logger::getLogger(QString("USBQUERY"));
	l->setLogLevel( loglevel );
	l->addConsoleAppender();
	l->addFileAppender("USBquery.log", enableLogfileAppend);

	l = Logger::getLogger("USBConn0");
	l->setLogLevel( loglevel );
	l->addConsoleAppender();
	l->addFileAppender("USB_0.log", enableLogfileAppend);

	l = Logger::getLogger("USBConn1");
	l->setLogLevel( loglevel );
	l->addConsoleAppender();
	l->addFileAppender("USB_1.log", enableLogfileAppend);

	l = Logger::getLogger("USBConn2");
	l->setLogLevel( loglevel );
	l->addConsoleAppender();
	l->addFileAppender("USB_2.log", enableLogfileAppend);

	l = Logger::getLogger("USBConn3");
	l->setLogLevel( loglevel );
	l->addConsoleAppender();
	l->addFileAppender("USB_3.log", enableLogfileAppend);

	l = Logger::getLogger("USBConn4");
	l->setLogLevel( loglevel );
	l->addConsoleAppender();
	l->addFileAppender("USB_4.log", enableLogfileAppend);

	l = Logger::getLogger("USBConn5");
	l->setLogLevel( loglevel );
	l->addConsoleAppender();
	l->addFileAppender("USB_5.log", enableLogfileAppend);

	l = Logger::getLogger("USBConn6");
	l->setLogLevel( loglevel );
	l->addConsoleAppender();
	l->addFileAppender("USB_6.log", enableLogfileAppend);

	l = Logger::getLogger("HUB0");
	l->setLogLevel( loglevel );
	l->addConsoleAppender();
	l->addFileAppender("HUB_0.log", enableLogfileAppend);

	l = Logger::getLogger("HUB1");
	l->setLogLevel( loglevel );
	l->addConsoleAppender();
	l->addFileAppender("HUB_1.log", enableLogfileAppend);

	l = Logger::getLogger("HUB2");
	l->setLogLevel( loglevel );
	l->addConsoleAppender();
	l->addFileAppender("HUB_2.log", enableLogfileAppend);

	l = Logger::getLogger("HUB3");
	l->setLogLevel( loglevel );
	l->addConsoleAppender();
	l->addFileAppender("HUB_3.log", enableLogfileAppend);

	l = Logger::getLogger("HUB4");
	l->setLogLevel( loglevel );
	l->addConsoleAppender();
	l->addFileAppender("HUB_4.log", enableLogfileAppend);

	l = Logger::getLogger("HUB5");
	l->setLogLevel( loglevel );
	l->addConsoleAppender();
	l->addFileAppender("HUB_5.log", enableLogfileAppend);

}

/**
 * Init/Set several configuration values.
 */
void initConfiguration( ConfigManager & conf ) {
	struct sysinfo si;
	char localHostname[65];

	conf.setStringValue("main.name", PROGNAME, false);
	conf.setStringValue("main.version", PROGVERSION, false);
	conf.setStringValue("main.build", QString(__DATE__ " " __TIME__), false );

	// uptime of system
	if( sysinfo(&si) == 0 )
		conf.setStringValue("main.startTimestamp", QString::number( si.uptime ), false );
	else
		conf.setStringValue("main.startTimestamp", "1", false );

	// load hostname
	localHostname[64] = '\0'; // just to be sure
	::gethostname( localHostname, 64 );
	//	char * hostnameEnv = getenv("HOSTNAME");
	//	printf("Hostname = %s\n", localHostname );
	if ( localHostname[0] != '\0' && ::strnlen( localHostname, 64 ) > 0 )
		conf.setStringValue( "hostname", QString( localHostname ).simplified(), false );
	if ( !conf.haveKey( "hostname" ) ||
			conf.getStringValue("hostname").isNull() || conf.getStringValue("hostname").isEmpty() )
		conf.setStringValue("hostname","localhost", false);

	// username (login)
	char * usernameEnv = ::getenv("USER");
	if ( usernameEnv && ::strnlen(usernameEnv, 64) > 0 )
		conf.setStringValue( "username", QString( usernameEnv ).simplified(), false );
	if ( !conf.haveKey( "username" ) ||
			conf.getStringValue("username").isNull() || conf.getStringValue("username").isEmpty() )
		conf.setStringValue("username","nobody", false);

}

int main( int argc, char *argv[] ) {
	// init random number generator
//	::srand( time(0) );

	QApplication app(argc, argv);


	// Load singular instance of configuration manager
	//  - to ensure access to stored configuration values we need
	//    to set application name and organization name first...
	QCoreApplication::setApplicationName(PROGNAME);
	QCoreApplication::setOrganizationName(PROGNAME);
	QCoreApplication::setOrganizationDomain(PROGORGADOMAIN);

	ConfigManager & conf = ConfigManager::getInstance();
	initLogger( conf );
	initConfiguration( conf );

	// Set text codec of translations to UTF-8
	QTextCodec::setCodecForTr(QTextCodec::codecForName("utf8"));

	const QString & localeConf = conf.getStringValue("main.language");
	QString locale;
	if ( localeConf.isNull() || localeConf.isEmpty() || localeConf.compare("auto", Qt::CaseInsensitive ) == 0 ) {
		// if there is no configuration or configuration value is 'auto': lookup system locale
		locale = QLocale::system().name();	// this gives a string in form: "language_country" -> we need only "language"!
		// Note: Qt is missing a useful method to get the language code without country code! (probably I didn't see...)
		int underscoreIdx = locale.indexOf('_');
		if ( underscoreIdx > 0 )
			locale = locale.left( underscoreIdx );
	} else
		locale = localeConf;
    QTranslator translator;
    if ( translator.load( QString("USBhubConnect_") + locale, QString("i18n") ) )
    	app.installTranslator(&translator);
    else
    	printf("Cannot load translation files for locale: %s\n", locale.toUtf8().data() );

    // Load main window - and core application
	mainFrame *mf = new mainFrame();
	mf->show();

    // quit window event
    QObject::connect(qApp, SIGNAL(lastWindowClosed()), qApp, SLOT(quit()));

    // Start event processing and wait
    int res = app.exec();
    Logger::closeAllLogger();
    return res;
}
