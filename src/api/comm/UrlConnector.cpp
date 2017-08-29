/*
 * UrlConnector.cpp
 *
 *  Created on: Nov 29, 2012
 *      Author: nmmrc18
 */

#include "UrlConnector.h"
#include "GoogleMusicApi.h"
#include <QSettings>
#include <bb/device/HardwareInfo>
#include <time.h>
#include <bb/data/JsonDataAccess>

const QString UrlConnector::COOKIE_FORMAT = QString("?u=0&xt=%1");
const QString UrlConnector::COOKIE_FORMAT_RADIO = QString("?u=0&xt=%1&format=jsarray");
const QString UrlConnector::LOGIN_AUTH_KEY = QString("Authorization");
const QString UrlConnector::LOGIN_AUTH_VALUE = QString("GoogleLogin auth=%1");
const QString UrlConnector::SERVICE_URL = QString("https://play.google.com/music/services/");
const QString UrlConnector::RADIO_URL = QString("https://www.googleapis.com/sj/v1/radio/station");

using namespace bb::device;
using namespace bb::data;

UrlConnector::UrlConnector() :
	QObject() {
	networkAccessManager = new QNetworkAccessManager(this);
	QSettings settings;
	if (settings.value(GoogleMusicApi::AUTHTOKEN).isNull()) {
		authToken = new QString("");
	} else {
		authToken = new QString(settings.value(GoogleMusicApi::AUTHTOKEN).toString());
	}
	if (!settings.value("MASTER_AUTH").isNull()) {
		masterAuth = new QString(settings.value(QString("MASTER_AUTH")).toString());
	} else {
	    masterAuth = new QString("");
	}
	bytes = false;
	cacheNum = 0;
	endingByte = 0;
}

void UrlConnector::post(QString address, FormBuilder *form) {
	currentUrl = QString(address);

	QNetworkRequest request = QNetworkRequest(QUrl(address));
	addHeaders(&request, form, !masterAuth->isEmpty());

	if (currentUrl.indexOf(UrlConnector::SERVICE_URL) != -1) {
		QString cookieAppend;
		if (currentUrl.indexOf(UrlConnector::RADIO_URL) != -1) {
			cookieAppend = QString(COOKIE_FORMAT_RADIO.arg(*xtcookie));
		} else {
			cookieAppend = QString(COOKIE_FORMAT.arg(*xtcookie));
		}
		address.append(cookieAppend);
		request.setUrl(QUrl(address));
	}

	qDebug() << "GMA: Sending request to: " << request.url().toString();

	QByteArray byte = form->outputStream.toAscii();

	qDebug() << "GMA: Request Body = " << form->outputStream;

	/*QDir tempStorage(QString("app/native/temp/"));
	if (!tempStorage.exists()) {
		QDir::root().mkpath(tempStorage.absolutePath());
	}

	QString tempName = QString("app/native/temp/cache%1").arg(cacheNum);
	temp = new QFile(tempName);
	while (temp->exists()) {
		tempName = QString("app/native/temp/cache%1").arg(cacheNum++);
		temp = new QFile(tempName);
	}
	temp->open(QIODevice::WriteOnly);
	temp->write("");
	temp->close();*/

	currentReply = networkAccessManager->post(request, byte);
	connect(currentReply, SIGNAL(finished()), this, SLOT(onGetReply()), Qt::UniqueConnection);
	connect(currentReply, SIGNAL(downloadProgress(qint64,qint64)), this,
						SLOT(sendDownloadProgress(qint64,qint64)));
	connect(currentReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onError(QNetworkReply::NetworkError)), Qt::UniqueConnection);

	/*Q_FOREACH(QByteArray header, reply->request().rawHeaderList()) {
		qDebug() << "Request Header: " << QString(header) << ": " << QString(request.rawHeader(header));
	}*/

}

