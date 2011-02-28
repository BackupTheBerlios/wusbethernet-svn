/*
 * USBdeviceInfoProducer.cpp
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2010-10.21
 */

#include "USBdeviceInfoProducer.h"
#include "USButils.h"
#include "utils/Logger.h"
#include "USBinfoTables.h"
#include "BasicUtils.h"

USBdeviceInfoProducer::USBdeviceInfoProducer() {
	buffer = QByteArray();
	tempBuffer = QByteArray();
	state = 0;
	sizeOfConfigurationSection = 0;
	currentStringIDinQuery = 0;
	metaData.isValid = false;
	preferedLanguage = 0x0409;	// en_US
	listOfStringIDsToquery = QList<int>();
	stringsByID.clear();
	logger = Logger::getLogger("USBQUERY");
}

USBdeviceInfoProducer::~USBdeviceInfoProducer() {
	buffer.clear();
	tempBuffer.clear();
	listOfStringIDsToquery.clear();
	stringsByID.clear();
}


bool USBdeviceInfoProducer::hasNextURB() {
	if ( state >= 5 )
		return false;
	return true;
}

QByteArray USBdeviceInfoProducer::nextURB() {
	switch ( state ) {
	case 0:
		// first state -> send device-request
		metaData.isValid = true;
		metaData.expectedReturnLength = 0x12;
		metaData.endpoint = 0;
		metaData.dataTransfer = TI_WusbStack::CONTROL_TRANSFER;
		metaData.dataDirection = TI_WusbStack::DATADIRECTION_IN;
		return USButils::createGetDescriptor_Device();
	case 1:
		// 2. state -> send get_configuration (for retrieving the size of config section)
		metaData.isValid = true;
		metaData.expectedReturnLength = 0x09;
		metaData.endpoint = 0;
		metaData.dataTransfer = TI_WusbStack::CONTROL_TRANSFER;
		metaData.dataDirection = TI_WusbStack::DATADIRECTION_IN;
		return USButils::createGetDescriptor_Configuration();
	case 2:
		metaData.isValid = true;
		metaData.expectedReturnLength = sizeOfConfigurationSection;
		metaData.endpoint = 0;
		metaData.dataTransfer = TI_WusbStack::CONTROL_TRANSFER;
		metaData.dataDirection = TI_WusbStack::DATADIRECTION_IN;
		return USButils::createGetDescriptor_Configuration( sizeOfConfigurationSection );
	case 3:
		// get language code list
		metaData.isValid = true;
		metaData.expectedReturnLength = 0xff;
		metaData.endpoint = 0;
		metaData.dataTransfer = TI_WusbStack::CONTROL_TRANSFER;
		metaData.dataDirection = TI_WusbStack::DATADIRECTION_IN;
		return USButils::createGetDescriptor_String( 0x0, 0x0, 0xff );
	case 4:
		// retrieve descriptor string
	{
		QListIterator<int> it(listOfStringIDsToquery);
		QString str = QString();
		while (it.hasNext()) {
			str.append( QString::number(it.next()));
			str.append(", ");
		}

		if ( !listOfStringIDsToquery.isEmpty() ) {
			int stringID = listOfStringIDsToquery.takeFirst();
			currentStringIDinQuery = stringID;
			return USButils::createGetDescriptor_String( stringID, preferedLanguage, 0xff );
		} else {
			// error! List should not be empty!
			logger->error("List of query strings is empty!");
			state = 5;
			return QByteArray();
		}
	}
	}
	return QByteArray();
}

USBdeviceInfoProducer::RequestMetaData USBdeviceInfoProducer::getMetaData() {
	return metaData;
}

