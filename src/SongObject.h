/*
 * SongObject.h
 *
 *  Created on: Apr 12, 2013
 *      Author: nmmrc18
 */

#ifndef SONGOBJECT_H_
#define SONGOBJECT_H_

#include <QObject>
#include <bb/cascades/GroupDataModel>
#include <bb/cascades/Image>
#include "GoogleMusicApi.h"
#include <QFile>

using namespace bb::cascades;

class SongObject : public QObject {
	Q_OBJECT
	Q_PROPERTY(QVariantMap metaData READ metaData NOTIFY metaDataChanged)
	Q_PROPERTY(QVariantMap song READ song NOTIFY textChanged)
	Q_PROPERTY(bb::cascades::Image albumArt READ albumArt NOTIFY imageChanged)
	Q_PROPERTY(QUrl albumArtThumb READ albumArtThumb NOTIFY imageChanged)
	Q_PROPERTY(QString nextSongLabel READ nextSongLabel NOTIFY textChanged)
	Q_PROPERTY(bool isLocal READ isLocal NOTIFY textChanged)

public:
	SongObject();
	virtual ~SongObject();
	void setIndexPath(QVariantList indexPath);
	QVariantList* getIndexPath();
	Q_INVOKABLE bool isSongDownloading();
	QString getId();
	Q_INVOKABLE QString getUrlLocation();

	void setSong(QVariantMap song);
	QVariantMap song() const;
	// if false then by the time it returns it will be downloading or has already started downloading
	// so the caller needs to connect to our songReadyToPlay signal
	Q_INVOKABLE bool readyToPlay();
	Q_INVOKABLE bool isAvailable();
	QString nextSongLabel();
	bool songIsDownloading;
	Q_INVOKABLE void retryAlbumArts();
	bool artIsDownloading;

	QVariantMap metaData() const;

Q_SIGNALS:
	void metaDataChanged();
	void textChanged();
	void imageChanged();
	void imageThumbChanged();
	void songReadyToPlay();
	void songProgress(int);
	void updatePlaylist(QString, QVariantMap);

	void goToNextSong();

	void cancelDLS();
	void error(QString, QString);

	void needToDownload();

public Q_SLOTS:
	void updateDownloadProgress(qint64, qint64);
	void updateDownloadProgressMediator(int);
	void notifyReady();
	void onReceivedAlbumArt(const QByteArray&);
	void onReceivedAlbumArtThumb(const QByteArray&);
	void getAlbumArts();
	void getAlbumArtThumb();
	QUrl albumArtThumb() const;
	void thumbUp();
	void thumbDown();
	void removeThumb();
	void cancelDownloads();
	void albumArtFailed();
	void setUrlLocation(QString);
	bool isLocal() const;
	void saveLocally(QString);
	void isAllAccess(bool);
	void saveLocallyAfterDownloading();

	void onError(QString, QString);

private:
	// NowPlayingConnection

	// Other UI Elements
	QVariantList* m_indexPath;
	Image albumArt() const;
	GoogleMusicApi *api;
	Image *m_albumArt;

	QUrl m_albumArtThumb;
	int currentPercentDownloaded;

	QString urlLocation;

	bool local;
	bool allAccess;

	QString toSaveLocally;

	QVariantMap m_song;

	// Data Fetching
	QThread *apiThread;

	void startDownload();
	bool isDownloadComplete(QFile&);

	void metaSync();
};

#endif /* SONGOBJECT_H_ */
