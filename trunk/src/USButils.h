/*
 * USButils.h
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2010-10-14
 */

#ifndef USBUTILS_H_
#define USBUTILS_H_


#include <QByteArray>
#include <QList>
#include <QString>
#include <stdint.h>

#define USB_DEFAULT_LANGUAGE_CODE	(0x0409);	// default language: english (en_US)

class USButils {
public:
	struct UsbSupplementalDescriptor {
		UsbSupplementalDescriptor() {
			data = NULL;
		};
		virtual ~UsbSupplementalDescriptor() {
			if ( data ) delete data;
		};
		uint8_t  bLength;
		uint8_t  bType;
		uint8_t* data;
	};
	// Audio-Interface Descriptor (header)
	struct UsbAudioInterfaceDescriptor : public UsbSupplementalDescriptor {
		uint8_t  bDescriptorSubtype;	// 0x01
		unsigned short bcdADC;
		unsigned short wTotalLength;
		uint8_t  bInCollection;
		QList<uint8_t> listBaInterfaceNr;
	};
	// Audio-Interface Input-Terminal descriptor
	struct UsbAudioInputTerminalDescriptor : public UsbSupplementalDescriptor {
		uint8_t  bDescriptorSubtype;	// 0x02
		uint8_t  bTerminalID;
		unsigned short wTerminalType;
		uint8_t  bAssocTerminal;
		uint8_t  bNrChannels;
		unsigned short wChannelConfig;
		uint8_t  iChannelNames;
		uint8_t  iTerminal;
	};
	// Audio-Interface Input-Terminal descriptor
	struct UsbAudioOutputTerminalDescriptor : public UsbSupplementalDescriptor {
		uint8_t  bDescriptorSubtype;	// 0x03
		uint8_t  bTerminalID;
		unsigned short wTerminalType;
		uint8_t  bAssocTerminal;
		uint8_t  bSourceID;
		uint8_t  iTerminal;
	};
	// Audio-Interface Mixer-Unit Terminal descriptor
	struct UsbAudioMixerTerminalDescriptor : public UsbSupplementalDescriptor {
		uint8_t  bDescriptorSubtype;	// 0x04
		uint8_t  bUnitID;
		// TODO
	};
	// Audio-Interface Selector-Unit Terminal descriptor
	struct UsbAudioSelectorTerminalDescriptor : public UsbSupplementalDescriptor {
		uint8_t  bDescriptorSubtype;	// 0x05
		uint8_t  bUnitID;
		// TODO

	};
	// Audio-Interface Feature-Unit Terminal descriptor
	struct UsbAudioFeatureTerminalDescriptor : public UsbSupplementalDescriptor {
		uint8_t  bDescriptorSubtype;	// 0x06
		uint8_t  bUnitID;
		uint8_t  bSourceID;
		uint8_t  bControlSize;
		unsigned int   bmaControls0;
		QList<unsigned int> listBmaControls;
		uint8_t  iFeature;
	};
	// Audio-Interface Processing-Unit Terminal descriptor
	struct UsbAudioProcessingTerminalDescriptor : public UsbSupplementalDescriptor {
		uint8_t  bDescriptorSubtype;	// 0x07
		uint8_t  bUnitID;
		// TODO

	};
	// Audio-Interface Extension-Unit Terminal descriptor
	struct UsbAudioExtensionTerminalDescriptor : public UsbSupplementalDescriptor {
		uint8_t  bDescriptorSubtype;	// 0x08
		uint8_t  bUnitID;
		// TODO
	};



	struct UsbEndpointDescriptor {
		~UsbEndpointDescriptor() {
			for ( int i = 0; i < listEndpointDescriptors.size(); i++ )
				delete listEndpointDescriptors[i];
			listEndpointDescriptors.clear();
		};
		uint8_t  bLength;
		uint8_t  bEndpointAddress;
		uint8_t  bmAttributes;
		unsigned short wMaxPacketSize;
		uint8_t  bInterval;
		uint8_t  bRefresh;
		uint8_t  bSynchAddress;
		QList<UsbSupplementalDescriptor*> listEndpointDescriptors;
	};

	struct UsbInterfaceDescriptor {
		~UsbInterfaceDescriptor() {
			for ( int i = 0; i < listEndpoints.size(); i++ )
				delete listEndpoints[i];
			for ( int i = 0; i < listInterfaceSupplementalDescriptors.size(); i++ )
				delete listInterfaceSupplementalDescriptors[i];
			listEndpoints.clear();
			listInterfaceSupplementalDescriptors.clear();
		};
		uint8_t  bInterfaceNumber;
		uint8_t  bAlternateSetting;
		uint8_t  bNumEndpoints;
		uint8_t  bInterfaceClass;
		uint8_t  bInterfaceSubClass;
		uint8_t  bInterfaceProtocol;
		uint8_t  iInterface;
		QString sInterface;
		QList<UsbEndpointDescriptor*> listEndpoints;
		QList<UsbSupplementalDescriptor*> listInterfaceSupplementalDescriptors;
	};