void USBdeviceInfoProducer::processAnswerURB( const QByteArray & bytes ) {
	switch( state ) {
	case 0:
		// device descriptor
		if ( !bytes.isNull() && !bytes.isEmpty() ) {
			deviceDescriptor = USButils::decodeDeviceDescriptor( bytes );
			if ( deviceDescriptor.bcdUSB == 0 ) {
				logger->warn("Error decoding device descriptor!");
				state = 99;
			} else {
				deviceDescriptor.sVendor = USBinfoTables::getInstance().getVendorByID( deviceDescriptor.idVendor, QString("(unknown)") );
				if ( deviceDescriptor.iManufactor > 0 )
					listOfStringIDsToquery << deviceDescriptor.iManufactor;
				if ( deviceDescriptor.iProduct > 0 )
					listOfStringIDsToquery << deviceDescriptor.iProduct;
				if ( deviceDescriptor.iSerialNumber > 0 )
					listOfStringIDsToquery << deviceDescriptor.iSerialNumber;
				state = 1;
			}
		}
		break;
	case 1:
		// first configuration section (just one configuration section - to get max. size)
		if ( !bytes.isNull() && !bytes.isEmpty() ) {
			sizeOfConfigurationSection = USButils::getConfigsectionLengthFromURB( bytes );
			logger->trace(QString("Length of configuration section is: %1 bytes").arg( QString::number(sizeOfConfigurationSection) ));
			state = 2;

			configDescriptor = USButils::decodeConfigurationSection( bytes, bytes.length());
			if ( configDescriptor.iConfiguration > 0 )
				listOfStringIDsToquery << configDescriptor.iConfiguration;
		}
		break;
	case 2:
		// complete configuration section (including first configuration section and all endpoint descriptors)
		if ( !bytes.isNull() && !bytes.isEmpty() ) {
			USButils::decodeConfigurationSectionComplete( bytes, configDescriptor );
			buffer.append( bytes );
			state = 3;
		}
		break;
	case 3:
		// language codes
		if ( !bytes.isNull() && !bytes.isEmpty() ) {
			preferedLanguage = USButils::decodeAvailableLanguageCodes( bytes, availableLanguageCodes );

			// debug output of all language codes
			QString langs = QString();
			QList<int>::const_iterator stlIter;
			for( stlIter = availableLanguageCodes.begin(); stlIter != availableLanguageCodes.end(); ++stlIter ) {
				langs.append( "0x" );
				langs.append( QString::number((*stlIter), 16) );
				langs.append( ", " );
			}
			if ( logger->isTraceEnabled() ) {
				logger->trace(QString("Preferred language code = 0x%1").arg( QString::number(preferedLanguage,16) ) );
				logger->trace(QString("Available languages = %1").arg( langs ) );
			}

			if ( !listOfStringIDsToquery.isEmpty() )
				state = 4;
			else
				state = 5;
		}
		break;
	case 4:
		// string descriptor
		if ( currentStringIDinQuery > 0 ) {
			QString s = USButils::decodeStringDescriptor( bytes );
			if ( logger->isInfoEnabled() )
				logger->info(QString("Decode String (index = %1) = %2").arg(
						QString::number(currentStringIDinQuery), s ) );
			if ( !s.isNull() )
				stringsByID[currentStringIDinQuery] = s;
		}
		if ( listOfStringIDsToquery.isEmpty() ) {
			// take all retreived string into structures
			if ( deviceDescriptor.iManufactor > 0 && stringsByID.contains(((int)deviceDescriptor.iManufactor) & 0xff) )
				deviceDescriptor.sManufactor = stringsByID[((int)deviceDescriptor.iManufactor) & 0xff];
			if ( deviceDescriptor.iProduct > 0 && stringsByID.contains(((int)deviceDescriptor.iProduct)&0xff)) {
				deviceDescriptor.sProduct = stringsByID[((int)deviceDescriptor.iProduct) & 0xff];
			}
			if ( deviceDescriptor.iSerialNumber > 0 && stringsByID.contains(((int)deviceDescriptor.iSerialNumber) & 0xff) )
				deviceDescriptor.sSerialNumber = stringsByID[((int)deviceDescriptor.iSerialNumber) & 0xff];
			if ( configDescriptor.iConfiguration > 0 && stringsByID.contains( ((int)configDescriptor.iConfiguration) & 0xff) )
				configDescriptor.sConfig = stringsByID[((int)configDescriptor.iConfiguration) & 0xff];

			state = 5;
		} else {
			QListIterator<int> it(listOfStringIDsToquery);
			QString str = QString();
			while (it.hasNext()) {
				str.append( QString::number(it.next()));
				str.append(", ");
			}
			logger->error(QString("List of StringIDs = %1").arg(str));
		}
		break;
	}
}

