/*
 * USButils.cpp
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2010-10-14
 */

#include "USButils.h"
#include "utils/Logger.h"
#include <string.h>

USButils::USButils() {
}

QByteArray USButils::createGetDescriptor_Device() {
	return QByteArray( "\x80\x06\x00\x01\x00\x00\x12\x00", 8 );
}

QByteArray USButils::createGetDescriptor_Configuration( int lengthOfConfigSection ) {
	if ( lengthOfConfigSection <= 0 ) lengthOfConfigSection = 0x09;
	char buff[8];
	memcpy( buff, "\x80\x06\x00\x02\x00\x00", 6 );
	buff[6] = (lengthOfConfigSection & 0x00ff);
	buff[7] = ((lengthOfConfigSection >> 8) & 0x00ff);
	return QByteArray( buff, 8 );
}

QByteArray USButils::createGetDescriptor_String( int stringIndex, int langCode, int length ) {
	char buff[] = "\x80\x06\x00\x03\x00\x00\xff\x00";
	buff[2] = (stringIndex & 0x00ff);
	buff[4] = (langCode & 0x00ff);
	buff[5] = ((langCode >> 8) & 0x00ff);
	buff[6] = (length & 0x00ff);
	buff[7] = ((length >> 8) & 0x00ff);
	return QByteArray( buff, 8 );
}

USButils::UsbDeviceDescriptor USButils::decodeDeviceDescriptor( const QByteArray & bytes ) {
	USButils::UsbDeviceDescriptor retVal;
	retVal.bcdUSB = 0;
	if ( bytes.isNull() || bytes.length() < 0x12 ) return retVal;

	int len = bytes[0];	// length of device descriptor dataset
	if ( len != 0x12 ) return retVal;	// length value must be 0x12 - otherwise something is terrible wrong
	if ( bytes[1] != 0x01 ) return retVal;	// descriptor type should be 0x01

	retVal.bcdUSB = bytesToShort( bytes[2], bytes[3] );
	retVal.bDeviceClass = bytes[4];
	retVal.bDeviceSubClass = bytes[5];
	retVal.bDeviceProtocol = bytes[6];
	retVal.idVendor = bytesToShort( bytes[8], bytes[9] );
	retVal.idProduct = bytesToShort( bytes[10], bytes[11] );
	retVal.bcdDevice = bytesToShort( bytes[12], bytes[13] );
	retVal.iManufactor = bytes[14];
	retVal.iProduct = bytes[15];
	retVal.iSerialNumber = bytes[16];
	retVal.bNumConfigurations = bytes[17];

	return retVal;
}



int USButils::decodeAvailableLanguageCodes( const QByteArray & bytes, QList<int> & availableLanguageCodes ) {
	if ( bytes.isNull() || bytes.length() < 4 || bytes[1] != 0x03 ) {
		availableLanguageCodes << USB_DEFAULT_LANGUAGE_CODE;
		return USB_DEFAULT_LANGUAGE_CODE;
	}
	int lenBytes = bytes.length();
	int idx = 2;
	int preferedLang = 0;
	while ( idx < lenBytes-1 ) {
		short langCode = bytesToShort( bytes[idx], bytes[idx+1] );
		if ( langCode != 0 ) {
			availableLanguageCodes << (int) langCode;
			if ( langCode == 0x0409 )
				preferedLang = USB_DEFAULT_LANGUAGE_CODE;
		}
		idx += 2;
	}
	if ( availableLanguageCodes.isEmpty() )
		preferedLang = USB_DEFAULT_LANGUAGE_CODE;

	if ( preferedLang == 0 )
		preferedLang = availableLanguageCodes[0];

	return preferedLang;
}

USButils::UsbConfigurationDescriptor USButils::decodeConfigurationSection(
		const QByteArray & bytes, int sizeOfConfigurationSection ) {
	USButils::UsbConfigurationDescriptor retVal;
	retVal.totalLength = 0;
	if ( bytes.isNull() || bytes.length() < 0x09 || bytes.length() < sizeOfConfigurationSection )
		return retVal;

	retVal.totalLength = bytesToShort( bytes[2], bytes[3] );
	retVal.bNumInterfaces = bytes[4];
	retVal.bConfigurationValue = bytes[5];
	retVal.iConfiguration = bytes[6];
	retVal.bmAttributes = bytes[7];
	retVal.bMaxPower = bytes[8];

	return retVal;
}

