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
	void editPreferencesBoxFinished( int result );
};

#endif // MAINFRAME_H