QString USBdeviceInfoProducer::getHTMLReport() {
	if ( buffer.isEmpty() ) return QString::null;

	if ( deviceDescriptor.bcdUSB == 0 ) {
		return QString::null;
	}

	QString result;
	result.reserve(10240);
	result.append("<html>\n");
	result.append("<center><h1>Device information</h1></center><br>\n");
	QString langs = QString();
	for ( int i = 0; i < availableLanguageCodes.size(); i++ ) {
		langs.append( QString("(0x%1) %2").arg(
				QString::number(availableLanguageCodes[i],16),
				USBinfoTables::getInstance().getLanguageByID( availableLanguageCodes[i], "English (United States)" )
		) );
		if ( i < availableLanguageCodes.size()-1 )
			langs.append( ", " );
	}
	result.append(QString::fromAscii("<b>Available languages:</b> %1<br>\n").arg(langs) );
	result.append("<b>Device (<tt>0x01</tt>):</b>\n");
	result.append("<table border=\"1\" bordercolor=\"#000000\" width=\"100%\">\n");
	result.append("<tr><th>Variable</th><th>Value</th><th>Description/Long value</th></tr>\n");
	result.append(QString::fromAscii("<tr><th>USB version<br>(<tt>bcdUSB</tt>)</th><td>%1</td></tr>\n").arg(
			bcdToString(deviceDescriptor.bcdUSB ) ) );
	result.append(QString::fromAscii("<tr><th>Device class<br>(<tt>bDeviceClass</tt>)</th><td>%1</td><td>%2</td></tr>\n").arg(
			QString::number( deviceDescriptor.bDeviceClass&0xff,16).rightJustified(2, '0', true),
			USBinfoTables::getInstance().getClassDescriptionByClassID( deviceDescriptor.bDeviceClass&0xff ) ) );
	result.append(QString::fromAscii("<tr><th>Device subclass<br>(<tt>bDeviceSubClass</tt>)</th><td>%1</td></tr>\n").arg(
			QString::number( deviceDescriptor.bDeviceSubClass&0xff,16).rightJustified(2, '0', true) ) );
	result.append(QString::fromAscii("<tr><th>Device protocol<br>(<tt>bDeviceProtocol</tt>)</th><td>%1</td><td>%2</td></tr>\n").arg(
			QString::number( deviceDescriptor.bDeviceProtocol&0xff,16).rightJustified(2, '0', true),
			USBinfoTables::getInstance().getDeviceDescriptionByClassSubclassAndProtocol(deviceDescriptor.bDeviceClass,deviceDescriptor.bDeviceSubClass,deviceDescriptor.bDeviceProtocol)) );
	result.append(QString::fromAscii("<tr><th>Max packet size (bytes)<br>(<tt>bMaxPacketSize</tt>)</th><td>%1</td></tr>\n").arg(
			QString::number( deviceDescriptor.bMaxPacketSize&0xff,10) ) );
	result.append(QString::fromAscii("<tr><th>Vendor ID<br>(<tt>idVendor</tt>)</th><td>%1</td><td>%2</td></tr>\n").arg(
			QString::number( deviceDescriptor.idVendor&0xffff,16).rightJustified(4, '0', true),
			deviceDescriptor.sVendor ));
	result.append(QString::fromAscii("<tr><th>Product ID<br>(<tt>idProduct</tt>)</th><td>%1</td></tr>\n").arg(
			QString::number( deviceDescriptor.idProduct&0xffff,16).rightJustified(4, '0', true) ) );

	result.append(QString::fromAscii("<tr><th>Manufactor<br>(<tt>iManufactor</tt>)</th><td>%1</td><td>%2</td></tr>\n").arg(
			QString::number( deviceDescriptor.iManufactor&0xff,10),
			deviceDescriptor.sManufactor.isNull()? QString("-n/a-") : deviceDescriptor.sManufactor ) );
	result.append(QString::fromAscii("<tr><th>Product<br>(<tt>iProduct</tt>)</th><td>%1</td><td>%2</td></tr>\n").arg(
			QString::number( deviceDescriptor.iProduct&0xff,10),
			deviceDescriptor.sProduct.isNull()? QString("-n/a-") : deviceDescriptor.sProduct ) );
	result.append(QString::fromAscii("<tr><th>Serial num<br>(<tt>iSerialNumber</tt>)</th><td>%1</td><td>%2</td></tr>\n").arg(
			QString::number( deviceDescriptor.iSerialNumber&0xff,10),
			deviceDescriptor.sSerialNumber.isNull()? QString("-n/a-") : deviceDescriptor.sSerialNumber ) );

	result.append(QString::fromAscii("<tr><th>Device version no.<br>(<tt>bcdUSB</tt>)</th><td>%1</td></tr>\n").arg(
			bcdToString(deviceDescriptor.bcdUSB ) ) );
	result.append(QString::fromAscii("<tr><th>Num configurations<br>(<tt>bNumConfigurations</tt>)</th><td>%1</td></tr>\n").arg(
			QString::number( deviceDescriptor.bNumConfigurations ) ) );
	result.append("</table>");


	result.append("<b>Configuration section (<tt>0x02</tt>):</b>\n");
	result.append("<table border=\"1\" style=\"border:1px solid black;\" width=\"100%\" >\n");
	result.append("<tr><th>Variable</th><th>Value</th><th>Description/Long value</th></tr>\n");
	result.append(QString::fromAscii("<tr><th>Num Interfaces<br>(<tt>bNumInterfaces</tt>)</th><td>%1</td></tr>\n").arg(
			QString::number( configDescriptor.bNumInterfaces ) ) );
	result.append(QString::fromAscii("<tr><th>No. config<br>(<tt>bConfigurationValue</tt>)</th><td>%1</td></tr>\n").arg(
			QString::number( configDescriptor.bConfigurationValue ) ) );
	result.append(QString::fromAscii("<tr><th>Configuration Descr.<br>(<tt>iConfiguration</tt>)</th><td>%1</td><td>%2</td></tr>\n").arg(
				QString::number( configDescriptor.iConfiguration ),
				configDescriptor.sConfig.isNull()? QString("-n/a-") : configDescriptor.sConfig ) );
	QString attribMap = QString();
	if ( configDescriptor.bmAttributes & 0x80 )
		attribMap.append("(<tt>Bus powered</tt>)<br>");
	if ( configDescriptor.bmAttributes & 0x40 )
		attribMap.append("<tt>SelfPowered</tt><br>");
	if ( configDescriptor.bmAttributes & 0x20 )
		attribMap.append("<tt>RemoteWakeup</tt>");

	result.append(QString::fromAscii("<tr><th>Attributes<br>(<tt>bmAttributes</tt>)</th><td>%1</td><td>%2</td></tr>\n").arg(
			QString::number( configDescriptor.bmAttributes ), attribMap ) );
	result.append(QString::fromAscii("<tr><th>Max power consumption<br>(<tt>bMaxPower</tt>)</th><td>%1</td><td>%2 mA</td></tr>\n").arg(
			QString::number( configDescriptor.bMaxPower ), QString::number( configDescriptor.bMaxPower * 2 ) ) );
	result.append("</table>");

	for ( int intIdx = 0; intIdx < configDescriptor.listInterface.size(); intIdx++ ) {
		result.append(QString::fromAscii("<b>Interface: %1</b>\n").arg( QString::number(intIdx) ));
		result.append("<table border=\"1\" width=\"100%\">\n");
		result.append(QString::fromAscii("<tr><th>Variable</th><th>Value</th><th>Description/Long value</th></tr>") );
		result.append(QString::fromAscii("<tr><th>Interface no.<br>(<tt>bInterfaceNumber</tt>)</th><td>%1</td></tr>").arg(
				QString::number( configDescriptor.listInterface[intIdx]->bInterfaceNumber ) ) );
		result.append(QString::fromAscii("<tr><th>Alternate setting no.<br>(<tt>bAlternateSetting</tt>)</th><td>%1</td></tr>").arg(
				QString::number( configDescriptor.listInterface[intIdx]->bAlternateSetting ) ) );
		result.append(QString::fromAscii("<tr><th>Num Endpoints<br>(<tt>bNumEndpoints</tt>)</th><td>%1</td></tr>").arg(
				QString::number( configDescriptor.listInterface[intIdx]->bNumEndpoints ) ) );
		result.append(QString::fromAscii("<tr><th>Interface class<br>(<tt>bInterfaceClass</tt>)</th><td>%1</td><td>%2</td></tr>").arg(
				QString::number( configDescriptor.listInterface[intIdx]->bInterfaceClass ),
				USBinfoTables::getInstance().getClassDescriptionByClassID( configDescriptor.listInterface[intIdx]->bInterfaceClass&0xff ) ) );
		result.append(QString::fromAscii("<tr><th>Interface subclass<br>(<tt>bInterfaceSubClass</tt>)</th><td>%1</td></tr>").arg(
				QString::number( configDescriptor.listInterface[intIdx]->bInterfaceSubClass ) ) );
		result.append(QString::fromAscii("<tr><th>Interface protocol<br>(<tt>bInterfaceProtocol</tt>)</th><td>%1</td><td>%2</td></tr>").arg(
				QString::number( configDescriptor.listInterface[intIdx]->bInterfaceProtocol ),
				USBinfoTables::getInstance().getDeviceDescriptionByClassSubclassAndProtocol(
						configDescriptor.listInterface[intIdx]->bInterfaceClass,
						configDescriptor.listInterface[intIdx]->bInterfaceSubClass,
						configDescriptor.listInterface[intIdx]->bInterfaceProtocol) ) );
		result.append(QString::fromAscii("<tr><th>Interface descr.<br>(<tt>iInterface</tt>)</th><td>%1</td><td>%2</td></tr>").arg(
				QString::number( configDescriptor.listInterface[intIdx]->iInterface ),
				configDescriptor.listInterface[intIdx]->sInterface.isNull()? QString("-n/a-") : configDescriptor.listInterface[intIdx]->sInterface ) );
		if ( !configDescriptor.listInterface[intIdx]->listInterfaceSupplementalDescriptors.isEmpty() ) {
			for ( int idxDS = 0; idxDS < configDescriptor.listInterface[intIdx]->listInterfaceSupplementalDescriptors.size(); idxDS++ ) {
				USButils::UsbSupplementalDescriptor *sd = configDescriptor.listInterface[intIdx]->listInterfaceSupplementalDescriptors[idxDS];
				result += QString::fromAscii("<tr><th valign=\"middle\"><i>Descriptor %1<br>Type/Length = 0x%2/0x%3</i></th><td colspan=\"3\"><table border=\"1\" width=\"100%\">\n").arg(
						QString::number( idxDS ),
						QString::number( sd->bType, 16 ),
						QString::number( sd->bLength, 16 ) );

				USButils::UsbAudioInterfaceDescriptor *sdAudioHeader;
				USButils::UsbAudioInputTerminalDescriptor *sdAudioTerminal_Input;
				USButils::UsbAudioOutputTerminalDescriptor *sdAudioTerminal_Output;
				USButils::UsbAudioMixerTerminalDescriptor *sdAudioTerminal_Mixer;
				USButils::UsbAudioFeatureTerminalDescriptor *sdAudioTerminal_Feature;

				if ( (sdAudioHeader = dynamic_cast<USButils::UsbAudioInterfaceDescriptor*>(sd)) ) {
					result += QString::fromAscii("<tr><th colspan=\"3\">USB Audio interface descriptor header</th></tr>\n");
					result += QString::fromAscii("<tr><th>Subtype<br>(<tt>bDescriptorSubtype</tt>)</th><td>0x%1</td></tr>\n").arg(
							QString::number( sdAudioHeader->bDescriptorSubtype, 16 ) );
					result += QString::fromAscii("<tr><th>USB Audio spec version<br>(<tt>bcdADC</tt>)</th><td>%1</td></tr>\n").arg(
							bcdToString( sdAudioHeader->bcdADC ) );
					result += QString::fromAscii("<tr><th>Total length of DS<br>(<tt>wTotalLength</tt>)</th><td>%1</td></tr>\n").arg(
							QString::number( sdAudioHeader->wTotalLength ) );
					result += QString::fromAscii("<tr><th>Number of interfaces<br>(<tt>bInCollection</tt>)</th><td>%1</td></tr>\n").arg(
							QString::number( sdAudioHeader->bInCollection ) );
					if ( !sdAudioHeader->listBaInterfaceNr.isEmpty() ) {
						for ( int idxBaIF = 0; idxBaIF < sdAudioHeader->listBaInterfaceNr.size(); idxBaIF++ ) {
							result += QString::fromAscii("<tr><th>Interface number %1<br>(<tt>baInterfaceNr</tt>)</th><td>%2</td></tr>\n").arg(
									QString::number( idxBaIF ),
									QString::number( sdAudioHeader->listBaInterfaceNr[idxBaIF] ) );
						}
					}
				} else if ( (sdAudioTerminal_Input = dynamic_cast<USButils::UsbAudioInputTerminalDescriptor *>(sd)) ) {
					result += QString::fromAscii("<tr><th colspan=\"3\">USB Audio interface descriptor: Input terminal</th></tr>");
					result += QString::fromAscii("<tr><th>Subtype<br>(<tt>bDescriptorSubtype</tt>)</th><td>0x%1</td></tr>\n").arg(
							QString::number( sdAudioTerminal_Input->bDescriptorSubtype, 16 ) );
					result += QString::fromAscii("<tr><th>Terminal ID<br>(<tt>bTerminalID</tt>)</th><td>0x%1</td></tr>\n").arg(
							QString::number( sdAudioTerminal_Input->bTerminalID,16 ) );
					result += QString::fromAscii("<tr><th>Type of terminal<br>(<tt>wTerminalType</tt>)</th><td>0x%1</td><td>%2</td></tr>\n").arg(
							QString::number( sdAudioTerminal_Input->wTerminalType,16 ),
							USBinfoTables::getInstance().getAudioTerminalTypeDescriptionByID(sdAudioTerminal_Input->wTerminalType) );
					result += QString::fromAscii("<tr><th>Associated OUT terminal<br>(<tt>bAssocTerminal</tt>)</th><td>%1</td></tr>\n").arg(
							QString::number( sdAudioTerminal_Input->bAssocTerminal ) );
					result += QString::fromAscii("<tr><th>Num. of channels<br>(<tt>bNrChannels</tt>)</th><td>%1</td></tr>\n").arg(
							QString::number( sdAudioTerminal_Input->bNrChannels ) );
					result += QString::fromAscii("<tr><th>Channel config<br>(<tt>wChannelConfig</tt>)</th><td>%1</td></tr>\n").arg(
							QString::number( sdAudioTerminal_Input->wChannelConfig ) );
					result += QString::fromAscii("<tr><th>Channel names<br>(<tt>iChannelNames</tt>)</th><td>%1</td></tr>\n").arg(
							QString::number( sdAudioTerminal_Input->iChannelNames ) );
					result += QString::fromAscii("<tr><th>Terminal name<br>(<tt>iTerminal</tt>)</th><td>%1</td></tr>\n").arg(
							QString::number( sdAudioTerminal_Input->iTerminal ) );

				} else if ( (sdAudioTerminal_Output = dynamic_cast<USButils::UsbAudioOutputTerminalDescriptor *>(sd)) ) {
					result += QString::fromAscii("<tr><th colspan=\"3\">USB Audio interface descriptor: Output terminal</th></tr>");
					result += QString::fromAscii("<tr><th>Subtype<br>(<tt>bDescriptorSubtype</tt>)</th><td>0x%1</td></tr>\n").arg(
							QString::number( sdAudioTerminal_Output->bDescriptorSubtype, 16 ) );
					result += QString::fromAscii("<tr><th>Terminal ID<br>(<tt>bTerminalID</tt>)</th><td>0x%1</td></tr>\n").arg(
							QString::number( sdAudioTerminal_Output->bTerminalID,16 ) );
					result += QString::fromAscii("<tr><th>Terminal Type<br>(<tt>wTerminalType</tt>)</th><td>0x%1</td><td>%2</td></tr>\n").arg(
							QString::number( sdAudioTerminal_Output->wTerminalType,16 ),
							USBinfoTables::getInstance().getAudioTerminalTypeDescriptionByID(sdAudioTerminal_Output->wTerminalType));
					result += QString::fromAscii("<tr><th>Associated IN terminal<br>(<tt>bAssocTerminal</tt>)</th><td>%1</td></tr>\n").arg(
							QString::number( sdAudioTerminal_Output->bAssocTerminal ) );
					result += QString::fromAscii("<tr><th>Source ID<br>(<tt>bSourceID</tt>)</th><td>%1</td></tr>\n").arg(
							QString::number( sdAudioTerminal_Output->bSourceID ) );
					result += QString::fromAscii("<tr><th>Terminal name<br>(<tt>iTerminal</tt>)</th><td>%1</td></tr>\n").arg(
							QString::number( sdAudioTerminal_Output->iTerminal ) );

				} else if ( (sdAudioTerminal_Mixer = dynamic_cast<USButils::UsbAudioMixerTerminalDescriptor *>(sd)) ) {
					result += QString::fromAscii("<tr><th colspan=\"3\">USB Audio interface descriptor: Mixer unit</th></tr>");

				} else if ( (sdAudioTerminal_Feature = dynamic_cast<USButils::UsbAudioFeatureTerminalDescriptor *>(sd)) ) {
					result += QString::fromAscii("<tr><th colspan=\"3\">USB Audio interface descriptor: Feature unit</th></tr>");
					result += QString::fromAscii("<tr><th>Subtype<br>(<tt>bDescriptorSubtype</tt>)</th><td>0x%1</td></tr>\n").arg(
							QString::number( sdAudioTerminal_Feature->bDescriptorSubtype, 16 ) );
					result += QString::fromAscii("<tr><th>Unit ID<br>(<tt>bUnitID</tt>)</th><td>0x%1</td></tr>\n").arg(
							QString::number( sdAudioTerminal_Feature->bUnitID,16 ) );
					result += QString::fromAscii("<tr><th>Source ID<br>(<tt>bSourceID</tt>)</th><td>%1</td></tr>\n").arg(
							QString::number( sdAudioTerminal_Feature->bSourceID ) );
					result += QString::fromAscii("<tr><th>Control size<br>(<tt>bControlSize</tt>)</th><td>%1</td></tr>\n").arg(
							QString::number( sdAudioTerminal_Feature->bControlSize ) );
					result += QString::fromAscii("<tr><th>Master control<br>(<tt>bmaControls0</tt>)</th><td>0x%1</td><td>%2</td></tr>\n").arg(
							QString::number( sdAudioTerminal_Feature->bmaControls0, 16 ),
							USBinfoTables::getInstance().audioFeatureMapToString( sdAudioTerminal_Feature->bmaControls0 ) );
					for ( int i = 0; i < sdAudioTerminal_Feature->listBmaControls.size(); i++ ) {
						result += QString::fromAscii("<tr><th>Control %1<br>(<tt>bmaControls(%2)</tt>)</th><td>0x%3</td><td>%4</td></tr>\n").arg(
								QString::number( i ),
								QString::number( i ),
								QString::number( sdAudioTerminal_Feature->listBmaControls[i], 16 ),
								USBinfoTables::getInstance().audioFeatureMapToString( sdAudioTerminal_Feature->listBmaControls[i] ) );
					}
					result += QString::fromAscii("<tr><th>Feature name<br>(<tt>iFeature</tt>)</th><td>%1</td></tr>\n").arg(
							QString::number( sdAudioTerminal_Feature->iFeature ) );
				} else {
					result += QString::fromAscii("<tr><th colspan=\"3\">Other:<br><tt>%1</tt></th></tr>").arg(
							messageToString( sd->data, 4 ) );
				}
				result += "</table></td></tr>\n";
			}
		}
		if ( !configDescriptor.listInterface[intIdx]->listEndpoints.isEmpty() ) {
			for ( int idxEpt = 0; idxEpt < configDescriptor.listInterface[intIdx]->listEndpoints.size(); idxEpt++ ) {
				result += QString::fromAscii("<tr><th valign=\"middle\"><i>Endpoint %1</i></th><td colspan=\"3\"><table border=\"1\" width=\"100%\">\n").arg(
						QString::number( idxEpt ) );
				result += QString::fromAscii("<tr><th>Address<br>(<tt>bEndpointAddress</tt>)</th><td>%1</td></tr>").arg(
						QString::number( configDescriptor.listInterface[intIdx]->listEndpoints[idxEpt]->bEndpointAddress ) );
				result += QString::fromAscii("<tr><th>Max. packetsize<br>(<tt>wMaxPacketSize</tt>)</th><td>%1</td></tr>").arg(
						QString::number( configDescriptor.listInterface[intIdx]->listEndpoints[idxEpt]->wMaxPacketSize ) );
				result += QString::fromAscii("<tr><th>Attributes<br>(<tt>bmAttributes</tt>)</th><td>%1</td><td>%2</td></tr>").arg(
						QString::number( configDescriptor.listInterface[intIdx]->listEndpoints[idxEpt]->bmAttributes ),
						endpointAttributesToString(configDescriptor.listInterface[intIdx]->listEndpoints[idxEpt]->bmAttributes) );
				result += QString::fromAscii("<tr><th>Interval<br>(<tt>bInterval</tt>)</th><td>%1</td></tr>").arg(
						QString::number( configDescriptor.listInterface[intIdx]->listEndpoints[idxEpt]->bInterval ) );
				if ( configDescriptor.listInterface[intIdx]->listEndpoints[idxEpt]->bLength > 7 ) {
					result += QString::fromAscii("<tr><th>Refresh<br>(<tt>bRefresh</tt>)</th><td>%1</td></tr>").arg(
							QString::number( configDescriptor.listInterface[intIdx]->listEndpoints[idxEpt]->bRefresh ) );
					result += QString::fromAscii("<tr><th>SynchAddress<br>(<tt>bSynchAddress</tt>)</th><td>%1</td></tr>\n").arg(
							QString::number( configDescriptor.listInterface[intIdx]->listEndpoints[idxEpt]->bSynchAddress ) );
				}
				result += "</table></td></tr>\n";
			}
		}
		result += "</table>\n";
	}

	result.append("</html>\n");
	return result;
}

