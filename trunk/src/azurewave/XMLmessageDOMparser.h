/*
 * XMLmessageDOMparser.h
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2010-07-26
 */

#ifndef XMLMESSAGEDOMPARSER_H_
#define XMLMESSAGEDOMPARSER_H_

#include <QObject>
#include <QByteArray>
#include <QDomDocument>

class HubDevice;
class DiscoveryMessageContent;
class USBTechDevice;
class QStringList;
class Logger;

class XMLmessageDOMparser : public QObject {
	Q_OBJECT
public:
	virtual ~XMLmessageDOMparser();
	static DiscoveryMessageContent* parseDiscoveryMessage( const QByteArray & message );
	static void parseServerInfoMessage( const QByteArray & message, HubDevice * refHubDevice );
	static QStringList parseStatusChangedMessage( const QByteArray & message, HubDevice * refHubDevice );
	static USBTechDevice & parseImportResponseMessage( const QByteArray & message, HubDevice * refHubDevice );

private:
	XMLmessageDOMparser( const QByteArray & message );

	DiscoveryMessageContent* processDiscoveryMessage();
	bool processServerInfoMessage( HubDevice * refHubDevice );
	void processServerInfoDeviceSection( HubDevice * refHubDevice, const QDomNode & deviceNode );
	void processDeviceInterfaceElement( USBTechDevice * refUSBDevice, const QDomNode & deviceNode );
	QStringList processStatusChangedMessage( HubDevice * refHubDevice );
	USBTechDevice & processImportResponseMessage( HubDevice * refHubDevice );

	int unmarshallIntValue( const QString & value, const QString & numFormat = "int", int defaultValue = -1 );
	QString unmarshallBCDversionValue( const QString & value, const QString & defaultValue );
	int interpretPlugStatus( const QString & value );

	Logger * logger;

	QDomDocument domDocument;
	void *resultData;
	QString errorString;
	bool success;
	QDomElement root;
};

#endif /* XMLMESSAGEDOMPARSER_H_ */