void UrlConnector::get(const QString &address) {
	QNetworkRequest request = QNetworkRequest(QUrl(address));
	//FormBuilder *form = new FormBuilder(true);
	//form->close();
	addHeaders(&request, NULL, true);

	QDir tempStorage(QString(QDir::homePath() + "/temp/"));
	if (!tempStorage.exists()) {
		QDir::root().mkpath(tempStorage.absolutePath());
	}

	//qDebug() << "GMA: Sending request to: " << address;

	currentReply = networkAccessManager->get(request);

	/*QString tempName = QString("app/native/temp/cache%1").arg(cacheNum);
	temp = new QFile(tempName);
	while (temp->exists()) {
		tempName = QString("app/native/temp/cache%1").arg(cacheNum++);
		temp = new QFile(tempName);
	}
	temp->open(QIODevice::WriteOnly);
	temp->write("");
	temp->close();*/

	connect(currentReply, SIGNAL(downloadProgress(qint64,qint64)), this,
							SLOT(sendDownloadProgress(qint64,qint64)), Qt::UniqueConnection);
	if (bytes) {
		connect(currentReply, SIGNAL(readyRead()), this, SLOT(readyRead()), Qt::UniqueConnection);
	} else {
		connect(currentReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onError(QNetworkReply::NetworkError)), Qt::UniqueConnection);
	}
	//
	connect(currentReply, SIGNAL(finished()), this, SLOT(onGetReply()), Qt::UniqueConnection);
}

void UrlConnector::readyRead() {
	if (currentReply) {
		if (currentReply->error() == QNetworkReply::NoError) {
			QList<QByteArray> headers = currentReply->rawHeaderList();
			if (fileToWriteTo && endingByte == 0) {
				// this is the first data we receive
				QByteArray finalSize = currentReply->rawHeader(QString("Content-Length").toAscii());
				qint64 currSize = fileToWriteTo->size();
				qint64 finSize = finalSize.toLongLong();
				if (currSize == finSize) {
					currentReply->close();
					emit complete(finalSize);
					return;
				} else if (currSize > 0) {
					// this means it was an incomplete download and needs to be removed before we write to it
					fileToWriteTo->remove();
				} else {
					// never been downloaded and need to download it
				}
				endingByte = 1;
				if (!fileToWriteTo->open(QIODevice::WriteOnly | QIODevice::Append)) {
					qDebug() << "GMA: Unable to write to cache";
					return;
				}
				QByteArray data = currentReply->readAll();

				fileToWriteTo->write(data);
				fileToWriteTo->close();
			} else if (fileToWriteTo && endingByte == 1) {
				if (!fileToWriteTo->open(QIODevice::WriteOnly | QIODevice::Append)) {
					qDebug() << "GMA: Unable to write to cache";
					return;
				}
				QByteArray data = currentReply->readAll();
				fileToWriteTo->write(data);
				fileToWriteTo->close();
			}
		}
	}
}