QString USBdeviceInfoProducer::bcdToString( unsigned short bcdValue ) {
	QString s = QString("%1.%2").arg( QString::number(((bcdValue>>8)&0xff),16), QString::number(bcdValue&0xff,16) );
	while( s.endsWith('0') )
		s = s.left( s.length() -1 );
	if ( s.endsWith('.') )
		s += "0";
	return s;
}

const QString USBdeviceInfoProducer::endpointAttributesToString( unsigned char bmAttributes ) {
	if ( bmAttributes == 0 ) return QString("none");
	int code = (bmAttributes & 0x03);
	QString retVal = QString();
	bool isIsoMode = false;
	switch ( code ) {
	case 0:
		// 00  control
		retVal.append( "Transfer Type: Control" );
		break;
	case 1:
		// 01  Isochronous
		retVal.append( "Transfer Type: Isochronous<br>" );
		isIsoMode  = true;
		break;
	case 2:
		// 10  Bulk
		retVal.append( "Transfer Type: Bulk" );
		break;
	case 3:
		// 11  Interrupt
		retVal.append( "Transfer Type: Interrupt" );
		break;
	}
	if ( isIsoMode ) {
		code = ((bmAttributes & 0x0c) >> 2);
		switch ( code ) {
		case 0:
			// 00	No syncronization
			retVal.append( "(no synchronization, " );
			break;
		case 1:
			// 01   Asynchronous
			retVal.append( "(Asynchronous sync, " );
			break;
		case 2:
			// 10   Adaptive
			retVal.append( "(Adaptive sync, " );
			break;
		case 3:
			// 11   Synchronous
			retVal.append( "(Synchronous, " );
			break;
		}
		code = ((bmAttributes & 0x30) >> 4);
		switch ( code ) {
		case 0:
			// 00  Data Endpoint
			retVal.append( "Data Endpoint)" );
			break;
		case 1:
			// 01  Feedback Endpoint
			retVal.append( "Feedback Endpoint)" );
			break;
		case 2:
			// 10  Explicit Feedback Data Endpoint
			retVal.append( "Explicit Feedback Data Endpoint)" );
			break;
		case 3:
			// 11  Reserved
			retVal.append( "Reserved(!))" );
			break;
		}
	}
	return retVal;
}
