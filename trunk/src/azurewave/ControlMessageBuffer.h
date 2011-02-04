/**
 * ControlMessageBuffer.h
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2010-07-14
 */

#ifndef CONTROLMESSAGEBUFFER_H_
#define CONTROLMESSAGEBUFFER_H_

#include <QString>

#define DEFAULT_BUFFER_SIZE (1024 * 64)

class HubDevice;
class QByteArray;
class Logger;

class ControlMessageBuffer {
public:
	/** Type of message given by header (read from network) */
	enum TypeOfMessage {
		TOM_NOP = 0,
		TOM_ALIVE,
		TOM_SERVERINFO,
		TOM_STATUSCHANGED,
		TOM_IMPORTINFO,
		TOM_UNIMPORT,
		TOM_MSG66
	};

	ControlMessageBuffer( HubDevice * recipientClass );
	~ControlMessageBuffer();

	/**
	 * Receive bytes read from network and add to internal buffering.
	 */
	void receive( const QByteArray & bytes );
private:
	Logger * logger;
	HubDevice * recipientForData;
	QByteArray * internalBuffer;
	int bytesLeftInCurrentTask;
	TypeOfMessage currentMessageTask;

	TypeOfMessage getTypeOfMessage( const QByteArray & bytes );
	int getLengthOfMessage( const QByteArray & bytes );
	void retrieveMessageLengthAndType();
	QString messageToString( const QByteArray & bytes, int lengthToPrint = 6 );
	QString typeOfMessageToString( ControlMessageBuffer::TypeOfMessage type );
};

#endif /* CONTROLMESSAGEBUFFER_H_ */
