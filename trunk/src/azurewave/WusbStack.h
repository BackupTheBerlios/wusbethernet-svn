/*
 * WusbStack.h
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2010-09-28
 */

#ifndef WUSBSTACK_H_
#define WUSBSTACK_H_

#include "../TI_WusbStack.h"
#include "WusbMessageBuffer.h"
#include <QObject>
#include <QHostAddress>
#include <QLinkedList>
#include <QQueue>
#include <QHash>
#include <QMutex>
#include <QAbstractSocket>

class QUdpSocket;
class QByteArray;
class WusbReceiverThread;
class WusbMessageBuffer;
class QTimer;
class Logger;
class USBconnectionWorker;
class TI_USB_VHCI;

/** Header length of a regular URB packet */
#define WUSB_AZUREWAVE_SEND_HEADER_LEN			28
/** Header length of a continued packet (packet bigger than MTU) */
#define WUSB_AZUREWAVE_SEND_SUBSQ_HEADER_LEN	4
/** Header length of a received packet (answer packet from hub) */
#define WUSB_AZUREWAVE_RECEIVE_HEADER_LEN		24
/** Basic timer interval to check for retransmit or to send ack/idle/keep alive messages */
#define WUSB_AZUREWAVE_TIMER_INTERVAL			50L
/** maximum size of one network packet (protocol/device does not support bigger packets or fragments!) */
#define WUSB_AZUREWAVE_NETWORK_DEFAULT_MTU		1472
/** after this time a ACK is send to hub when no further packets (URB) are ready to send  */
#define WUSB_AZUREWAVE_TIMER_SEND_ACK			100L
/** after this time of idle running a KEEP ALIVE message is send to hub */
#define WUSB_AZUREWAVE_TIMER_SEND_KEEP_ALIVE	2000L

class WusbStack : public TI_WusbStack {
	Q_OBJECT
friend class WusbReceiverThread;
public:
	WusbStack( Logger *parentLogger, const QHostAddress & destinationIP, int destinationPort );
	virtual ~WusbStack();

	/**
	 * Open connection to device
	 * @return <code>true</code> if no fatal errors (socket open, network error etc) occur.
	 */
	bool openConnection();
	/**
	 * Closing connection to device and end up (finalize) network connection to device
	 * @return <code>true</code> if no fatal errors occur and connection is active.
	 */
	bool closeConnection();
	/**
	 * Send USB request block (<em>URB</em>) to device. The URB is wrapped with
	 * headers and if necessary broken into smaller pieces for transport.<br>
	 * PRE: network connection to device initialized and not in error state.
	 * @param	urbData				URB data (raw data, NOT 0x0 terminated)
	 * @param	urbDataLen			Length of urb data
	 * @param	dataTransferType	Transfer type of URB
	 * @param	directionType		Direction of data transfer
	 * @parma	endpoint			Endpoint / Recipient
	 * @param	receiveLength		Length (in bytes) of expected respond from device
	 * @return	<code>true</code> if no fatal errors occur and connection is active.
	 */
	bool sendURB( void * refData, const char * urbData, int urbDataLen,
			eDataTransferType dataTransferType, eDataDirectionType directionType,
			uint8_t endpoint, uint16_t transferFlags,
			uint8_t intervalVal,
			int receiveLength = 0);
	/**
	 * Send USB request block (<em>URB</em>) to device. The URB is wrapped with
	 * headers and if necessary broken into smaller pieces for transport.<br>
	 * PRE: network connection to device initialized and not in error state.
	 * @param	urbData
	 * @param	dataTransferType	Transfer type of URB
	 * @param	directionType		Direction of data transfer
	 * @parma	endpoint			Endpoint / Recipient
	 * @param	receiveLength		Length (in bytes) of expected respond from device
	 * @return	<code>true</code> if no fatal errors occur and connection is active.
	 */
	bool sendURB( void * refData, QByteArray * urbData,
			eDataTransferType dataTransferType, eDataDirectionType directionType,
			uint8_t endpoint, uint16_t transferFlags,
			uint8_t intervalVal,
			int receiveLength = 0);


	/**
	 * Registers an URB receiver object.
	 * This object will get all URB replys from network hub instead of sending per signal.
	 */
	virtual void registerURBreceiver( TI_USB_VHCI * urbSink );

	/**
	 * Returns reference to logger.
	 */
	Logger * getLogger();


private:
	static bool isFirstInstance;

	eStackState state;
	QHostAddress destAddress;
	int destPort;
	QUdpSocket *udpSocket;
	bool haveAnswer;
	QLinkedList<QByteArray*> receiveBuffer;
	QQueue<const QByteArray*> sendBuffer;
	QMutex receiveBufferMutex;
	QMutex sendBufferMutex;
	int maxMTU;
	int currentSendTransactionNum;
	int currentReceiveTransactionNum;
	int currentTransactionNum;
	int sendPacketCounter;

	/** Timestamp: Last packet sent */
	long long lastPacketSendTimeMillis;
	/** Timestamp: Last packet received */
	long long lastPacketReceiveTimeMillis;
	/** Timestamp: Last keep-alive packet sent */
	long long lastSendAlivePacket;

	WusbReceiverThread *receiverThread;
	WusbMessageBuffer *messageBuffer;

	Logger * logger;	// ref to logger

	QTimer * connectionKeeperTimer;

	/** Receiver of URBs from network hub */
	TI_USB_VHCI * urbReceiver;
	/** Reference data for last data packet sent */
	void * packetRefData;
	QHash<unsigned int, void*> packetRefDataByPacketID;

	bool openSocket();
	bool writeToSocket( const QByteArray & buffer );
	bool openDevice();
	bool closeDevice();
	const QString stateToString();
	void stopTimer();

	/** Send idle / keepalive message to device  */
	bool sendIdleMessage( uint8_t sendTAN = 0, uint8_t recTAN = 0, uint8_t tan = 0 );
	/** Send receive acknowledge message */
	void sendAcknowledgeReplyMessage();
private slots:
	/** Receive routine to read data on UDP socket. Will be called upon signal from QT socket stack. */
	void processPendingData();
	/** Signal from socket that an uncorrectable (on socket layer) error occured */
	void socketError(QAbstractSocket::SocketError socketError);
	/** Status of connection changed or some special packet received */
	void processStatusMessage( WusbMessageBuffer::eTypeOfMessage typeMsg, uint8_t, uint8_t, uint8_t );
	/** Receive of a URB pacekt from network */
	void processURBmessage( unsigned int packetID, QByteArray * urbBytes );
	void informReceivedPacket( int newReceiverTAN, int lastSessionTAN, unsigned int packetCounter );
	void timerInterrupt();
	virtual void processURB( void * refData, uint16_t transferFlags, uint8_t endPointNo,
			TI_WusbStack::eDataTransferType transferType, TI_WusbStack::eDataDirectionType dDirection,
			QByteArray * urbData, uint8_t intervalVal, int expectedReceiveLength );
signals:
	void receivedUDPdata(const QByteArray &);
	void receivedURB( const QByteArray &);
};

#endif /* WUSBSTACK_H_ */
