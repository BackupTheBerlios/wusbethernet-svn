/*
 * USBinfoTables.cpp
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2010-10-25
 */

#include "USBinfoTables.h"
#include "config.h"
#include "USBinfoTablesVIDs.cpp"
#include "USBinfoTablesLID.cpp"

// init static variables
USBinfoTables::USBinfoTables * USBinfoTables::singletonInst = NULL;

/**
 * Init tables
 */
USBinfoTables::USBinfoTables() {
	vendorIDmap.clear();
	languageIDmap.clear();
	initTables();
}

USBinfoTables::~USBinfoTables() {
	vendorIDmap.clear();
	languageIDmap.clear();
}

USBinfoTables & USBinfoTables::getInstance() {
	USBinfoTables * inst = singletonInst;
	if ( inst == NULL ) {
		singletonInst = inst = new USBinfoTables();
	}
	return *inst;
}

void USBinfoTables::initTables() {
	initVIDtable();
	initLIDtable();
	initDeviceClassTable();
	initExtendedDeviceClassTable();
	initAudioTerminalTypesTable();
}

const QString USBinfoTables::getVendorByID( unsigned short vid, const QString & defaultVal ) {
	if ( vendorIDmap.contains( vid ) )
		return vendorIDmap.value( vid, defaultVal );
	else
		return defaultVal;
}

const QString USBinfoTables::getLanguageByID( unsigned short lid, const QString & defaultVal ) {
	if ( languageIDmap.contains( lid ) )
		return languageIDmap.value( lid, defaultVal );
	else
		return defaultVal;
}

const QString USBinfoTables::getClassDescriptionByClassID( unsigned char cid, const QString & defaultVal ) {
	if ( classIDmap.contains( cid ) )
		return classIDmap.value( cid, defaultVal );
	else
		return defaultVal;

}
const QString USBinfoTables::getDeviceDescriptionByClassSubclassAndProtocol(
		unsigned char cid, unsigned char sid, unsigned char protoId, const QString & defaultVal ) {

	int key = (protoId & 0xff);
	key |= ((sid << 8) & 0xff00);
	key |= ((cid << 16) & 0xff0000);
	if ( extendedClassIDmap.contains( key ) )
		return extendedClassIDmap.value( key, defaultVal );
	else {
		key = ((sid << 8) & 0xff00);
		key |= ((cid << 16) & 0xff0000);
		if ( extendedClassIDmap.contains( key ) )
			return extendedClassIDmap.value( key, defaultVal );
		return getClassDescriptionByClassID( cid, defaultVal );
	}
	return defaultVal;
}


const QString USBinfoTables::getAudioTerminalTypeDescriptionByID(
		unsigned short wTerminalType, const QString & defaultVal ) {
	if ( usbAudioTerminalTypesMap.contains( wTerminalType ) )
		return usbAudioTerminalTypesMap.value( wTerminalType, defaultVal );
	else
		return defaultVal;
}

