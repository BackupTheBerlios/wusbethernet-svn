/*
 * TI_USBhub.h
 * Abstract interface to define all functions of a network attached USB hub.
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2011-01-04
 */

#ifndef TI_USBHUB_H_
#define TI_USBHUB_H_

#include <QObject>

class Logger;
class QString;


class TI_USBhub : public QObject {
	Q_OBJECT
public:
	TI_USBhub( QObject * parent = 0 ) : QObject( parent ) {};
	virtual ~TI_USBhub() {};

	/**
	 * Return state if this network hub device is still alive or unreachable.
	 * This only takes into account if the hub device is reachable over network and
	 * answers correctly to corresponding requests.
	 * @return State of network hub device
	 */
	virtual bool isAlive() = 0;

	/**
	 * Returns reference to used logger.
	 */
	virtual Logger * getLogger() = 0;


	/**
	 * Debug output: gives brief information about state of device.
	 */
	QString toString();

};

#endif /* TI_USBHUB_H_ */