void USButils::decodeConfigurationSectionComplete(
		const QByteArray & bytes, USButils::UsbConfigurationDescriptor & configSection ) {
	if ( bytes.isNull() || bytes.length() < 0x09 ) return;

	USButils::UsbInterfaceDescriptor *currentInterfaceDescriptor;
	USButils::UsbEndpointDescriptor  *currentEndpointDescriptor;
	unsigned char lengthOfPacket = bytes[0];
	unsigned char typeOfPacket = bytes[1];
	bool done = false;
	int offset = 0;
	bool isFirst = true, haveEndPtDS = false;
	int lastDecodedDescriptorType = 0;
	do {
		lengthOfPacket = bytes[ 0 + offset ];
		typeOfPacket = bytes[ 1 + offset ];
		switch ( typeOfPacket ) {
		case 0x01:
			// device descriptor
			// should not occur - ignore!
			lastDecodedDescriptorType = 0x01;
			break;
		case 0x02:
			// configuration descriptor - already decoded
			// ignore
			lastDecodedDescriptorType = 0x02;
			break;
		case 0x03:
			// string descriptor
			// should not occur (?) - ignore
			lastDecodedDescriptorType = 0x03;
			break;
		case 0x04:
			// Interface descriptor
			if ( ! isFirst ) {
				if ( haveEndPtDS )
					currentInterfaceDescriptor->listEndpoints.append( currentEndpointDescriptor );
				configSection.listInterface.append( currentInterfaceDescriptor );
			}
			currentInterfaceDescriptor = decodeInterfaceSection( bytes, offset );
			isFirst = false;
			haveEndPtDS = false;
			lastDecodedDescriptorType = 0x04;
			break;
		case 0x05:
			// Endpoint descriptor
			if ( haveEndPtDS )
				currentInterfaceDescriptor->listEndpoints.append( currentEndpointDescriptor );
			currentEndpointDescriptor = decodeEndpointSection( bytes, offset );
			haveEndPtDS = true;
			lastDecodedDescriptorType = 0x05;
			break;


		case 0x24:
			decodeUsbAudioInterfaceDescriptor( bytes, offset, currentInterfaceDescriptor );
			break;
		case 0x25:
			// AudioControl Endpoint Descriptor
			break;

		case 0x21:
			// HID descriptor
		case 0x22:
			// report descriptor (?)
		case 0x23:
			// physical device descriptor
		case 0x29:
			// HUB descriptor
		default: {
			USButils::UsbSupplementalDescriptor *dt = new USButils::UsbSupplementalDescriptor();
			dt->bType = typeOfPacket;
			dt->bLength = lengthOfPacket;
			dt->data = new unsigned char[lengthOfPacket +1];
			::memcpy( dt->data, bytes.data(), lengthOfPacket );
			dt->data[lengthOfPacket] = 0;
			if ( lastDecodedDescriptorType == 0x04 ) {
				// Interface descriptor
				currentInterfaceDescriptor->listInterfaceSupplementalDescriptors.append( dt );
			} else if ( lastDecodedDescriptorType == 0x05 ) {
				// Endpoint descriptor
				currentEndpointDescriptor->listEndpointDescriptors.append( dt );
			}
			// something different...
			Logger::getLogger("USB")->info( QString("Found USB descriptor with type 0x%1 and size %2 bytes").arg(
					QString::number(typeOfPacket, 16),
					QString::number(lengthOfPacket, 10)
					) );
			break;
		}
		}

		// end of config section
		if ( offset + lengthOfPacket >= bytes.length()-1 )
			done = true;
		else
			offset += lengthOfPacket;
	} while ( !done );

	if ( haveEndPtDS )
		currentInterfaceDescriptor->listEndpoints.append( currentEndpointDescriptor );
	configSection.listInterface.append( currentInterfaceDescriptor );
}

