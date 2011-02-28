/*
 * XMLmessageDOMparser.cpp
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2010-07-26
 */

#include "XMLmessageDOMparser.h"
#include "HubDevice.h"
#include "../ConfigManager.h"
#include "../utils/Logger.h"
#include "../BasicUtils.h"
#include <QStringList>

XMLmessageDOMparser::XMLmessageDOMparser( const QByteArray & message ) {
	errorString = QString::null;
	success = true;

	logger = Logger::getLogger("XML");

	// parsing the XML document and producing DOM tree
	QString errorStr;
	int errorLine;
	int errorColumn;

	if (!domDocument.setContent( message, true, &errorStr, &errorLine, &errorColumn) ) {
		errorString = QString("Parse error at line %1, column %2: %3")
				.arg(errorLine)
				.arg(errorColumn)
				.arg(errorStr);
		success = false;
		return;
	}

	root = domDocument.documentElement();
}

XMLmessageDOMparser::~XMLmessageDOMparser() {
}



ControlMsg_DiscoveryResponse* XMLmessageDOMparser::parseDiscoveryMessage( const QByteArray & message ) {
	/*
	 <discoverResponse>
		<name>MedionHUB</name>
		<hwID type="hex">000000000000</hwID>
		<pid type="int">4000</pid>
		<vid type="int">2</vid>
	</discoverResponse>

	NOTE: since firmware version 1.17 the field "hwID" is filled with MAC address of device
	*/

	XMLmessageDOMparser parser( message );
	// Sanity check
	if ( !parser.success ) {
		return NULL;
	}
	return parser.processDiscoveryMessage();
}

ControlMsg_DiscoveryResponse* XMLmessageDOMparser::processDiscoveryMessage() {
	if ( root.tagName() != "discoverResponse" || !root.hasChildNodes() ) {
		errorString = tr("XML message is not of type \"discoverResponse\"!");
		return NULL;
	}

	ControlMsg_DiscoveryResponse * resultSet = new ControlMsg_DiscoveryResponse();
	QString typeAtt;

	QDomElement domElem = root.firstChildElement("name");
	if ( domElem.isNull() ) {
		errorString = tr("XML message does not contain \"name\" element!");
		return NULL;
	}
	resultSet->name = domElem.text();

	domElem = root.firstChildElement("hwID");
	if ( domElem.isNull() ) {
		errorString = tr("XML message does not contain \"hwID\" element!");
		return NULL;
	}

//	typeAtt = domElem.attribute("type", "hex");
	resultSet->hwid = domElem.text();


	domElem = root.firstChildElement("pid");
	if ( domElem.isNull() ) {
		errorString = tr("XML message does not contain \"pid\" element!");
		return NULL;
	}
	typeAtt = domElem.attribute("type", "int");
	resultSet->pid = unmarshallIntValue( domElem.text(), typeAtt, 0 );


	domElem = root.firstChildElement("vid");
	if ( domElem.isNull() ) {
		errorString = tr("XML message does not contain \"vid\" element!");
		return NULL;
	}
	typeAtt = domElem.attribute("type", "int");
	resultSet->vid = unmarshallIntValue( domElem.text(), typeAtt, 0 );

	return resultSet;
}

void XMLmessageDOMparser::parseServerInfoMessage( const QByteArray & message, HubDevice * refHubDevice ) {
	/*
	<getServerInfoResponse>
		<protocol>WUSB 1.0</protocol>
		<manufacturer>MEDION</manufacturer>
		<modelName>MD-86097</modelName>
		<deviceName>MedionHUB</deviceName>
		<version>1.0.13</version>
		<date>Thu Feb 25 18:18:44 CST 2010</date>
		<usbDeviceList>
			<device>
				<port type="int">5</port>
				<deviceID type="hex">DDCCBBAA</deviceID>
				<status>pluged</status>
				<bcdUSB type="hex">0110</bcdUSB>
				<bClass type="hex">00</bClass>
				<bSubClass type="hex">00</bSubClass>
				<bProtocol type="hex">00</bProtocol>
				<interface>
					<bInterfaceNumber type="int">0</bInterfaceNumber>
					<bClass type="hex">01</bClass>
					<bSubClass type="hex">01</bSubClass>
					<bProtocol type="hex">00</bProtocol>
					<bNumEndpoints type="int">0</bNumEndpoints>
				</interface>
				<idVendor type="hex">13D3</idVendor>
				<idProduct type="hex">3278</idProduct>
				<bcdDevice type="hex">0100</bcdDevice>
				<manufacturer>Manufacturer</manufacturer>
				<product>Remote Audio</product>
			</device>
		</usbDeviceList>
	</getServerInfoResponse>
	*/
	XMLmessageDOMparser parser( message );
	// Sanity check
	if ( !parser.success ) {
		return;
	}
	parser.processServerInfoMessage( refHubDevice );
}