void UrlConnector::onGetReply() {
	QObject *r = sender();
	QNetworkReply *reply = qobject_cast<QNetworkReply*>(r);
	qDebug() << "GMA: Request complete: " << reply->request().url().toString();
	/*QVariant cookiesvariant = reply->header(QNetworkRequest::SetCookieHeader);
	if (!cookiesvariant.isNull()) {
		QSettings settings;
		bool toComplete = cookies.size() == 0;
		cookies = qvariant_cast<QList<QNetworkCookie> >(cookiesvariant);
		QByteArray cookieBytes;
		Q_FOREACH(QNetworkCookie cook, cookies) {
			QString name(cook.name());
			cookieBytes.append(cook.toRawForm());
			cookieBytes.append("\n");
			if (name.compare(QString("xt")) == 0) {
				xtcookie = new QString(cook.value());
				//qDebug() << "GMA: Saving XTCookie...";
				settings.setValue(QString("xtcookie"), QString(cook.value()));
			}
		}
		if (toComplete) {
			emit complete();
		}
	}*/

	QVariant redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
	QByteArray data = reply->readAll();
	qDebug() << "WHAT: " << QString(data);
	if (!redirectUrl.isNull()) {
		//qDebug() << QString("Redirect URL: %1").arg(redirectUrl.toString());
		//get(redirectUrl.toString());
		complete(QString("{ \"url\" : \"%1\" }").arg(redirectUrl.toString()));
		return;
	}

	/*QList<QByteArray> headers = reply->rawHeaderList();
	Q_FOREACH(QByteArray head, headers) {
		QByteArray value = reply->rawHeader(head);
		qDebug() << "GMA HEADER: " << QString(head) << ": " << QString(value);
	}*/

	QString response;
	if (reply->error() == QNetworkReply::NoError) {
		QSettings settings;
		if (authToken->isEmpty()) {
			//temp->open(QIODevice::ReadOnly);
			//emit jsonDump("Login success, found auth token requesting cookies...");
			QString rawResponse(data);
			//rawResponse = rawResponse.trimmed().simplified();
			int start = rawResponse.indexOf("Token=") + 6;
			int end = rawResponse.indexOf('\n', start) - start;

			//JsonDataAccess jda;
			//QVariant jsonVar = jda.loadFromBuffer(rawResponse);
			//QVariantMap jsonMap = jsonVar.toMap();

			QString token = QString(rawResponse.mid(start, end)).trimmed().simplified();

			qDebug() << "EGGS TOKEN!! " << token << " Blag " << token.length() << " Start " << start << " End " << end;

			//qDebug() << "GMA: Raw: " << rawResponse;

			//authToken = new QString(tmp.trimmed().simplified());
			//qDebug() << "GMA: Setting authtoken... " << authToken;
			settings.setValue(GoogleMusicApi::AUTHTOKEN, token);
			authToken = new QString(token);

			emit complete(token);

			/*tmp = QString(rawResponse.mid(start, end));
			QString sid = QUrl::toPercentEncoding(tmp);

			start = rawResponse.indexOf("LSID=") + 4;
			end = rawResponse.indexOf("\n", start);

			tmp = QString(rawResponse.mid(start, end));

			QString lsid = QUrl::toPercentEncoding(tmp);

			//QString url = QString(GoogleMusicApi::ISSUE_AUTH).arg(sid).arg(lsid);
			QString url = QString(GoogleMusicApi::PLAY_LISTEN);

			// Because we didn't have an auth token, we need to get cookies for
			// it
			QNetworkRequest request = QNetworkRequest(QUrl(url));
			FormBuilder *form = new FormBuilder(true);
			form->close();

			addHeaders(&request, form, false);

			//temp->remove();
			currentReply = NULL;
			currentReply = networkAccessManager->post(request, form->outputStream.toAscii());
			//qDebug() << "GMA: Requesting cookies...";
			connect(currentReply, SIGNAL(finished()), this, SLOT(onGetReply()));
			connect(currentReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onError(QNetworkReply::NetworkError)));
		    */
		} else if (masterAuth->isEmpty()) {
			//qDebug() << "GMA: Setting cookies...";
			//emit jsonDump("Got cookies, login complete");

			/*QVariant cookiesvariant = reply->header(QNetworkRequest::SetCookieHeader);
			cookies = qvariant_cast<QList<QNetworkCookie> >(cookiesvariant);
			QByteArray cookieBytes;
			Q_FOREACH(QNetworkCookie cook, cookies) {
				QString name(cook.name());
				cookieBytes.append(cook.toRawForm());
				cookieBytes.append("\n");
				if (name.compare(QString("xt")) == 0) {
					xtcookie = new QString(cook.value());
					//qDebug() << "GMA: Saving XTCookie...";
					settings.setValue(QString("xtcookie"), QString(cook.value()));
				}
			}

			settings.setValue("cookies", cookieBytes);
			QString poop(reply->readAll());
			//qDebug() << "GMA: Complete String: " << poop;*/
		    QString rawResponse(data);
		    /*JsonDataAccess jda;
		    QVariant jsonVar = jda.loadFromBuffer(rawResponse);
		    QVariantMap jsonMap = jsonVar.toMap();
		    QString token = jsonMap.value("Auth").toString();*/
		    int start = rawResponse.indexOf("Auth=") + 5;
		    int end = rawResponse.indexOf("\n", start) - start;
		    QString token = rawResponse.mid(start, end);
		    masterAuth = new QString(token);
		    settings.setValue("MASTER_AUTH", token);

			currentReply->deleteLater();
			emit complete(rawResponse);
			emit complete();
		} else {
			if (bytes) {
				//temp->open(QIODevice::ReadOnly);

				/*if (endingByte != 0) {
					if (fileToWriteTo) {
						int size = arr.size();
						//qDebug() << "GMA: Size of segment: " << size;
						if (size == 0) {
							QList<QByteArray> headers = reply->rawHeaderList();
							Q_FOREACH(QByteArray head, headers) {
								QByteArray value = reply->rawHeader(head);
								qDebug() << "GMA HEADER: " << QString(head) << ": " << QString(value);
							}
							//qDebug() << "GMA: Body: " << QString(reply->readAll());
							emit networkError("Network Error", "There was an issue downloading the song, please notify the developer you got this message.  Message 25.");
							return;
						} else {
							if (!fileToWriteTo->isOpen()) {
								fileToWriteTo->open(QIODevice::WriteOnly);
							}
							if (startingByte != 0) {
								if (fileToWriteTo->seek(startingByte)) {
								} else {
									qDebug() << "GMA: Unable to seek";
								}
							}
							fileToWriteTo->write(arr);
						}
					}
				}*/
				/*if (fileToWriteTo) {
					fileToWriteTo.open(QIODevice::WriteOnly | QIODevice::Append);
					fileToWriteTo.write(arr);
					fileToWriteTo.close();
				} else {*/
					emit complete(data);
				//}
				//QFileInfo fileInfo(temp->fileName());
				//qDebug() << "GMA: Removing file: " << fileInfo.fileName();
				//temp->remove();
			} else {
				QString poop(data);
				//qDebug() << "GMA: Complete String: " << poop;

				/*QDir tempStorage("app/native/temp/");
				if (!tempStorage.exists()) {
					QDir::root().mkpath(tempStorage.absolutePath());
				}
				QFile temp("app/native/temp/stuffies");
				temp.open(QIODevice::WriteOnly | QIODevice::Append);
				temp.write(poop.toAscii());
				temp.close();
*/
				//temp->open(QIODevice::ReadOnly);
				emit complete(poop);
				//QFileInfo fileInfo(temp->fileName());
				//qDebug() << "GMA: Removing file: " << fileInfo.fileName();
				//temp->remove();
			}
		}
		/*
		 * For debugging headers
		Q_FOREACH(QByteArray header, reply->request().rawHeaderList()) {
			qDebug() << "Request Header: " << QString(header) << ": " << QString(reply->request().rawHeader(header));
		}
		Q_FOREACH(QByteArray header, reply->rawHeaderList()) {
			qDebug() << "Reply Header: " << QString(header) << ": " << reply->rawHeader(header);
		}*/
	} else {
		// login failed
		response = tr("Error: %1 status: %2").arg(reply->errorString(), reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString());
		qDebug() << response;
	}

	currentReply = NULL;
	reply->deleteLater();
}

