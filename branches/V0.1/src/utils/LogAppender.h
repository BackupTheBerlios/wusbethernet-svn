/*
 * LogAppender.h
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	16.11.2010
 */

#ifndef LOGAPPENDER_H_
#define LOGAPPENDER_H_

class QString;
class Logger;

class LogAppender {
	friend class Logger;
public:
	virtual ~LogAppender() {};
protected:
	LogAppender() {};
	virtual void write( const QString & text ) = 0;
};

#endif /* LOGAPPENDER_H_ */