/**
 * Parses all elements of a <em>ServerInfo</em> message and puts processed values into
 * a <tt>HubDevice</tt>.
 * @param	refHubDevice	Reference to <tt>HubDevice</tt>
 * @return	<code>true</code> if parsing was successfull, otherwise <code>false</code>
 */
bool XMLmessageDOMparser::processServerInfoMessage( HubDevice * refHubDevice ) {
	if ( root.tagName() != "getServerInfoResponse" || !root.hasChildNodes() ) {
		errorString = tr("ServerInfo XML message is not of type \"getServerInfoResponse\"!");
		return false;
	}
	// first parsing / extracting all attributes of/for HubDevice
	QString typeAtt;
	QDomElement domElem = root.firstChildElement("protocol");
	if ( domElem.isNull() ) {
		errorString = tr("ServerInfo XML message does not contain \"protocol\" element!");
		return false;
	} else
		refHubDevice->protocol = domElem.text();

	domElem = root.firstChildElement("manufacturer");
	if ( domElem.isNull() ) {
		errorString = tr("ServerInfo XML message does not contain \"manufacturer\" element!");
		return false;
	} else
		refHubDevice->manufacturer = domElem.text();

	domElem = root.firstChildElement("modelName");
	if ( domElem.isNull() ) {
		errorString = tr("ServerInfo XML message does not contain \"modelName\" element!");
		return false;
	} else
		refHubDevice->modelName = domElem.text();

	domElem = root.firstChildElement("deviceName");
	if ( domElem.isNull() ) {
		errorString = tr("ServerInfo XML message does not contain \"deviceName\" element!");
		return false;
	} else
		refHubDevice->deviceName = domElem.text();

	domElem = root.firstChildElement("version");
	if ( domElem.isNull() ) {
		errorString = tr("ServerInfo XML message does not contain \"version\" element!");
		return false;
	} else
		refHubDevice->firmwareVersion = domElem.text();

	domElem = root.firstChildElement("date");
	if ( domElem.isNull() ) {
		errorString = tr("ServerInfo XML message does not contain \"date\" element!");
		return false;
	} else
		refHubDevice->firmwareDate = domElem.text();

	domElem = root.firstChildElement("usbDeviceList");
	if ( !domElem.isNull() && domElem.hasChildNodes() ) {
		QDomNodeList childList = domElem.childNodes();
		for ( int c = 0; c < childList.size(); c++ ) {
			QDomNode childNode = childList.at( c );
			if ( !childNode.isNull() && childNode.nodeName() == "device" && childNode.hasChildNodes() )
				processServerInfoDeviceSection( refHubDevice, childNode );
		}
	}
	return true;
}