void UrlConnector::addHeaders(QNetworkRequest *request, FormBuilder *form, bool addContentType) {
	// Default headers
	//request->setHeader(QNetworkRequest::ContentTypeHeader, form->contentType);
	request->setRawHeader(QString("Accept").toAscii(), QString("text/html, image/gif, image/jpeg, *; q=.2, */*; q=.2").toAscii());
	request->setRawHeader(QString("User-Agent").toAscii(), QString("Android-Music/5106").toAscii());
	HardwareInfo *info = new HardwareInfo(this);
	QString pin = info->pin().mid(info->pin().indexOf("0x")+2, 8);
	QString id = QString("24f1bb1fa9e2a3e9");//.arg(info->pin()).toLower();
	request->setRawHeader(QString("X-Device-ID").toAscii(), id.toLower().toAscii());
	//request->setRawHeader(QString("X-Device-Logging-ID").toAscii(), QString("15447e918328e65f").toAscii());
	// if radio
	if (addContentType) {
		request->setRawHeader(QString("Content-Type").toAscii(), QString("application/json").toAscii());
	} else {
	    request->setRawHeader(QString("Content-Type").toAscii(), QString("application/x-www-form-urlencoded").toAscii());
	}
	request->setRawHeader(QString("Connection").toAscii(), QString("keep-alive").toAscii());

	// auth token
	if (!masterAuth->isEmpty()) {
		QString authTokened = LOGIN_AUTH_VALUE.arg(*masterAuth);
		//qDebug() << "GMA: Auth token: " << authTokened;
		request->setRawHeader(LOGIN_AUTH_KEY.toAscii(), authTokened.toAscii());
	}

	// SSL Config
	QSslConfiguration config = request->sslConfiguration();
	config.setPeerVerifyMode(QSslSocket::VerifyNone);
	config.setProtocol(QSsl::TlsV1);
	request->setSslConfiguration(config);
}

void UrlConnector::sendDownloadProgress(qint64 curSize, qint64 actualSize) {
	//qDebug() << "GMA: Download: " << curSize << "/" << actualSize;
	emit downloadProgress(curSize, actualSize);
}

