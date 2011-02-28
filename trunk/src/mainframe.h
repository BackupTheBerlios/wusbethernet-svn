/*
 * mainframe.cpp
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2010-06-21
 */

#ifndef MAINFRAME_H
#define MAINFRAME_H

#include <QtGui/QMainWindow>
#include "ui_mainframe.h"
#include "USBconnectionWorker.h"

class QAction;
class ConnectionController;
class PreferencesBox;
class QTreeWidgetItem;

class mainFrame : public QMainWindow
{
    Q_OBJECT

public:
    mainFrame(QWidget *parent = 0);
    ~mainFrame();

private:
    QAction * editAction;
    QAction * runAction;
    QAction * quitAction;
    QAction * infoAction;
    ConnectionController * cc;
    PreferencesBox * prefBoxDialog;

    Ui::mainFrameClass ui;
    void setUpToolbar();


private slots:
	void editPreferences();
	void showAboutInfo();
	void quitProgram();
	void runDiscovery();
	void contextMenuSlot( const QPoint & );
	void treeItemClicked( QTreeWidgetItem * item, int column );
	void editPreferencesBoxFinished( int result );
	/**
	 * Receive user specific info/warning/question message from hub device
	 */
	void userInfoMessageSlot( const QString & key, const QString & message, int answerBits );
	/**
	 * MrProper sequence for application.
	 * TODO this method should be extracted to an own class or other place
	 */
	void cleanUpAllStuff();

signals:
	/**
	 * Signalize answer from user interaction to preceding message.
	 */
	void userInfoMessageReply( const QString & key, const QString & message, int answerBits );
};

#endif // MAINFRAME_H
