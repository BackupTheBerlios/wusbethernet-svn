/*
 * mainframe.cpp
 * This builds up and displays the "main window" of application.
 * If auto startup of device discovery is enabled this will be started. Otherwise
 * start is triggered by button.
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2010-06-21
 */

#include "mainframe.h"
#include "azurewave/ConnectionController.h"
#include "ConfigManager.h"
#include "preferencesbox.h"
#include "AboutBox.h"
#include "config.h"
#include "utils/Logger.h"
#include "vhci/LinuxVHCIconnector.h"
#include <QMessageBox>
#include <QTimer>
#include <QMessageBox>
#include "QMenu"

extern volatile bool applicationShouldRun;

mainFrame::mainFrame(QWidget *parent)
    : QMainWindow(parent)
{
	cc = NULL;
	prefBoxDialog = NULL;

	ui.setupUi(this);
	setUpToolbar();
}

mainFrame::~mainFrame()
{
	if ( prefBoxDialog )
		delete prefBoxDialog;
}

void mainFrame::setUpToolbar() {
	runAction = new QAction( QIcon(":/images/gnome-run.png"), tr("&Run"), this );
	runAction->setStatusTip(tr("Start/Stop network discovery"));
	runAction->setCheckable( true );
//	runAction->setChecked( true );
	connect(runAction, SIGNAL(triggered()), this, SLOT(runDiscovery()));
	ui.toolBar->addAction(runAction);
	if ( ConfigManager::getInstance().getBoolValue("main.startupDiscovery", false ) ) {
		runAction->setChecked( true );
		QTimer::singleShot ( 200, this, SLOT( runDiscovery() ) );
	}


	editAction = new QAction( QIcon(":/images/configure2.png"), tr("&Preferences"), this );
	editAction->setStatusTip(tr("Show/Edit preferences"));
	connect(editAction, SIGNAL(triggered()), this, SLOT(editPreferences()));
	ui.toolBar->addAction(editAction);

	infoAction = new QAction( QIcon(":/images/system-help.png"), tr("&About"), this );
	infoAction->setStatusTip( tr("About this program and help") );
	connect(infoAction, SIGNAL(triggered()), this, SLOT(showAboutInfo()));
	ui.toolBar->addAction(infoAction);

	quitAction = new QAction( QIcon(":/images/gtk-quit.png"), tr("&Quit"), this );
	quitAction->setStatusTip(tr("Exit program"));
	connect(quitAction, SIGNAL(triggered()), this, SLOT(quitProgram()));
	ui.toolBar->addAction(quitAction);
}


void mainFrame::editPreferences() {
	if ( prefBoxDialog ) {
		prefBoxDialog->raise();
	} else {
		prefBoxDialog = new PreferencesBox( this );
		prefBoxDialog->show();
		connect( prefBoxDialog, SIGNAL(finished(int)), this, SLOT(editPreferencesBoxFinished( int ) ) );
	}
}

void mainFrame::showAboutInfo() {
	// About-Button
	// create a new instance of aboutbox and show it
	AboutBox *aboutbox = new AboutBox(this);
	aboutbox->setAttribute( Qt::WA_DeleteOnClose );
	aboutbox->show();
}

void mainFrame::runDiscovery() {
	if ( cc && cc->isRunning() ) {
		cc->stop();
	} else {
		if ( !cc ) {
			cc = new ConnectionController( 1550 );
			cc->setVisualTreeWidget( ui.treeWidget );

			connect(ui.treeWidget, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(contextMenuSlot(const QPoint &)));
			connect(ui.treeWidget, SIGNAL(itemClicked( QTreeWidgetItem*, int )), this, SLOT( treeItemClicked(QTreeWidgetItem*, int)));

			connect(cc, SIGNAL(userInfoMessage(const QString &,const QString &,int)),
					this, SLOT(userInfoMessageSlot(const QString &,const QString &,int) ) );
			connect( this, SIGNAL( userInfoMessageReply(const QString &,const QString &,int)),
					cc, SLOT(relayUserInfoReply(const QString &,const QString &,int)) );

			// init USB-VHCI host interface
			LinuxVHCIconnector * connector = LinuxVHCIconnector::getInstance();
			if ( !connector || !connector->openInterface() ) {
				userInfoMessageSlot( "none",
						tr("<html>Cannot open OS interface (<em>VHCI</em>)!<br>"
								"<b>You will not be able to connect<br>USB devices to system!</b></html>"), -1 );
			} else {
				// start USB interface
				connector->startWork();
			}
		}
		cc->start();
	}
}

void mainFrame::quitProgram() {
	if ( cc && cc->isRunning() )
		cc->stop();
	delete cc;
	this->setVisible( false );
	this->close();
}

void mainFrame::contextMenuSlot( const QPoint & pos ) {
	QTreeWidgetItem * item = NULL;
	item = ui.treeWidget->itemAt( pos ) ;
	if ( item ) {
		QMenu *menu = cc->widgetItemContextMenu( item );
		if ( menu )
			menu->exec( ui.treeWidget->mapToGlobal( pos ) );
	}
}


void mainFrame::treeItemClicked( QTreeWidgetItem * item, int column ) {
	if ( item->parent() ) {
		printf("treeItemClicked Col=%i\n", column );
		// XXX process event in ConnectionController
	}
}


void mainFrame::editPreferencesBoxFinished( int result ) {
	if ( prefBoxDialog ) {
		if ( result == 1 ) {
			// OK button was pressed
			ConfigManager & conf = ConfigManager::getInstance();
			//  set logger:
			Logger::enableConsoleLogging( conf.getBoolValue("main.logging.enableConsole", false ));
			Logger::enableFileLogging( conf.getBoolValue( "main.logging.enableLogfiles", false ));
		}
		disconnect( prefBoxDialog, SIGNAL(finished(int)), this, SLOT(editPreferencesBoxFinished( int ) ) );
		prefBoxDialog = NULL;
	}

}

void mainFrame::userInfoMessageSlot( const QString & key, const QString & message, int answerBits ) {
	if ( answerBits == -1 ) {
		// Warning message
		QMessageBox::warning( this, QString(PROGNAME) + tr(": Warning"), message );
		// no answer needed/reasonable for error and info messages!
	} else if ( answerBits < -1 ) {
		// Error message
		QMessageBox::critical( this, QString(PROGNAME) + tr(": Error"), message );
		// no answer needed/reasonable for error and info messages!
	} else if ( answerBits < 2 ) {
		// Info message
		QMessageBox::information( this, QString(PROGNAME) + tr(": Info"), message );
		// no answer needed/reasonable for error and info messages!
	} else {
		// Question messsage
		// TODO define and handle answerBits better!
		int result = 0;
		switch (answerBits) {
		case 2:
			result = QMessageBox::question( this, QString(PROGNAME) + tr(": Question"), message, QMessageBox::Yes | QMessageBox::No );
		case 3:
			result = QMessageBox::question( this, QString(PROGNAME) + tr(": Question"), message, QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel );
		case 4:
			result = QMessageBox::question( this, QString(PROGNAME) + tr(": Question"), message, QMessageBox::Abort | QMessageBox::Retry );
		}
		emit userInfoMessageReply( key, message, result );
	}
}

void mainFrame::cleanUpAllStuff() {
	applicationShouldRun = false;
	//
	Logger::getLogger()->info("Exiting application...");

	// close usb-vhci interface
	delete ( LinuxVHCIconnector::getInstance() );

    delete &(ConfigManager::getInstance());

    // finalize logger
    Logger::closeAllLogger();
}