void UrlConnector::onError(QNetworkReply::NetworkError error) {
	QObject *temp = sender();
	QNetworkReply *reply = qobject_cast<QNetworkReply*>(temp);
	qDebug() << "GMA: Error connecting: " << error;
	QString errorMessage("");
	QString title("");
	if (error == 3 || error == 2) {
		// unable to reach host
		title = QString("Download Failed");
		errorMessage = QString("Unable to reach Google, please check your network settings and try again.");
	} else if (error == 202 || error == 204) {
		// unauthorized
		// If error 401 then we need to reauthenticate
		QString url = reply->request().url().toString();
		if (error == 204) {
			title = "Refresh Login Details: 401";
			errorMessage = "Google requires that you refresh your login details";
		} else if (url.contains("loadalltracks")) {
			title = QString("Google Music Not Setup");
			errorMessage = QString("Please go to music.google.com and setup Google Music with the library you want to sync.");
		} else {
			title = QString("Login Failed: %1").arg(error);
			errorMessage = QString("Login failed due to invalid username or password.  If you use 2 factor authentication, you will need to generate an app specific password to use this app.");
		}
	} else if (error == 99) {
		title = QString("Connection Issue");
		errorMessage = QString("There was an error connecting to Google.  Check that you have a strong connection signal.");
	} else if (error == 203) {
		title = QString("All Access Feature");
		errorMessage = QString("This is an All Access feature.  You can sign up for All Access at play.google.com/about/music.");
	} else {
		title = QString("Connection Error");
		errorMessage = QString("Error when connecting, error code: %1").arg(error);
	}
	QList<QByteArray> headers = reply->rawHeaderList();
	//errorMessage = "";
	Q_FOREACH(QByteArray head, headers) {
		QByteArray value = reply->rawHeader(head);
		qDebug() << "GMA HEADER: " << QString(head) << ": " << QString(value);
		if (QString(head).startsWith("Update-Client-Auth")) {
			authToken = new QString(value);
			QSettings settings;
			settings.setValue(GoogleMusicApi::AUTHTOKEN, *authToken);
			get(GoogleMusicApi::PLAY_LISTEN);
			return;
		}

		if (QString(head).startsWith("X-Rejected-Reason")) {
			if (QString(value).startsWith("DEVICE_NOT_")) {
				title = "Too Many Registered Devices";
				errorMessage = "Go to Google Music on a web browser and click the gear icon, select Music Settings and deauthorize devices you don't use";
				break;
			} else if (QString(value) == "TRACK_NOT_FOUND") {
				title = "Invalid Track ID";
				errorMessage = "Tell the developer you got this error message, the information about this song is wrong";
				break;
			} else if (QString(value) == "ANOTHER_STREAM_BEING_PLAYED") {
			    title = "Another Device Playing Music";
			    errorMessage = "Another device is using your account.  Only one can stream at a time.";
			}
		}

		//errorMessage.append(QString(head) + " : " + QString(value));


		/*
		 *  "Update-Client-Auth" :  "DQAAAMcAAAAa7zeRZq-OKTfLaXBi5
38jrFLKuHNWWd14QDkSQ9T1zYqXoruv3Ha0yF1sSuWRkoRBx9ALhaoF1GG5Us30SQTCA3eMx2_tGiNFwsbhlwFebW
3jIHbbtRSO9qmZArjM7OlDzASWpB1H-ORpiV5em6SgHp-pInFKXyrghlZqi14IqL1UBzz32xHz-Zom99UIq0ed-0v
t3RF-pCaZySbbSgNU73bw6PkCmUoFfvEnhUFxMIpXkt5Ox9vjaFi2eRCbUx3E5UuMIUA4oSgKnBzCLgKg

			this happens with 202
		 */
	}
	qDebug() << "GMA: Body: " << QString(reply->readAll());

	if (error != 5 && error != 299) {
		// error 5/299 means I canceled the download
		emit failed();
		if (error == 301) {
			title = "Error 502";
			emit networkError(title, reply->errorString());
		} else {
			emit networkError(title, errorMessage);
		}
	}

	reply->deleteLater();
}

void UrlConnector::cancelRequest() {
	if (currentReply) {
		currentReply->abort();
		currentReply = NULL;
		//temp->remove();
	}
}

UrlConnector::~UrlConnector() {
	// TODO Auto-generated destructor stub
}