USButils::UsbInterfaceDescriptor * USButils::decodeInterfaceSection( const QByteArray & bytes, int offset ) {
	USButils::UsbInterfaceDescriptor *retVal = new USButils::UsbInterfaceDescriptor;
	retVal->bInterfaceNumber = bytes[2+offset];
	retVal->bAlternateSetting = bytes[3+offset];
	retVal->bNumEndpoints = bytes[4+offset];
	retVal->bInterfaceClass = bytes[5+offset];
	retVal->bInterfaceSubClass = bytes[6+offset];
	retVal->bInterfaceProtocol = bytes[7+offset];
	retVal->iInterface = bytes[8+offset];
	retVal->sInterface = QString::null;
	return retVal;
}

USButils::UsbEndpointDescriptor * USButils::decodeEndpointSection( const QByteArray & bytes, int offset ) {
	USButils::UsbEndpointDescriptor *retVal = new USButils::UsbEndpointDescriptor;
	unsigned char lengthOfPacket = bytes[ 0 + offset ];
	retVal->bLength = lengthOfPacket;
	retVal->bEndpointAddress = bytes[2+offset];
	retVal->bmAttributes = bytes[3+offset];
	retVal->wMaxPacketSize = bytesToShort( bytes[4+offset], bytes[5+offset]);
	retVal->bInterval = bytes[6+offset];
	if ( lengthOfPacket > 7 ) {
		retVal->bRefresh = bytes[7+offset];;
		retVal->bSynchAddress = bytes[8+offset];
	} else {
		retVal->bRefresh = 0;
		retVal->bSynchAddress = 0;
	}
	return retVal;
}


int USButils::getConfigsectionLengthFromURB( const QByteArray & bytes ) {
	if ( bytes.isNull() || bytes.length() < 0x09 ) return 0x09;
	return bytesToShort( bytes[2], bytes[3] );
}


/* ************ USB Audio class decriptor decoding ********** */


