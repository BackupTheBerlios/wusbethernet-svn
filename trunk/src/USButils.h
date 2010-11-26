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
		unsigned char  bLength;
		unsigned char  bType;
		unsigned char* data;
	};
	// Audio-Interface Descriptor (header)
	struct UsbAudioInterfaceDescriptor : public UsbSupplementalDescriptor {
		unsigned char  bDescriptorSubtype;	// 0x01
		unsigned short bcdADC;
		unsigned short wTotalLength;
		unsigned char  bInCollection;
		QList<unsigned char> listBaInterfaceNr;
	};
	// Audio-Interface Input-Terminal descriptor
	struct UsbAudioInputTerminalDescriptor : public UsbSupplementalDescriptor {
		unsigned char  bDescriptorSubtype;	// 0x02
		unsigned char  bTerminalID;
		unsigned short wTerminalType;
		unsigned char  bAssocTerminal;
		unsigned char  bNrChannels;
		unsigned short wChannelConfig;
		unsigned char  iChannelNames;
		unsigned char  iTerminal;
	};
	// Audio-Interface Input-Terminal descriptor
	struct UsbAudioOutputTerminalDescriptor : public UsbSupplementalDescriptor {
		unsigned char  bDescriptorSubtype;	// 0x03
		unsigned char  bTerminalID;
		unsigned short wTerminalType;
		unsigned char  bAssocTerminal;
		unsigned char  bSourceID;
		unsigned char  iTerminal;
	};
	// Audio-Interface Mixer-Unit Terminal descriptor
	struct UsbAudioMixerTerminalDescriptor : public UsbSupplementalDescriptor {
		unsigned char  bDescriptorSubtype;	// 0x04
		unsigned char  bUnitID;
		// TODO
	};
	// Audio-Interface Selector-Unit Terminal descriptor
	struct UsbAudioSelectorTerminalDescriptor : public UsbSupplementalDescriptor {
		unsigned char  bDescriptorSubtype;	// 0x05
		unsigned char  bUnitID;
		// TODO

	};
	// Audio-Interface Feature-Unit Terminal descriptor
	struct UsbAudioFeatureTerminalDescriptor : public UsbSupplementalDescriptor {
		unsigned char  bDescriptorSubtype;	// 0x06
		unsigned char  bUnitID;
		unsigned char  bSourceID;
		unsigned char  bControlSize;
		unsigned int   bmaControls0;
		QList<unsigned int> listBmaControls;
		unsigned char  iFeature;
	};
	// Audio-Interface Processing-Unit Terminal descriptor
	struct UsbAudioProcessingTerminalDescriptor : public UsbSupplementalDescriptor {
		unsigned char  bDescriptorSubtype;	// 0x07
		unsigned char  bUnitID;
		// TODO

	};
	// Audio-Interface Extension-Unit Terminal descriptor
	struct UsbAudioExtensionTerminalDescriptor : public UsbSupplementalDescriptor {
		unsigned char  bDescriptorSubtype;	// 0x08
		unsigned char  bUnitID;
		// TODO
	};



	struct UsbEndpointDescriptor {
		~UsbEndpointDescriptor() {
			for ( int i = 0; i < listEndpointDescriptors.size(); i++ )
				delete listEndpointDescriptors[i];
			listEndpointDescriptors.clear();
		};
		unsigned char  bLength;
		unsigned char  bEndpointAddress;
		unsigned char  bmAttributes;
		unsigned short wMaxPacketSize;
		unsigned char  bInterval;
		unsigned char  bRefresh;
		unsigned char  bSynchAddress;
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
		unsigned char  bInterfaceNumber;
		unsigned char  bAlternateSetting;
		unsigned char  bNumEndpoints;
		unsigned char  bInterfaceClass;
		unsigned char  bInterfaceSubClass;
		unsigned char  bInterfaceProtocol;
		unsigned char  iInterface;
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
		unsigned char  bNumInterfaces;
		unsigned char  bConfigurationValue;
		unsigned char  iConfiguration;
		unsigned char  bmAttributes;
		unsigned char  bMaxPower;
		QString sConfig;
		QList<UsbInterfaceDescriptor*> listInterface;
	};
	struct UsbDeviceDescriptor {
		unsigned short bcdUSB;
		unsigned char bDeviceClass;
		unsigned char bDeviceSubClass;
		unsigned char bDeviceProtocol;
		unsigned char bMaxPacketSize;
		unsigned short idVendor;
		unsigned short idProduct;
		unsigned short bcdDevice;
		unsigned char iManufactor;
		unsigned char iProduct;
		unsigned char iSerialNumber;
		unsigned char bNumConfigurations;
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
private:
	static inline unsigned short bytesToShort( unsigned char byteLSB, unsigned char byteMSB );
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
