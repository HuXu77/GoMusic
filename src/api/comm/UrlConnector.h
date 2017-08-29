/*
 * UrlConnector.h
 *
 *  Created on: Nov 29, 2012
 *      Author: nmmrc18
 */

#ifndef URLCONNECTOR_H_
#define URLCONNECTOR_H_

#include <QObject>
#include <QFile>
#include <QtNetwork/QNetworkAccessManager.h>
#include <QtNetwork/QNetworkRequest.h>
#include <QtNetwork/QNetworkReply.h>
#include <QtNetwork/QSslconfiguration.h>
#include <QtNetwork/qnetworkcookiejar.h>
#include <QtNetwork/qnetworkcookie.h>
#include <QDebug>
#include "FormBuilder.h"

class QNetworkAccessManager;

class UrlConnector : public QObject {
	Q_OBJECT
public:
	UrlConnector();
	virtual ~UrlConnector();
	static const QString COOKIE_FORMAT;
	static const QString COOKIE_FORMAT_RADIO;
	static const QString LOGIN_AUTH_KEY;
	static const QString LOGIN_AUTH_VALUE;
	static const QString SERVICE_URL;
	static const QString RADIO_URL;

	void get(const QString &address);
	void post(QString address, FormBuilder *form);

	bool bytes;
	int cacheNum;
	QFile *fileToWriteTo;
	qint64 startingByte;
	qint64 endingByte;

public Q_SLOTS:
	void cancelRequest();
	void sendDownloadProgress(qint64,qint64);
	void onError(QNetworkReply::NetworkError);
	void readyRead();

private Q_SLOTS:
	void onGetReply();

Q_SIGNALS:
	// used for login
	void complete();
	// used for subsequent calls for JSON stuff
	void complete(QString string);
	void complete(QByteArray array);

	void failed();
	void networkError(QString title, QString body);

	void downloadProgress(qint64,qint64);

	void jsonDump(QString);

private:
	QNetworkAccessManager *networkAccessManager;
	QString *xtcookie;
	QString *masterAuth;
	QString *authToken;
	QString currentUrl;

	QNetworkReply *currentReply;

	//QFile *temp;

	void addHeaders(QNetworkRequest*, FormBuilder*, bool);
};

#endif /* URLCONNECTOR_H_ */