void XMLmessageDOMparser::processServerInfoDeviceSection( HubDevice * refHubDevice, const QDomNode & deviceNode ) {
	int port = 0;
	QDomElement domElem = deviceNode.firstChildElement("port");
	if ( domElem.isNull() ) {
		errorString = tr("ServerInfo XML message: device section without \"port\" tag!");
		return;
	}
	QString typeAtt = domElem.attribute("type", "int");
	port = unmarshallIntValue( domElem.text(), typeAtt, 0 );

	if ( port < 0 || port > 9 ) return;	// ??? should not occur???


	USBTechDevice * USBdev = refHubDevice->deviceList[port];
	USBdev->isValid = true;
	USBdev->portNum = port;

	// Special feature since firmware 1.0.21(?): give client software a hint about usage of device
	domElem = deviceNode.firstChildElement("use_isoc");
	if ( !domElem.isNull() ) {
		// this tag is optional
		typeAtt = domElem.attribute("type", "int");
		USBdev->usageHint = unmarshallIntValue( domElem.text(), typeAtt, 0 );
	}

	domElem = deviceNode.firstChildElement("product");
	if ( !domElem.isNull() )
		USBdev->product = domElem.text();
	domElem = deviceNode.firstChildElement("manufacturer");
	if ( !domElem.isNull() )
		USBdev->manufacturer = domElem.text();
	domElem = deviceNode.firstChildElement("deviceID");
	if ( !domElem.isNull() )
		USBdev->deviceID = domElem.text();

	domElem = deviceNode.firstChildElement("status");
	if ( !domElem.isNull() ) {
		QString statusText = domElem.text();
		USBdev->status = (USBTechDevice::PlugStatus) interpretPlugStatus( statusText );
	} else
		USBdev->status = USBTechDevice::PS_NotAvailable;

	// if device is claimed by any host there is an extra tag with hostname / IP
	if ( USBdev->status == USBTechDevice::PS_Claimed ) {
		domElem = deviceNode.firstChildElement("hostName");
		if ( !domElem.isNull() ) {
			USBdev->claimedByName = domElem.text();
		} else
			USBdev->claimedByName = "unknownHost";
		domElem = deviceNode.firstChildElement("hostIP");
		if ( !domElem.isNull() ) {
			USBdev->claimedByIP = flipIPaddress(domElem.text());
		} else
			USBdev->claimedByIP = USBdev->claimedByName;

		QString localhostname = ConfigManager::getInstance().getStringValue("hostname");
		if ( !localhostname.isNull() && localhostname.compare( USBdev->claimedByName, Qt::CaseInsensitive ) == 0 )
			USBdev->owned = true;
		else
			USBdev->owned = false;
	} else if ( USBdev->status == USBTechDevice::PS_Unplugged || USBdev->status == USBTechDevice::PS_NotAvailable )
		USBdev->isValid = false;

	domElem = deviceNode.firstChildElement("bClass");
	if ( !domElem.isNull() ) {
		typeAtt = domElem.attribute("type", "hex");
		USBdev->bClass = unmarshallIntValue( domElem.text(), typeAtt, 0 );
	}
	domElem = deviceNode.firstChildElement("bSubClass");
	if ( !domElem.isNull() ) {
		typeAtt = domElem.attribute("type", "hex");
		USBdev->bSubClass = unmarshallIntValue( domElem.text(), typeAtt, 0 );
	}
	domElem = deviceNode.firstChildElement("bProtocol");
	if ( !domElem.isNull() ) {
		typeAtt = domElem.attribute("type", "hex");
		USBdev->bProtocol = unmarshallIntValue( domElem.text(), typeAtt, 0 );
	}
	domElem = deviceNode.firstChildElement("bProtocol");
	if ( !domElem.isNull() ) {
		typeAtt = domElem.attribute("type", "hex");
		USBdev->bProtocol = unmarshallIntValue( domElem.text(), typeAtt, 0 );
	}

	domElem = deviceNode.firstChildElement("bcdUSB");
	if ( !domElem.isNull() ) {
		USBdev->bcdUSB = domElem.text();
		USBdev->sbcdUSB = unmarshallBCDversionValue( USBdev->bcdUSB, "1.0" );
	}
	domElem = deviceNode.firstChildElement("bcdDevice");
	if ( !domElem.isNull() ) {
		USBdev->bcdDevice = domElem.text();
		USBdev->sbcdDevice = unmarshallBCDversionValue( USBdev->bcdDevice, "0.0" );
	}
	domElem = deviceNode.firstChildElement("idVendor");
	if ( !domElem.isNull() ) {
		typeAtt = domElem.attribute("type", "hex");
		USBdev->idVendor = unmarshallIntValue( domElem.text(), typeAtt, 0 );
	}
	domElem = deviceNode.firstChildElement("idProduct");
	if ( !domElem.isNull() ) {
		typeAtt = domElem.attribute("type", "hex");
		USBdev->idProduct = unmarshallIntValue( domElem.text(), typeAtt, 0 );
	}

	// parse all interface structures of a device
	domElem = deviceNode.firstChildElement("interface");
	while ( !domElem.isNull() && domElem.hasChildNodes() ) {
		processDeviceInterfaceElement( USBdev, domElem );
		domElem = domElem.nextSiblingElement("interface");
	}
}