void USButils::decodeUsbAudioInterfaceDescriptor( const QByteArray & bytes, int offset, USButils::UsbInterfaceDescriptor * refInterface ) {
	unsigned char subClassInterface = refInterface->bInterfaceSubClass;
	unsigned char descriptorLen     = bytes[ offset ];
	unsigned char descriptorType    = bytes[ 1 + offset ];
	unsigned char descriptorSubType = bytes[ 2 + offset ];
	switch ( subClassInterface ) {
	case 0x01: {
		// Audio control interface
		switch ( descriptorSubType ) {
		case 0x01: {
			// Header
			USButils::UsbAudioInterfaceDescriptor *dt = new USButils::UsbAudioInterfaceDescriptor();	// TODO use autopointer
			dt->bLength = descriptorLen;
			dt->bType = descriptorType;
			dt->bDescriptorSubtype = descriptorSubType;
			decodeUsbAudioInterfaceHeader( bytes, offset, *dt );
			refInterface->listInterfaceSupplementalDescriptors.append( dt );
		}
		case 0x02: {
			// Input_Terminal
			USButils::UsbAudioInputTerminalDescriptor *dt = new USButils::UsbAudioInputTerminalDescriptor();	// TODO use autopointer
			dt->bLength = descriptorLen;
			dt->bType = descriptorType;
			dt->bDescriptorSubtype = descriptorSubType;
			decodeUsbAudioTerminalDescription_Input( bytes, offset, *dt );
			refInterface->listInterfaceSupplementalDescriptors.append( dt );
			break;
		}
		case 0x03: {
			// Output_Terminal
			USButils::UsbAudioOutputTerminalDescriptor *dt = new USButils::UsbAudioOutputTerminalDescriptor();	// TODO use autopointer
			dt->bLength = descriptorLen;
			dt->bType = descriptorType;
			dt->bDescriptorSubtype = descriptorSubType;
			decodeUsbAudioTerminalDescription_Output( bytes, offset, *dt );
			refInterface->listInterfaceSupplementalDescriptors.append( dt );
			break;
		}
		case 0x04: {
			// Mixer_Unit
			USButils::UsbAudioMixerTerminalDescriptor *dt = new USButils::UsbAudioMixerTerminalDescriptor;
			dt->bLength = descriptorLen;
			dt->bType = descriptorType;
			dt->bDescriptorSubtype = descriptorSubType;
			decodeUsbAudioTerminalDescription_Mixer( bytes, offset, *dt );
			refInterface->listInterfaceSupplementalDescriptors.append( dt );
			break;
		}
		case 0x05: {
			// Selector_Unit
			USButils::UsbAudioSelectorTerminalDescriptor *dt = new USButils::UsbAudioSelectorTerminalDescriptor;
			dt->bLength = descriptorLen;
			dt->bType = descriptorType;
			dt->bDescriptorSubtype = descriptorSubType;
			decodeUsbAudioTerminalDescription_Selector( bytes, offset, *dt );
			refInterface->listInterfaceSupplementalDescriptors.append( dt );
			break;
		}
		case 0x06: {
			// Feature_Unit
			USButils::UsbAudioFeatureTerminalDescriptor *dt = new USButils::UsbAudioFeatureTerminalDescriptor;
			dt->bLength = descriptorLen;
			dt->bType = descriptorType;
			dt->bDescriptorSubtype = descriptorSubType;
			decodeUsbAudioTerminalDescription_Feature( bytes, offset, *dt );
			refInterface->listInterfaceSupplementalDescriptors.append( dt );
			break;
		}
		case 0x07: {
			// Processing_Unit
			USButils::UsbAudioProcessingTerminalDescriptor *dt = new USButils::UsbAudioProcessingTerminalDescriptor;
			dt->bLength = descriptorLen;
			dt->bType = descriptorType;
			dt->bDescriptorSubtype = descriptorSubType;
			decodeUsbAudioTerminalDescription_Processing( bytes, offset, *dt );
			refInterface->listInterfaceSupplementalDescriptors.append( dt );
			break;
		}
		case 0x08: {
			// Extension_Unit
			USButils::UsbAudioExtensionTerminalDescriptor *dt = new USButils::UsbAudioExtensionTerminalDescriptor;
			dt->bLength = descriptorLen;
			dt->bType = descriptorType;
			dt->bDescriptorSubtype = descriptorSubType;
			decodeUsbAudioTerminalDescription_Extension( bytes, offset, *dt );
			refInterface->listInterfaceSupplementalDescriptors.append( dt );
			break;
		}
		default: {
			USButils::UsbSupplementalDescriptor *dt = new USButils::UsbSupplementalDescriptor();
			dt->bType = descriptorType;
			dt->bLength = descriptorLen;
			dt->data = new unsigned char[descriptorLen +1];
			::memcpy( dt->data, bytes.data(), descriptorLen );
			dt->data[descriptorLen] = 0;
			refInterface->listInterfaceSupplementalDescriptors.append( dt );
		}
		}
		break;
	}
	case 0x02:
		// Audio stream interface: DSP
	case 0x03:
		// Audio stream interface: MIDI
		USButils::UsbSupplementalDescriptor *dt = new USButils::UsbSupplementalDescriptor();
		dt->bType = descriptorType;
		dt->bLength = descriptorLen;
		dt->data = new unsigned char[descriptorLen +1];
		::memcpy( dt->data, bytes.data(), descriptorLen );
		dt->data[descriptorLen] = 0;
		refInterface->listInterfaceSupplementalDescriptors.append( dt );
		break;
	}
}


void USButils::decodeUsbAudioInterfaceHeader(
		const QByteArray & bytes, int offset, USButils::UsbAudioInterfaceDescriptor & dt ) {
	dt.bcdADC = bytesToShort( bytes[ 3+offset ], bytes[ 4+offset ] );
	dt.wTotalLength = bytesToShort( bytes[ 5+offset ], bytes[ 6+offset ] );
	dt.bInCollection = bytes[ 7 + offset ];
	// fill dynamic length interface association values
	for ( int i = 8; i < dt.bLength; i++ ) {
		unsigned char baInterfaceNr = bytes[ i + offset ];
		dt.listBaInterfaceNr << baInterfaceNr;
	}
}
void USButils::decodeUsbAudioTerminalDescription_Input(
		const QByteArray & bytes, int offset, USButils::UsbAudioInputTerminalDescriptor & dt ) {
	if ( bytes.length() <  offset + 12 || dt.bLength < 12 ) return;
	dt.bTerminalID = bytes[ 3 + offset ];
	dt.wTerminalType  = bytesToShort( bytes[4+offset], bytes[5+offset] );
	dt.bAssocTerminal = bytes[ 6 + offset ];
	dt.bNrChannels    = bytes[ 7 + offset ];
	dt.wChannelConfig = bytesToShort( bytes[8+offset], bytes[9+offset] );
	dt.iChannelNames  = bytes[ 10 + offset ];
	dt.iTerminal      = bytes[ 11 + offset ];
}
void USButils::decodeUsbAudioTerminalDescription_Output(
		const QByteArray & bytes, int offset, USButils::UsbAudioOutputTerminalDescriptor & dt ) {
	if ( bytes.length() <  offset + 9 || dt.bLength < 9 ) return;
	dt.bTerminalID = bytes[ 3 + offset ];
	dt.wTerminalType  = bytesToShort( bytes[4+offset], bytes[5+offset] );
	dt.bAssocTerminal = bytes[ 6 + offset ];
	dt.bSourceID      = bytes[ 7 + offset ];
	dt.iTerminal      = bytes[ 8 + offset ];
}


