/*
 * ConfigManager.h
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2010-08-03
 */

#ifndef CONFIGMANAGER_H_
#define CONFIGMANAGER_H_

#include <QHash>
#include <QString>
#include "utils/Logger.h"

class QSettings;

class ConfigManager {
public:
	~ConfigManager();
	/**
	 * Get singleton instance of this class.<br>
	 * Note that this implementation is NOT thread safe! So place
	 * <tt>ConfigManager::getInstance()</tt> in a very early stage of program
	 * to ensure single thread execution.
	 */
	static ConfigManager & getInstance();
	/**
	 * Gets a string constant from configuration.
	 * If no configuration value is stored for <tt>key</tt> then
	 * <tt>defaultValue</tt> is returned.
	 */
	const QString & getStringValue( const QString & key, const QString & defaultValue = QString::null );
	/**
	 * Gets a integer value from configuration.
	 * If no configuration value is stored for <tt>key</tt> or if value is
	 * not interpretable as integer value <tt>defaultValue</tt> is returned.
	 */
	int getIntValue( const QString & key, int defaultValue );
	/**
	 * Gets a boolean value from configuration.
	 * If no configuration value is stored for <tt>key</tt> or if value is
	 * not interpretable as boolean value <tt>defaultValue</tt> is returned.
	 */
	bool getBoolValue( const QString & key, bool defaultValue = false );
	/**
	 * Checks if a specified key is present in configuration.
	 */
	bool haveKey( const QString & key );
	/**
	 * Sets the given <tt>value</tt> for <tt>key</tt>.
	 * If <tt>value</tt> is <code>QString::null</code> then <tt>key</tt>
	 * will be removed from configuration.
	 */
	void setStringValue( const QString & key, const QString & value, bool permanentSetting = true );
private:
	static ConfigManager * instance;
	Logger * logger;
	ConfigManager();
	ConfigManager(const ConfigManager & inst);

	void loadStatic();
	void debugPrintout();

	/** All settings as string values.
	 * -- probably better QVariant?? */
	QHash<QString, QString> properties;

	/** All settings which are permanently stored */
	QSettings * settings;
};

#endif /* CONFIGMANAGER_H_ */