void XMLmessageDOMparser::processDeviceInterfaceElement( USBTechDevice * refUSBDevice, const QDomNode & deviceNode ) {
	int interfaceNum = 0;

	QDomElement domElem = deviceNode.firstChildElement("bInterfaceNumber");
	if ( domElem.isNull() ) {
		errorString = tr("ServerInfo XML message: interface section without \"interfaceNumber\" tag!");
		return;
	}
	QString typeAtt = domElem.attribute("type", "int");
	interfaceNum = unmarshallIntValue( domElem.text(), typeAtt, interfaceNum );

	if ( interfaceNum < 0 || interfaceNum > 255 ) return;
	// asure that list of interface definitions is large enough
	if ( refUSBDevice->interfaceList.size() <= interfaceNum )
		do {
			refUSBDevice->interfaceList.append( USBTechInterface() );
		} while( refUSBDevice->interfaceList.size() <= interfaceNum );

	refUSBDevice->interfaceList[interfaceNum].isValid = true;
	refUSBDevice->interfaceList[interfaceNum].if_number = interfaceNum;

	domElem = deviceNode.firstChildElement("bClass");
	if ( !domElem.isNull() ) {
		typeAtt = domElem.attribute("type", "hex");
		refUSBDevice->interfaceList[interfaceNum].if_class = unmarshallIntValue( domElem.text(), typeAtt, 0 );
	} else
		refUSBDevice->interfaceList[interfaceNum].if_class = 0;

	domElem = deviceNode.firstChildElement("bSubClass");
	if ( !domElem.isNull() ) {
		typeAtt = domElem.attribute("type", "hex");
		refUSBDevice->interfaceList[interfaceNum].if_subClass = unmarshallIntValue( domElem.text(), typeAtt, 0 );
	} else
		refUSBDevice->interfaceList[interfaceNum].if_subClass = 0;

	domElem = deviceNode.firstChildElement("bProtocol");
	if ( !domElem.isNull() ) {
		typeAtt = domElem.attribute("type", "hex");
		refUSBDevice->interfaceList[interfaceNum].if_protocol = unmarshallIntValue( domElem.text(), typeAtt, 0 );
	} else
		refUSBDevice->interfaceList[interfaceNum].if_protocol = 0;

	domElem = deviceNode.firstChildElement("bNumEndpoints");
	if ( !domElem.isNull() ) {
		typeAtt = domElem.attribute("type", "int");
		refUSBDevice->interfaceList[interfaceNum].if_numEndpoints = unmarshallIntValue( domElem.text(), typeAtt, 0 );
	} else
		refUSBDevice->interfaceList[interfaceNum].if_numEndpoints = 0;

	// XXX endpoints definition?
}


QStringList XMLmessageDOMparser::parseStatusChangedMessage( const QByteArray & message, HubDevice * refHubDevice ) {
	/*
	<devStatusChanged>
		<device>
			<id type="hex">DDCCBBAA</id>
			<status>imported</status>
			<hostName>Notbuch</hostName>
		</device>
	</devStatusChanged>
	*/
	XMLmessageDOMparser parser( message );
	// Sanity check
	if ( !parser.success ) {
		return QStringList();
	}
	return parser.processStatusChangedMessage( refHubDevice );
}

