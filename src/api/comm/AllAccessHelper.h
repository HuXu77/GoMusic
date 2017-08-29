/*
 * AllAccessHelper.h
 *
 *  Created on: Jul 2, 2013
 *      Author: nmmrc18
 */

#ifndef ALLACCESSHELPER_H_
#define ALLACCESSHELPER_H_

#include <QtCore/QObject>
#include "humac.h"
#include "husha1.h"

class AllAccessHelper : public QObject {
	Q_OBJECT
public:
	AllAccessHelper();
	virtual ~AllAccessHelper();
	QString getSalt();
	void getSig(QString songIdSalt, unsigned char* message);

private:
	static const QString ALPHANUM_LOWERCASE;
	static const QString AA_KEY;

	static const QString S1;
	static const QString S2;

	sb_GlobalCtx globalContext;
};

#endif /* ALLACCESSHELPER_H_ */
