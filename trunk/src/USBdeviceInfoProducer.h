/*
 * USBdeviceInfoProducer.h
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2010-10-21
 */

#ifndef USBDEVICEINFOPRODUCER_H_
#define USBDEVICEINFOPRODUCER_H_

#include <QByteArray>
#include <QList>
#include <QHash>
#include "azurewave/WusbStack.h"
#include "USButils.h"

class Logger;

class USBdeviceInfoProducer {
public:
	struct RequestMetaData {
		bool isValid;
		int expectedReturnLength;
		int endpoint;
		WusbStack::DataTransferType dataTransfer;
		WusbStack::DataDirectionType dataDirection;
	};
	USBdeviceInfoProducer();
	virtual ~USBdeviceInfoProducer();

	bool hasNextURB();
	QByteArray nextURB();
	USBdeviceInfoProducer::RequestMetaData getMetaData();
	void processAnswerURB( const QByteArray & bytes );

	QString getHTMLReport();
private:
	Logger * logger;
	int state;
	int sizeOfConfigurationSection;
	RequestMetaData metaData;
	QList<int> availableLanguageCodes;
	int preferedLanguage;
	int currentStringIDinQuery;

	USButils::UsbDeviceDescriptor deviceDescriptor;
	USButils::UsbConfigurationDescriptor configDescriptor;
	QList<int> listOfStringIDsToquery;
	QHash<int, QString> stringsByID;

	QByteArray buffer;
	QByteArray tempBuffer;
	QString bcdToString( unsigned short bcdValue );
	const QString endpointAttributesToString( unsigned char bmAttributes );
};

#endif /* USBDEVICEINFOPRODUCER_H_ */