QStringList XMLmessageDOMparser::processStatusChangedMessage( HubDevice * refHubDevice ) {
	QStringList retList = QStringList();
	if ( root.tagName() != "devStatusChanged" || !root.hasChildNodes() ) {
		errorString = tr("StatusChanged XML message is not of type \"devStatusChanged\"!");
		return QStringList();
	}
	// finding the correct device
	QString typeAtt;
	QDomElement domElem;
	// getting all related devices
	domElem = root.firstChildElement("device");
	while ( !domElem.isNull() && domElem.hasChildNodes() ) {
		QDomElement subElem = domElem.firstChildElement("id");
		if ( !subElem.isNull() ) {
			QString sId = subElem.text();
			if ( sId.isNull() || sId.isEmpty() ) continue;
			USBTechDevice & hUSBDev = refHubDevice->findDeviceByID( sId );
			if ( hUSBDev.isValid ) {

				retList.append( hUSBDev.deviceID );	// append deviceID to return list

				subElem = domElem.firstChildElement("status");
				QString statusText = subElem.text();
				hUSBDev.status = (USBTechDevice::PlugStatus) interpretPlugStatus( statusText );
				if ( hUSBDev.status == USBTechDevice::PS_Claimed ) {
					subElem = domElem.firstChildElement("hostName");
					if ( !subElem.isNull() ) {
						hUSBDev.claimedByName = subElem.text();
						QString localhostname = ConfigManager::getInstance().getStringValue("hostname");
						if ( !localhostname.isNull() && localhostname.compare( hUSBDev.claimedByName, Qt::CaseInsensitive ) == 0 )
							hUSBDev.owned = true;
						else
							hUSBDev.owned = false;
					}
				} else if ( hUSBDev.status == USBTechDevice::PS_Unplugged || hUSBDev.status == USBTechDevice::PS_NotAvailable ) {
//						logger->info(QString("Setting device 'non valid': %1!").arg(hUSBDev.deviceID));
					hUSBDev.isValid = false;
					hUSBDev.owned = false;
				} else {
					hUSBDev.owned = false;
				}
			}
/*
			QListIterator<USBTechDevice> it( refHubDevice->deviceList );
			int index = -1;
			while ( it.hasNext() ) {
				const USBTechDevice & usbDev = it.next();
				index++;
				if ( usbDev.isValid && !usbDev.deviceID.isNull() && usbDev.deviceID == sId ) {
					// match! -> now fill status of device according to message
					USBTechDevice & hUSBDev = refHubDevice->deviceList[index];
			}
*/
		}
		// go to next device section - if existing
		domElem = domElem.nextSiblingElement("device");
	}
	return retList;
}

USBTechDevice & XMLmessageDOMparser::parseImportResponseMessage( const QByteArray & message, HubDevice * refHubDevice ) {
	/*
		<importResponse>
			<errorCode type="int">0</errorCode>
			<deviceID type="hex">0064BA81</deviceID>
			<port type="int">8006</port>
		</importResponse>
	*/
	XMLmessageDOMparser parser( message );
	// Sanity check
	if ( !parser.success ) {
		return USBTechDevice::invalid();
	}
	return parser.processImportResponseMessage( refHubDevice );
}

USBTechDevice & XMLmessageDOMparser::processImportResponseMessage( HubDevice * refHubDevice ) {
	if ( root.tagName() != "importResponse" || !root.hasChildNodes() ) {
		errorString = tr("ImportResponse XML message is not of type \"importResponse\"!");
		return USBTechDevice::invalid();
	}
	// finding the correct device
	QString typeAtt;
	QDomElement domElem;

	domElem = root.firstChildElement("deviceID");
	if ( domElem.isNull() ) {
		errorString = QString("ImportResponse XML message: no section \"deviceID\" contained!");
		return USBTechDevice::invalid();
	}
	USBTechDevice & device = refHubDevice->findDeviceByID( domElem.text() );
	logger->trace(QString("processImportResponseMessage dev=%1").arg( domElem.text() ) );
	if ( !device.isValid ) {
		errorString = QString("Cannot find USB device with ID=%1 in list of devices - not importing device!").arg( domElem.text() );
		return USBTechDevice::invalid();
	}

	domElem = root.firstChildElement("errorCode");
	if ( domElem.isNull() ) {
		errorString = tr("ImportResponse XML message: no section \"errorCode\" contained!");
		return USBTechDevice::invalid();
	}
	typeAtt = domElem.attribute("type", "int");
	device.lastOperationErrorCode = unmarshallIntValue( domElem.text(), typeAtt, 0 );

	domElem = root.firstChildElement("port");
	if ( domElem.isNull() ) {
		errorString = tr("ImportResponse XML message: no section \"port\" contained!");
		return USBTechDevice::invalid();
	}
	typeAtt = domElem.attribute("type", "int");
	device.connectionPortNum = unmarshallIntValue( domElem.text(), typeAtt, 0 );
	return device;
}


