/*
 * BasicUtils.h
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2010-10-19
 */

#ifndef BASICUTILS_H_
#define BASICUTILS_H_

#include <QString>

extern void sleepThread( long millis );
extern long long currentTimeMillis();
/**
 * <em>Flips</em> an IP-address from little-endian to big-endian notation and v.v.
 * @parm	IP-address in form <tt>WW.XX.YY.ZZ</tt>
 * @return	IP-address in form <tt>ZZ.YY.XX.WW</tt> or <ttQString::null</tt> if input was NULL or empty
 */
extern QString flipIPaddress( const QString & address );

/** Debug function: Print at most <em>length</em> bytes in hex notation into result string. */
QString messageToString( const QByteArray & bytes, int lengthToPrint = 4 );
/** Debug function: Print at most <em>length</em> bytes in hex notation into result string. */
QString messageToString( const unsigned char* bytes, int lengthToPrint = 1 );


#endif /* BASICUTILS_H_ */
