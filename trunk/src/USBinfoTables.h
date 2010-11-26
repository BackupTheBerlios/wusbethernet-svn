/*
 * USBinfoTables.h
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2010-10-25
 */

#ifndef USBINFOTABLES_H_
#define USBINFOTABLES_H_

#include <QMap>
#include <QString>

class USBinfoTables {
public:
	virtual ~USBinfoTables();

	static USBinfoTables & getInstance();

	const QString getVendorByID( unsigned short vid, const QString & defaultVal = "n/a" );
	const QString getLanguageByID( unsigned short lid, const QString & defaultVal = "en_US" );
	const QString getClassDescriptionByClassID( unsigned char cid, const QString & defaultVal = "n/a" );
	const QString getDeviceDescriptionByClassSubclassAndProtocol(
			unsigned char cid, unsigned char sid, unsigned char protoId, const QString & defaultVal = "n/a" );
	const QString getAudioTerminalTypeDescriptionByID( unsigned short wTerminalType, const QString & defaultVal = "n/a" );
	const QString audioFeatureMapToString( int bmaControl );
private:

	static USBinfoTables * singletonInst;
	QMap<unsigned short,QString> vendorIDmap;
	QMap<unsigned short,QString> languageIDmap;
	QMap<unsigned char,QString> classIDmap;
	QMap<int,QString> extendedClassIDmap;
	QMap<unsigned short,QString> usbAudioTerminalTypesMap;

	USBinfoTables();

	void initTables();
	void initVIDtable();
	void initLIDtable();
	void initDeviceClassTable();
	void initExtendedDeviceClassTable();
	void initAudioTerminalTypesTable();
};

#endif /* USBINFOTABLES_H_ */
