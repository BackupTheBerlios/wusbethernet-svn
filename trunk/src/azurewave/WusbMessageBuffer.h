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
    enum TypeOfMessage {
    	DEVICE_OPEN_SUCCESS,
    	DEVICE_CLOSE_SUCCESS,
    	DEVICE_ALIVE,
    };


	WusbMessageBuffer( WusbStack * owner );
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
    	unsigned int packetNum;
    	bool isComplete;
    	bool transactionCompleted;
    	QByteArray * contentURB;
    };
	WusbStack * parentRef;
	Logger * logger;
	QByteArray * internalBuffer;
	QByteArray lastReceivedPacket;
	int bytesLeftInCurrentTask;
	struct WusbMessageBuffer::answerMessageParts splitMessage( const QByteArray & bytes );
public slots:
	/**
	 * Receive bytes read from network and add message to internal buffering/processing.
	 */
	void receive( const QByteArray & bytes );
signals:
	void statusMessage( WusbMessageBuffer::TypeOfMessage );
	void urbMessage( QByteArray * );
	void informPacketMeta( int newReceiverTAN, int lastSessionTAN, int packetCounter );
};

#endif /* WUSBMESSAGEBUFFER_H_ */
