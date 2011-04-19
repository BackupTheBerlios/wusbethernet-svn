/*
 * WusbMessageBuffer.h
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2010-10-09
 */

#ifndef WUSBMESSAGEBUFFER_H_
#define WUSBMESSAGEBUFFER_H_

#include <QThread>
#include <QString>
#include <QByteArray>
#include <stdint.h>

class WusbStack;
class Logger;

class WusbMessageBuffer : public QThread {
	Q_OBJECT
public:
	/** Type of message given by header (read from network) */
    enum eTypeOfMessage {
    	DEVICE_OPEN_SUCCESS,
    	DEVICE_CLOSE_SUCCESS,
    	DEVICE_ALIVE,
    	DEVICE_STALL,
    	DEVICE_RECEIVED_DUP
    };


	WusbMessageBuffer( WusbStack * owner, int mtuSize = 1500 );
	~WusbMessageBuffer();


	/**
	 * Thread run loop.<br>
	 * For technical reasons this method must be declared <em>public</em>...
	 */
	void run();

private:
	static bool isFirstInstance;

    struct sAnswerMessageParts {
		bool slotInUse;
    	bool isCorrect;
    	uint8_t senderTAN;
    	uint8_t receiverTAN;
    	uint8_t TAN;
    	unsigned int packetNum;		// packet counter
    	int contentLength;	// length of contentURB
    	bool isComplete;
    	bool transactionCompleted;
    	QByteArray * contentURB;
    };

	WusbStack * parentRef;
	Logger * logger;
	QByteArray * internalBuffer;
	QByteArray * lastReceivedPacket;
	int bytesLeftInCurrentTask;
//	struct WusbMessageBuffer::sAnswerMessageParts incompleteMessage;
//	bool lastMessageWasIncomplete;
	bool haveIncompleMessages;
	struct WusbMessageBuffer::sAnswerMessageParts * incompleteMessages;


	struct WusbMessageBuffer::sAnswerMessageParts splitMessage( const QByteArray & bytes );
	struct WusbMessageBuffer::sAnswerMessageParts splitContinuedMessage(
			const QByteArray & bytes, const WusbMessageBuffer::sAnswerMessageParts & prevMessageDesc );
public slots:
	/**
	 * Receive bytes read from network and add message to internal buffering/processing.
	 */
	void receive( const QByteArray & bytes );
signals:
	/** Status changed (or an event occured) on/in connection */
	void statusMessage( WusbMessageBuffer::eTypeOfMessage, uint8_t, uint8_t, uint8_t );
	/** An URB was completely received and can be processed */
	void urbMessage( unsigned int packetID, QByteArray * urbData );
	void informPacketMeta( int newReceiverTAN, int lastSessionTAN, unsigned int packetCounter );
};

#endif /* WUSBMESSAGEBUFFER_H_ */