void USBinfoTables::initDeviceClassTable() {
	classIDmap[0x00] = "(Defined at Interface level)";
	classIDmap[0x01] = "Audio";
	classIDmap[0x02] = "Communications";
	classIDmap[0x03] = "Human Interface Device (HID)";
	classIDmap[0x05] = "Physical Interface Device";
	classIDmap[0x06] = "Imaging";
	classIDmap[0x07] = "Printer";
	classIDmap[0x08] = "Mass Storage";
	classIDmap[0x09] = "Hub";
	classIDmap[0x0a] = "CDC Data";
	classIDmap[0x0b] = "Chip/SmartCard";
	classIDmap[0x0d] = "Content Security";
	classIDmap[0x0e] = "Video";
	classIDmap[0x58] = "Xbox";
	classIDmap[0xdc] = "Diagnostic";
	classIDmap[0xe0] = "Wireless Communication";
	classIDmap[0xef] = "Miscellaneous";
	classIDmap[0xfe] = "Application Specific Interface";
	classIDmap[0xff] = "Vendor Specific Class";
}
void USBinfoTables::initExtendedDeviceClassTable() {
	extendedClassIDmap[0x00] = "(Defined at Interface level)";
	extendedClassIDmap[0x010100] = "Audio: Control Device";	// Audio: Class = 01 Subclass = 01
	extendedClassIDmap[0x010200] = "Audio: Streaming";		// Audio: Class = 01 Subclass = 02
	extendedClassIDmap[0x010300] = "Audio: MIDI Streaming";	// Audio: Class = 01 Subclass = 03
	extendedClassIDmap[0x020100] = "Communication: Direct Line";		// Communication: 02 01 00
	extendedClassIDmap[0x020200] = "Communication: Abstract (Modem)";
	extendedClassIDmap[0x020201] = "Communication: Modem: AT-commands (v.25ter)";
	extendedClassIDmap[0x020202] = "Communication: Modem: AT-commands (PCCA101)";
	extendedClassIDmap[0x020203] = "Communication: Modem: AT-commands (PCCA101 + wakeup)";
	extendedClassIDmap[0x020204] = "Communication: Modem: AT-commands (GSM)";
	extendedClassIDmap[0x020205] = "Communication: Modem: AT-commands (3G)";
	extendedClassIDmap[0x020206] = "Communication: Modem: AT-commands (CDMA)";
	extendedClassIDmap[0x0202fe] = "Communication: Modem: Defined by command set descriptor";
	extendedClassIDmap[0x0202ff] = "Communication: Modem: Vendor Specific (MSFT RNDIS?)";
	extendedClassIDmap[0x020300] = "Communication: Telephone";
	extendedClassIDmap[0x020400] = "Communication: Multi-Channel";
	extendedClassIDmap[0x020500] = "Communication: CAPI Control";
	extendedClassIDmap[0x020600] = "Communication: Ethernet Networking";
	extendedClassIDmap[0x020700] = "Communication: ATM Networking";
	extendedClassIDmap[0x020800] = "Communication: Wireless Handset Control";
	extendedClassIDmap[0x020900] = "Communication: Device Management";
	extendedClassIDmap[0x020a00] = "Communication: Mobile Direct Line";
	extendedClassIDmap[0x020b00] = "Communication: OBEX";
	extendedClassIDmap[0x020c00] = "Communication: Ethernet Emulation";
	extendedClassIDmap[0x020c07] = "Communication: Ethernet Emulation (EEM)";
	extendedClassIDmap[0x030001] = "HID: Keyboard";	// HID
	extendedClassIDmap[0x030002] = "HID: Mouse";
	extendedClassIDmap[0x030100] = "HID: Boot Interface Subclass";
	extendedClassIDmap[0x030101] = "HID: Boot Interface: Keyboard";
	extendedClassIDmap[0x030101] = "HID: Boot Interface: Mouse";
	extendedClassIDmap[0x060101] = "Imaging: Picture Transfer Protocol (PIMA 15470)";
	extendedClassIDmap[0x070100] = "Printer: (Reserved/Undefined)";
	extendedClassIDmap[0x070101] = "Printer (Unidirectional)";
	extendedClassIDmap[0x070102] = "Printer (Bidirectional)";
	extendedClassIDmap[0x070103] = "Printer (IEEE 1284.4 compatible bidirectional)";
	extendedClassIDmap[0x0701ff] = "Printer (Vendor Specific)";
	extendedClassIDmap[0x080100] = "Mass Storage: RBC: Control/Bulk/Interrupt";
	extendedClassIDmap[0x080101] = "Mass Storage: RBC: Control/Bulk";
	extendedClassIDmap[0x080150] = "Mass Storage: RBC: Bulk (Zip drive)";
	extendedClassIDmap[0x080200] = "Mass Storage: SFF-8020i, MMC-2 (ATAPI)";
	extendedClassIDmap[0x080300] = "Mass Storage: QIC-157";
	extendedClassIDmap[0x080400] = "Mass Storage: Floppy (UFI): Control/Bulk/Interrupt";
	extendedClassIDmap[0x080401] = "Mass Storage: Floppy (UFI): Control/Bulk";
	extendedClassIDmap[0x080450] = "Mass Storage: Floppy (UFI): Bulk (Zip)";
	extendedClassIDmap[0x080500] = "Mass Storage: SFF-8070i";
	extendedClassIDmap[0x080600] = "Mass Storage: SCSI: Control/Bulk/Interrupt";
	extendedClassIDmap[0x080601] = "Mass Storage: SCSI: Control/Bulk";
	extendedClassIDmap[0x080650] = "Mass Storage: SCSI: Bulk (Zip)";
	extendedClassIDmap[0x090000] = "Hub: Full speed (or root) hub";
	extendedClassIDmap[0x090100] = "Hub: Single TT";
	extendedClassIDmap[0x090200] = "Hub: TT per port";
	extendedClassIDmap[0x0a0030] = "CDC Data: I.430 ISDN BRI";
	extendedClassIDmap[0x0a0031] = "CDC Data: HDLC";
	extendedClassIDmap[0x0a0032] = "CDC Data: Transparent";
	extendedClassIDmap[0x0a0050] = "CDC Data: Q.921M";
	extendedClassIDmap[0x0a0051] = "CDC Data: Q.921";
	extendedClassIDmap[0x0a0052] = "CDC Data: Q.921TM";
	extendedClassIDmap[0x0a0090] = "CDC Data: V.42bis";
	extendedClassIDmap[0x0a0091] = "CDC Data: Q.932 EuroISDN";
	extendedClassIDmap[0x0a0092] = "CDC Data: V.120 V.24 rate ISDN";
	extendedClassIDmap[0x0a0093] = "CDC Data: CAPI 2.0";
	extendedClassIDmap[0x0a00fd] = "CDC Data: Host Based Driver";
	extendedClassIDmap[0x0a00fe] = "CDC Data: CDC PUF";
	extendedClassIDmap[0x0a00ff] = "CDC Data: Vendor specific";
	extendedClassIDmap[0x0e0000] = "Video: Undefined";
	extendedClassIDmap[0x0e0100] = "Video Control";
	extendedClassIDmap[0x0e0200] = "Video Streaming";
	extendedClassIDmap[0x0e0300] = "Video Interface Collection";
	extendedClassIDmap[0x584200] = "Xbox Controller";
	extendedClassIDmap[0xdc0101] = "Diagnostics: Reprogrammable: USB2 Compliance";
	extendedClassIDmap[0xe00101] = "Wireless: Radio Frequency: Bluetooth";
	extendedClassIDmap[0xe00102] = "Wireless: Radio Frequency: Ultra WideBand Radio Control";
	extendedClassIDmap[0xe00103] = "Wireless: Radio Frequency: RNDIS";
	extendedClassIDmap[0xe00201] = "Wireless USB: Host Wire Adapter: Control/Data";
	extendedClassIDmap[0xe00202] = "Wireless USB: Device Wire Adapter: Control/Data";
	extendedClassIDmap[0xe00203] = "Wireless USB: Device Wire Adapter: Isochronous Streaming";
	extendedClassIDmap[0xef0101] = "Misc device: Microsoft ActiveSync";
	extendedClassIDmap[0xef0102] = "Misc device: Palm Sync";
	extendedClassIDmap[0xef0201] = "Misc device: Interface Association";
	extendedClassIDmap[0xef0202] = "Misc device: Wire Adapter Multifunction Peripheral";
	extendedClassIDmap[0xef0301] = "Misc device: Cable Based Association";
	extendedClassIDmap[0xfe0100] = "Application Specific Interface: Device Firmware Update";
	extendedClassIDmap[0xfe0200] = "Application Specific Interface: IRDA Bridge";
	extendedClassIDmap[0xfe0300] = "Application Specific Interface: Test and Measurement";
	extendedClassIDmap[0xfe0301] = "Application Specific Interface: Test and Measurement: TMC";
	extendedClassIDmap[0xfe0302] = "Application Specific Interface: Test and Measurement: USB488";
	extendedClassIDmap[0xffffff] = "Vendor Specific";
}