	struct UsbConfigurationDescriptor {
		~UsbConfigurationDescriptor() {
			for ( int i = 0; i < listInterface.size(); i++ )
				delete listInterface[i];
			listInterface.clear();
		};
		unsigned short totalLength;
		uint8_t  bNumInterfaces;
		uint8_t  bConfigurationValue;
		uint8_t  iConfiguration;
		uint8_t  bmAttributes;
		uint8_t  bMaxPower;
		QString sConfig;
		QList<UsbInterfaceDescriptor*> listInterface;
	};
	struct UsbDeviceDescriptor {
		unsigned short bcdUSB;
		uint8_t bDeviceClass;
		uint8_t bDeviceSubClass;
		uint8_t bDeviceProtocol;
		uint8_t bMaxPacketSize;
		unsigned short idVendor;
		unsigned short idProduct;
		unsigned short bcdDevice;
		uint8_t iManufactor;
		uint8_t iProduct;
		uint8_t iSerialNumber;
		uint8_t bNumConfigurations;
		QString sVendor;
		QString sManufactor;
		QString sProduct;
		QString sSerialNumber;
	};

	USButils();

	/**
	 * Creates a simple "Get Descriptor" request for <em>overall</em> device information.
	 */
	static QByteArray createGetDescriptor_Device();
	static QByteArray createGetDescriptor_Configuration( int lengthOfConfigSection = 0x09 );
	static USButils::UsbDeviceDescriptor decodeDeviceDescriptor( const QByteArray & bytes );
	static QByteArray createGetDescriptor_String( int stringIndex = 0x00, int langCode = 0x00, int length = 0xff );
	static int getConfigsectionLengthFromURB( const QByteArray & bytes );
	static int decodeAvailableLanguageCodes( const QByteArray & bytes, QList<int> & availableLanguageCodes );
	static USButils::UsbConfigurationDescriptor decodeConfigurationSection( const QByteArray & bytes, int sizeOfConfigurationSection );
	static void decodeConfigurationSectionComplete( const QByteArray & bytes, USButils::UsbConfigurationDescriptor & configSection );
	static USButils::UsbInterfaceDescriptor *decodeInterfaceSection( const QByteArray & bytes, int offset );
	static USButils::UsbEndpointDescriptor *decodeEndpointSection( const QByteArray & bytes, int offset );
	static QString decodeStringDescriptor( const QByteArray & bytes );
	static void decodeUsbAudioInterfaceDescriptor           ( const QByteArray & bytes, int offset, USButils::UsbInterfaceDescriptor * refInterface );

	/** Decodes a BCD string to a short value */
	static short decodeBCDToShort( const QString & sBCDstring );

private:
	static inline unsigned short bytesToShort( uint8_t byteLSB, uint8_t byteMSB );
	static void decodeUsbAudioInterfaceHeader               ( const QByteArray & bytes, int offset, USButils::UsbAudioInterfaceDescriptor & dt );
	static void decodeUsbAudioTerminalDescription_Input     ( const QByteArray & bytes, int offset, USButils::UsbAudioInputTerminalDescriptor & dt );
	static void decodeUsbAudioTerminalDescription_Output    ( const QByteArray & bytes, int offset, USButils::UsbAudioOutputTerminalDescriptor & dt );
	static void decodeUsbAudioTerminalDescription_Mixer     ( const QByteArray & bytes, int offset, USButils::UsbAudioMixerTerminalDescriptor & dt );
	static void decodeUsbAudioTerminalDescription_Selector  ( const QByteArray & bytes, int offset, USButils::UsbAudioSelectorTerminalDescriptor & dt );
	static void decodeUsbAudioTerminalDescription_Feature   ( const QByteArray & bytes, int offset, USButils::UsbAudioFeatureTerminalDescriptor & dt );
	static void decodeUsbAudioTerminalDescription_Processing( const QByteArray & bytes, int offset, USButils::UsbAudioProcessingTerminalDescriptor & dt );
	static void decodeUsbAudioTerminalDescription_Extension ( const QByteArray & bytes, int offset, USButils::UsbAudioExtensionTerminalDescriptor & dt );
};

#endif /* USBUTILS_H_ */
