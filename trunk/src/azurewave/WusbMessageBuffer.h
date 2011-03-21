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
    	DEVICE_STALL
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

    struct answerMessageParts {
    	bool isCorrect;
    	int senderTAN;
    	int receiverTAN;
    	int TAN;
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
	struct WusbMessageBuffer::answerMessageParts incompleteMessage;
	bool lastMessageWasIncomplete;

	struct WusbMessageBuffer::answerMessageParts splitMessage( const QByteArray & bytes );
	struct WusbMessageBuffer::answerMessageParts splitContinuedMessage( const QByteArray & bytes );
public slots:
	/**
	 * Receive bytes read from network and add message to internal buffering/processing.
	 */
	void receive( const QByteArray & bytes );
signals:
	void statusMessage( WusbMessageBuffer::eTypeOfMessage );
	void urbMessage( QByteArray * );
	void informPacketMeta( int newReceiverTAN, int lastSessionTAN, int packetCounter );
};

#endif /* WUSBMESSAGEBUFFER_H_ */
