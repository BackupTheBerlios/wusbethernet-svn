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
#include <QMessageBox>
#include <QTimer>
#include "QMenu"

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
		}
		cc->start();

/*
		WusbStack *stack = new WusbStack( this, );
		bool result = stack->openConnection();
		LogWriter::info(QString("Open Connection result = %1").arg( result? "true": "false" ) );
		if ( result ) {
			::sleep(2);
			stack->sendURB( USButils::createDeviceDescriptorRequest(), WusbStack::CONTROL_TRANSFER, WusbStack::DATADIRECTION_IN,0, 0x12 );
			::sleep(2);
			result = stack->closeConnection();
			LogWriter::info(QString("Close Connection result = %1").arg( result? "true": "false" ) );
		}
*/
/*		USBconnectionWorker * worker = new USBconnectionWorker( this );
		connect( worker, SIGNAL(workIsDone(USBconnectionWorker::WorkDoneExitCode)), this, SLOT(blubb(USBconnectionWorker::WorkDoneExitCode)));
		worker->queryDevice(QHostAddress("127.0.0.1"), 8002 );
*/
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