void USBinfoTables::initAudioTerminalTypesTable() {
	usbAudioTerminalTypesMap[0x0100] = "[I/O] Undefined";
	usbAudioTerminalTypesMap[0x0101] = "[I/O] Streaming";
	usbAudioTerminalTypesMap[0x01ff] = "[I/O] vendor specific";

	usbAudioTerminalTypesMap[0x0200] = "[I] Input undefined";
	usbAudioTerminalTypesMap[0x0201] = "[I] Microphone";
	usbAudioTerminalTypesMap[0x0202] = "[I] Desktop microphone";
	usbAudioTerminalTypesMap[0x0203] = "[I] Personal microphone";
	usbAudioTerminalTypesMap[0x0204] = "[I] Omni-directional microphone";
	usbAudioTerminalTypesMap[0x0205] = "[I] Microphone array";
	usbAudioTerminalTypesMap[0x0206] = "[I] Processing microphone array";

	usbAudioTerminalTypesMap[0x0300] = "[O] Output undefined";
	usbAudioTerminalTypesMap[0x0301] = "[O] Speaker";
	usbAudioTerminalTypesMap[0x0302] = "[O] Headphones";
	usbAudioTerminalTypesMap[0x0303] = "[O] Head mounted display audio";
	usbAudioTerminalTypesMap[0x0304] = "[O] Desktop speaker";
	usbAudioTerminalTypesMap[0x0305] = "[O] Room speaker";
	usbAudioTerminalTypesMap[0x0306] = "[O] Communication speaker";
	usbAudioTerminalTypesMap[0x0307] = "[O] Low frequency effects speaker";

	usbAudioTerminalTypesMap[0x0400] = "[I/O] Bi-directional undefined";
	usbAudioTerminalTypesMap[0x0401] = "[I/O] Handset";
	usbAudioTerminalTypesMap[0x0402] = "[I/O] Headset";
	usbAudioTerminalTypesMap[0x0403] = "[I/O] Speakerphone, w/o echo reduction";
	usbAudioTerminalTypesMap[0x0404] = "[I/O] Speakerphone w/ echo suppression";
	usbAudioTerminalTypesMap[0x0405] = "[I/O] Speakerphone w/ echo cancelation";

	usbAudioTerminalTypesMap[0x0500] = "[I/O] Telephony undefined";
	usbAudioTerminalTypesMap[0x0501] = "[I/O] Phone line";
	usbAudioTerminalTypesMap[0x0502] = "[I/O] Telephone";
	usbAudioTerminalTypesMap[0x0503] = "[I/O] Down Line Phone";

	usbAudioTerminalTypesMap[0x0600] = "[I/O] External undefined";
	usbAudioTerminalTypesMap[0x0601] = "[I/O] Analog connector";
	usbAudioTerminalTypesMap[0x0602] = "[I/O] Digital audio interface";
	usbAudioTerminalTypesMap[0x0603] = "[I/O] Line connector";
	usbAudioTerminalTypesMap[0x0604] = "[I/O] Legacy audio connector";
	usbAudioTerminalTypesMap[0x0605] = "[I/O] S/PDIF interface";
	usbAudioTerminalTypesMap[0x0606] = "[I/O] 1394 DA stream";
	usbAudioTerminalTypesMap[0x0607] = "[I/O] 1394 DV stream soundtrack";

	usbAudioTerminalTypesMap[0x0700] = "[I/O] Embedded undefined";
	usbAudioTerminalTypesMap[0x0701] = "[O] Level calibration noise source";
	usbAudioTerminalTypesMap[0x0702] = "[O] Equalization noise";
	usbAudioTerminalTypesMap[0x0703] = "[I] CD player";
	usbAudioTerminalTypesMap[0x0704] = "[I/O] DAT";
	usbAudioTerminalTypesMap[0x0705] = "[I/O] DCC";
	usbAudioTerminalTypesMap[0x0706] = "[I/O] MiniDisk";
	usbAudioTerminalTypesMap[0x0707] = "[I/O] Analog tape";
	usbAudioTerminalTypesMap[0x0708] = "[I] Phono";
	usbAudioTerminalTypesMap[0x0709] = "[I] VCR Audio";
	usbAudioTerminalTypesMap[0x070a] = "[I] Video Disc Audio";
	usbAudioTerminalTypesMap[0x070b] = "[I] DVD Audio";
	usbAudioTerminalTypesMap[0x070c] = "[I] TV Tuner Audio";
	usbAudioTerminalTypesMap[0x070d] = "[I] Satellite Receiver Audio";
	usbAudioTerminalTypesMap[0x070e] = "[I] Cable Tuner Audio";
	usbAudioTerminalTypesMap[0x070f] = "[I] DSS Audio";
	usbAudioTerminalTypesMap[0x0710] = "[I] Radio Receiver";
	usbAudioTerminalTypesMap[0x0711] = "[O] Radio Transmitter";
	usbAudioTerminalTypesMap[0x0712] = "[I/O] Multi-track Recorder";
	usbAudioTerminalTypesMap[0x0713] = "[I] Synthesizer";
}

const QString USBinfoTables::audioFeatureMapToString( int bmaControl ) {
	QString res;
	if ( bmaControl & 0x01 )
		res += "Mute<br>";
	if ( bmaControl & (0x01 << 1) )
		res += "Volume<br>";
	if ( bmaControl & (0x01 << 2) )
		res += "Bass<br>";
	if ( bmaControl & (0x01 << 3) )
		res += "Mid<br>";
	if ( bmaControl & (0x01 << 4) )
		res += "Treble<br>";
	if ( bmaControl & (0x01 << 5) )
		res += "Graphic Equalizer<br>";
	if ( bmaControl & (0x01 << 6) )
		res += "Automatic Gain<br>";
	if ( bmaControl & (0x01 << 7) )
		res += "Delay<br>";
	if ( bmaControl & (0x01 << 8) )
		res += "Bass Boost<br>";
	if ( bmaControl & (0x01 << 9) )
		res += "Loudness<br>";
	return res;
}