ControlMsg_UnimportRequest* XMLmessageDOMparser::parseUnimportMessage( const QByteArray & message ) {
	/*
	 * <unimport>
	 *   <hostName>horst</hostName>
	 *   <deviceID type="hex">00e09a81</deviceID>
	 * </unimport>
	 */
	XMLmessageDOMparser parser( message );
	// Sanity check
	if ( !parser.success ) {
		return NULL;
	}
	return parser.processUnimportRequestMessage();
}

ControlMsg_UnimportRequest* XMLmessageDOMparser::processUnimportRequestMessage() {
	if ( root.tagName() != "unimport" || !root.hasChildNodes() ) {
		errorString = QString("XML message is not of type \"unimport\"!");
		return NULL;
	}

	ControlMsg_UnimportRequest * resultSet = new ControlMsg_UnimportRequest();
	QString typeAtt;

	QDomElement domElem = root.firstChildElement("hostName");
	if ( domElem.isNull() ) {
		errorString = QString("XML message does not contain \"hostName\" element!");
		return NULL;
	}
	resultSet->hostname = domElem.text();

	domElem = root.firstChildElement("deviceID");
	if ( domElem.isNull() ) {
		errorString = QString("XML message does not contain \"deviceID\" element!");
		return NULL;
	}
	resultSet->deviceID = domElem.text();

	domElem = root.firstChildElement("message");
	if ( !domElem.isNull() ) {
		resultSet->message = domElem.text();
	} else
		resultSet->message = QString::null;

	return resultSet;
}

/* **********************  some utility methods ****************** */

int XMLmessageDOMparser::unmarshallIntValue( const QString & value, const QString & numFormat, int defaultValue ) {
	bool forceInt = false;
	if ( numFormat.isNull() || numFormat.isEmpty() )
		forceInt = true;
	bool convOk;
	if ( forceInt || numFormat.startsWith("int", Qt::CaseInsensitive ) ) {
		int retVal = value.toInt( &convOk );
		if ( convOk ) return retVal;
		else return defaultValue;
	} else if ( numFormat.startsWith("hex", Qt::CaseInsensitive ) ) {
		int retVal = value.toInt( &convOk, 16 );
		if ( convOk ) return retVal;
		else return defaultValue;
	}
	return defaultValue;
}

QString XMLmessageDOMparser::unmarshallBCDversionValue( const QString & value, const QString & defaultValue ) {
	QString retValue = QString(defaultValue);
	if ( value.isNull() || value.isEmpty() ) {
		return retValue;
	}
	int major = 0;
	int minor = 0;
	int subMinor = 0;
	bool isOK = true;
	if ( value.length() > 1 )
		major = value.left(2).toInt( &isOK, 16 );
	if ( value.length() > 2 )
		minor = value.mid(2,1).toInt( &isOK, 16 );
	if ( value.length() > 3 )
		subMinor = value.mid(3,1).toInt( &isOK, 16 );
	if ( subMinor > 0 )
		retValue = QString("%1.%2.%3").arg(major).arg(minor).arg(subMinor);
	else
		retValue = QString("%1.%2").arg(major).arg(minor);
	return retValue;
}

int XMLmessageDOMparser::interpretPlugStatus( const QString & statusText ) {
	if ( statusText.isNull() ) return USBTechDevice::PS_NotAvailable;

	// in firmware version 1.0.13 status field is "pluged" -
	// maybe someone corrects this in a future firmware...
	if ( statusText == "pluged" || statusText == "plugged" )
		return USBTechDevice::PS_Plugged;
	else if ( statusText == "unpluged" || statusText == "unplugged" )
		return USBTechDevice::PS_Unplugged;
	else if ( statusText == "imported" )
		return USBTechDevice::PS_Claimed;
	else if ( statusText == "importing" )
		return USBTechDevice::PS_Claimed;	// using "Claimed" also for "importing" state, cause there's no difference (?)...
	else
		return USBTechDevice::PS_NotAvailable;
}

