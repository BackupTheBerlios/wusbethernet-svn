/*
 * ConfigManager.cpp
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2010-08-03
 */

#include "ConfigManager.h"
#include <QSettings>
#include <QStringList>
#include <QStringListIterator>

ConfigManager * ConfigManager::instance = NULL;

ConfigManager::ConfigManager() {
	instance = this;
	logger = Logger::getLogger();
	settings = new QSettings;
	loadStatic();
	debugPrintout();
}

ConfigManager::ConfigManager(const ConfigManager & inst) {
}

ConfigManager::~ConfigManager() {
	instance = NULL;
	if ( settings )
		delete settings;
}

ConfigManager & ConfigManager::getInstance() {
	if ( !instance ) {
		new ConfigManager();
	}
	return *instance;
}


void ConfigManager::loadStatic() {
	// this loads all keys from permanent settings
	// and stores them to hash map
	QStringList keys = settings->allKeys();
	QStringListIterator it(keys);
	while ( it.hasNext() ) {
		const QString & key = it.next();
		QString value = settings->value( key ).toString();
		if ( !value.isNull() && !value.isEmpty() )
			properties[ key ] = value;
	}
}


const QString & ConfigManager::getStringValue( const QString & key, const QString & defaultValue ) {
	if ( properties.contains( key ) )
		return properties[ key ];
	else
		return defaultValue;
}

int ConfigManager::getIntValue( const QString & key, int defaultValue ) {
	if ( properties.contains( key ) ) {
		bool isOK;
		int retVal = properties[ key ].toInt( &isOK );
		if ( isOK ) return retVal;
		else return defaultValue;
	}
	return defaultValue;
}

bool ConfigManager::getBoolValue( const QString & key, bool defaultValue ) {
	if ( properties.contains( key ) ) {
		QString & val = properties[ key ];
		if ( val.isNull() || val.isEmpty() ) return defaultValue;
		if ( val == "0" ) return false;
		if ( val == "false" ) return false;
		if ( val.startsWith('n') ) return false;
		if ( val == "1" ) return true;
		if ( val == "true" ) return true;
		if ( val.startsWith('y') ) return true;
	}
	return defaultValue;
}

bool ConfigManager::haveKey( const QString & key ) {
	return properties.contains( key );
}

void ConfigManager::setStringValue( const QString & key, const QString & value, bool permanentSetting ) {
	if ( value.isNull() )
		properties.remove( key );
	else
		properties[ key ] = value;

	if ( permanentSetting )
		settings->setValue( key, value );
}

void ConfigManager::setBoolValue( const QString & key, bool value, bool permanentSetting ) {
	const QString & v = value? "1" : "0";
	properties[ key ] = v;
	if ( permanentSetting )
		settings->setValue( key, v );
}

void ConfigManager::setIntValue( const QString & key, int value, bool permanentSetting ) {
	const QString & v = QString::number(value);
	properties[ key ] = v;
	if ( permanentSetting )
		settings->setValue( key, v );
}
void ConfigManager::debugPrintout() {
	QHashIterator<QString, QString>  it( properties );
	while ( it.hasNext() ) {
		it.next();
		logger->debug( it.key() + " -> " + it.value() );
	}
}

