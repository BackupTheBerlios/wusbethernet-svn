#include "preferencesbox.h"
#include "ConfigManager.h"

PreferencesBox::PreferencesBox(QWidget *parent)
    : QDialog(parent)
{
	ui.setupUi(this);
	setAttribute( Qt::WA_DeleteOnClose, true );	// deletes this widget on close
	setValues();
}

PreferencesBox::~PreferencesBox()
{

}

void PreferencesBox::okButtonSlot() {
	saveValues();
	// call "normal" OK button slot
	accept();
}

void PreferencesBox::setValues() {
	ConfigManager & conf = ConfigManager::getInstance();

	// General settings
	if ( conf.haveKey("main.language") ) {
		const QString & localeConf = conf.getStringValue("main.language", "auto");
		// TODO change this - use ambiguation values to find correct index!
		if ( localeConf.compare("en", Qt::CaseInsensitive ) == 0 )
			ui.comboBox->setCurrentIndex(1);
		else if ( localeConf.compare("de", Qt::CaseInsensitive ) == 0 )
			ui.comboBox->setCurrentIndex(2);
		else
			ui.comboBox->setCurrentIndex(0);
	} else
		ui.comboBox->setCurrentIndex(0);

	if ( conf.haveKey("main.startupDiscovery") )
		ui.checkBox->setChecked( conf.getBoolValue( "main.startupDiscovery", false ) );
	else
		ui.checkBox->setChecked( false );

	// Azurewave / Medion settings
	if ( conf.haveKey("azurewave.enabled") )
		ui.checkBox_2->setChecked( conf.getBoolValue( "azurewave.enabled", true ) );
	else
		ui.checkBox_2->setChecked( true );

	if ( conf.haveKey("azurewave.discovery.type") ) {
		int discoveryTypeConf = conf.getIntValue( "azurewave.discovery.type", 1 );
		switch ( discoveryTypeConf ) {
		case 0:
			// global broadcast
			ui.comboBox_2->setCurrentIndex(1);
			break;
		case 1:
			// local broadcast
			ui.comboBox_2->setCurrentIndex(0);
			break;
		case 2:
			// selected IP addresses
			ui.comboBox_2->setCurrentIndex(2);
			break;
		default:
			ui.comboBox_2->setCurrentIndex(0);
		}
	} else
		ui.comboBox_2->setCurrentIndex(0);

	if ( conf.haveKey("azurewave.discovery.addresses") )
		ui.lineEdit->setText( conf.getStringValue("azurewave.discovery.addresses") );
	else
		ui.lineEdit->clear();

	// TODO get default portnumbers from global definitions
	if ( conf.haveKey("azurewave.discovery.portnumber") )
		ui.lineEdit_3->setText( conf.getStringValue("azurewave.discovery.portnumber","16708"));
	else
		ui.lineEdit_3->setText( "16708" );
	if ( conf.haveKey("azurewave.devctrl.portnumber") )
		ui.lineEdit_4->setText( conf.getStringValue("azurewave.devctrl.portnumber", "21827") );
	else
		ui.lineEdit_4->setText( "21827" );

	ui.checkBox_6->setChecked( conf.getBoolValue( "azurewave.devctrl.addUnimportUsername", false ) );


	// Debug / Logging settings
	if ( conf.haveKey("main.logging.enableConsole" ) )
		ui.checkBox_3->setChecked( conf.getBoolValue("main.logging.enableConsole", true ) );
	else
		ui.checkBox_3->setChecked( false );
	if ( conf.haveKey("main.logging.enableLogfiles" ) )
		ui.checkBox_4->setChecked( conf.getBoolValue("main.logging.enableLogfiles", false ) );
	else
		ui.checkBox_4->setChecked( false );
	if ( conf.haveKey("main.logging.enableFileAppend" ) )
		ui.checkBox_5->setChecked( conf.getBoolValue("main.logging.enableFileAppend", false ) );
	else
		ui.checkBox_5->setChecked( false );
	if ( conf.haveKey("main.logging.directory") )
		ui.lineEdit_2->setText(conf.getStringValue("main.logging.directory", "/tmp/USBhubConnect/"));
	else
		ui.lineEdit_2->setText(conf.getStringValue("main.logging.directory", "/tmp/USBhubConnect/"));

	if ( conf.haveKey("main.logging.loglevel") ) {
		int confLoglevel = conf.getIntValue("main.logging.loglevel", 1 );
		switch ( confLoglevel ) {
		case 0:
			// Error
			ui.comboBox_3->setCurrentIndex(0); break;
		case 1:
			// WARN
			ui.comboBox_3->setCurrentIndex(1); break;
		case 2:
			// INFO
			ui.comboBox_3->setCurrentIndex(2); break;
		case 3:
			// DEBUG
			ui.comboBox_3->setCurrentIndex(3); break;
		case 4:
			// TRACE
			ui.comboBox_3->setCurrentIndex(4); break;
		}
	}
}

void PreferencesBox::saveValues() {
	ConfigManager & conf = ConfigManager::getInstance();

	conf.setBoolValue( "main.startupDiscovery", ui.checkBox->isChecked(), true );
	int cboxIndex = ui.comboBox->currentIndex();
	switch ( cboxIndex ) {
	case 1:
		// en
		conf.setStringValue( "main.language", "en", true );
		break;
	case 2:
		// de
		conf.setStringValue( "main.language", "de", true );
		break;
	case 0:
	default:
		// auto is default
		conf.setStringValue( "main.language", "auto", true );
		break;
	}
	conf.setBoolValue( "azurewave.enabled", ui.checkBox_2->isChecked(), true );
	// azurewave.discovery.addresses  azurewave.discovery.portnumber   azurewave.devctrl.portnumber
	int intValue = ui.lineEdit_3->text().toInt();
	if ( intValue > 512 )
		conf.setIntValue( "azurewave.discovery.portnumber", intValue, true );
	intValue = ui.lineEdit_4->text().toInt();
	if ( intValue > 512 )
		conf.setIntValue( "azurewave.devctrl.portnumber", intValue, true );
	conf.setStringValue( "azurewave.discovery.addresses", ui.lineEdit->text(), true );

	cboxIndex = ui.comboBox_2->currentIndex();
	switch ( cboxIndex ) {
	default:
	case 0:
		conf.setIntValue( "azurewave.discovery.type", 1 );
		break;
	case 1:
		conf.setIntValue( "azurewave.discovery.type", 0 );
		break;
	case 2:
		conf.setIntValue( "azurewave.discovery.type", 2 );
		break;
	}
	conf.setBoolValue( "azurewave.devctrl.addUnimportUsername", ui.checkBox_6->isChecked(), true );


	// logging / debug
	conf.setBoolValue("main.logging.enableConsole", ui.checkBox_3->isChecked(), true );
	conf.setBoolValue("main.logging.enableLogfiles", ui.checkBox_4->isChecked(), true );
	conf.setBoolValue("main.logging.enableFileAppend", ui.checkBox_5->isChecked(), true );
	conf.setStringValue( "main.logging.directory", ui.lineEdit_2->text(), true );

	intValue = ui.comboBox_3->currentIndex();
	if ( intValue >= 0 && intValue < 5 )
		conf.setIntValue( "main.logging.loglevel", intValue );
}