void USButils::decodeUsbAudioTerminalDescription_Mixer     (
		const QByteArray & bytes, int offset, USButils::UsbAudioMixerTerminalDescriptor & dt ) {
	dt.bUnitID = bytes[ 3 + offset ];
}
void USButils::decodeUsbAudioTerminalDescription_Selector  (
		const QByteArray & bytes, int offset, USButils::UsbAudioSelectorTerminalDescriptor & dt ) {
	dt.bUnitID = bytes[ 3 + offset ];
}
void USButils::decodeUsbAudioTerminalDescription_Feature   (
		const QByteArray & bytes, int offset, USButils::UsbAudioFeatureTerminalDescriptor & dt ) {
	unsigned char len = bytes[ offset ];
	dt.bUnitID = bytes[ 3 + offset ];
	dt.bSourceID = bytes[ 4 + offset ];
	dt.bControlSize = bytes[ 5 + offset ];
	if ( dt.bControlSize < 1 ) dt.bControlSize = 1;

	if ( len <= 7 ) dt.bmaControls0 = 0;
	if ( len > 7 ) {
		for ( int pt = 6; pt < len -1; pt += dt.bControlSize ) {
			int bmaControl = 0;
			for ( int b = 0; b < dt.bControlSize; b++ ) {
				if ( b > 3 ) continue; // not supported
				int val = (bytes[ offset + pt + b ] & 0xff);
				if ( b > 1 )
					val = (val << (b*8));
				bmaControl |= val;
			}
			if ( pt == 6 ) {
				// first entry == master control ( aka: bmaControl_0 )
				dt.bmaControls0 = bmaControl;
			} else
				dt.listBmaControls << bmaControl;
		}
	}
	dt.iFeature = bytes[ offset + len -1 ];
}
void USButils::decodeUsbAudioTerminalDescription_Processing(
		const QByteArray & bytes, int offset, USButils::UsbAudioProcessingTerminalDescriptor & dt ) {
	dt.bUnitID = bytes[ 3 + offset ];
}
void USButils::decodeUsbAudioTerminalDescription_Extension (
		const QByteArray & bytes, int offset, USButils::UsbAudioExtensionTerminalDescriptor & dt ) {
	dt.bUnitID = bytes[ 3 + offset ];
}



/* ************* Byte converstion methods ************ */

QString USButils::decodeStringDescriptor( const QByteArray & bytes ) {
//	LogWriter::trace(QString("decodeStringDescriptor bytes.len = %1").arg( QString::number( bytes.length()) ));
	if ( bytes.isNull() || bytes.length() < 2 || bytes[1] != 0x03 ) return QString::null;

	int len = bytes[0];
	if ( bytes.length() < len ) len = bytes.length();
	len -= 2;	// minus 2 bytes header
//	LogWriter::trace(QString("decodeStringDescriptor string.len = %1").arg( QString::number( len ) ));
	if ( len <= 0 ) return QString("");	// empty string (not an error!)
	QByteArray stringBytes = bytes.right( len );
	return QString::fromUtf16( (ushort*) stringBytes.data(), len / 2 );
}




unsigned short USButils::bytesToShort( unsigned char byteLSB, unsigned char byteMSB ) {
	unsigned short sh = 0;
	sh |= byteLSB;
	sh |= ((byteMSB << 8) & 0xff00);
	return sh;
}
