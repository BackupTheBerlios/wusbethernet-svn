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

#define WUSB_AZUREWAVE_SEND_HEADER_LEN		28
#define WUSB_AZUREWAVE_SEND_SUBSQ_HEADER_LEN	4
#define WUSB_AZUREWAVE_RECEIVE_HEADER_LEN	24
#define WUSB_AZUREWAVE_TIMER_INTERVAL		75
// maximum size of one network packet (protocol/device does not support bigger packets or fragments!)
#define WUSB_AZUREWAVE_NETWORK_DEFAULT_MTU	1472

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
	bool sendURB( const char * urbData, int urbDataLen,
			eDataTransferType dataTransferType, eDataDirectionType directionType,
			uint8_t endpoint, uint16_t transferFlags,
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
	bool sendURB( QByteArray * urbData,
			eDataTransferType dataTransferType, eDataDirectionType directionType,
			uint8_t endpoint, uint16_t transferFlags,
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

	bool openSocket();
	bool writeToSocket( const QByteArray & buffer );
	bool openDevice();
	bool closeDevice();
	const QString stateToString();
	void stopTimer();

	/** Send idle / keepalive message to device  */
	bool sendIdleMessage();

private slots:
	/** Receive routine to read data on UDP socket. Will be called upon signal from QT socket stack. */
	void processPendingData();
	void socketError(QAbstractSocket::SocketError socketError);
	void processStatusMessage( WusbMessageBuffer::eTypeOfMessage typeMsg );
	void processURBmessage( QByteArray * urbBytes );
	void informReceivedPacket( int newReceiverTAN, int lastSessionTAN, int packetCounter );
	void timerInterrupt();
	virtual void processURB(	void * refData, uint16_t transferFlags, uint8_t endPointNo,
			TI_WusbStack::eDataTransferType transferType, TI_WusbStack::eDataDirectionType dDirection,
			QByteArray * urbData, int expectedReceiveLength );
signals:
	void receivedUDPdata(const QByteArray &);
	void receivedURB( const QByteArray &);
};

#endif /* WUSBSTACK_H_ */
