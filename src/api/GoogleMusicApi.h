/*
 * GoogleMusicApi.h
 *
 *  Created on: Apr 12, 2013
 *      Author: nmmrc18
 */

#ifndef GOOGLEMUSICAPI_H_
#define GOOGLEMUSICAPI_H_

#include <QtCore/QObject>
#include <QtCore/QVariant>
#include <bb/cascades/GroupDataModel>
#include "UrlConnector.h"

using namespace bb::cascades;

class GoogleMusicApi : public QObject
{
	Q_OBJECT
public:
	GoogleMusicApi();
	GoogleMusicApi(QVariantMap&);
	virtual ~GoogleMusicApi();
	static const QString MAIN_URL;
	static const QString ACCOUNTS_LOGIN;
	static const QString ISSUE_AUTH;
	static const QString TOKEN_AUTH;
	static const QString PLAY_LISTEN;
	static const QString HTTPS_LOADALLTRACKS;
	static const QString HTTPS_EPHEN_NUMERAL;
	static const QString MUSIC_PLAY_SONGID;
	static const QString MUSIC_PLAY_AASONGID;
	static const QString MUSIC_PLAY_MJKSONG;
	static const QString LOAD_PLAYLIST_FEED;
	static const QString LOAD_PLAYLIST_TRACKS;
	static const QString UPDATE_META;
	static const QString CREATE_RADIO;
	static const QString LOAD_RADIOS;
	static const QString LOAD_RADIO_STATION;
	static const QString ADD_AA_TO_LIBRARY;
	static const QString SEARCH_ALL_ACCESS;

	static const QString AUTHTOKEN;
	static const QString IS_LOGGED_IN;
	static const QString USER_CONFIG;
	static const QString AUTH_KEY;

	QString *email;
	QString *password;

	QVariantList playlistToDownload;
	QVariantMap *songToSync;

	int saveLocation;
	QString saveLocationName;

	QFile *toPassToUrlCon;

	QVariantMap songToDownload;

	void getPlaylist(QString id);
	void getSongs(QString continuationToken);
	void setAlbumArtUrl(QString url);
	void renewCookie();
	void getRadio(QString songId, QVariant seed, QList<QVariantMap> recentlyPlayed);
	void loadRadios();
	QString fixLocalName(QString name);


private Q_SLOTS:
	void login();
	QString signature();
	void loginComplete();
	void loginAuthComplete(QString);

	void getAlbumArt();
	void moveToMusicDir();

public Q_SLOTS:
	void loadSong(const QVariantMap);
	void loadSong();
	void onSongUrl(QString);
	void onAllAccessUrl(QByteArray);
	void onSaveSong(QByteArray);
	void getAllSongs();
	void getRecentSongs();
	void getEphenNumeral();
	void getAllPlaylists();
	void getAllPlaylistSongs();

	void updateProgress(qint64, qint64);

	void albumArtDataComplete(QByteArray);
	void requestComplete(QString);
	void allAccessCheckComplete(QString);
	void radioStationsComplete(QString);
	void radioPlaylistComplete(QString);
	void basicComplete(QString);

	void startPlaylistDownload();
	void syncMetaData(QVariantMap&);

	void cancelDownloads();

	void updateUserConfig();
	void onUserConfigUpdate(QString);

	void onFailed();
	void onError(QString, QString);
	void setJsonDump(QString);

Q_SIGNALS:
	// for login completion
	void complete();

	// other completions
	void complete(QString);

	void downloadProgress(qint64,qint64);

	void albumArtData(QByteArray);

	void playlistDownloadComplete();

	void updatePlaylistCount(qint64,qint64);

	void failed();

	void abortConnection();

	void error(QString, QString);

	void jsonDump(QString);

	void radioStations(QVariantList);
	void radioPlaylist(QVariantList);

	void urlLocation(QString);

	void allAccess(bool);

private:
	UrlConnector *connector;

	QString songIdToSave;
	QString songNameToSave;

	QVariantList currentDownload;
	QVariantList urlsToDownload;
	int currIndexInUrls;

	QString *albumArtUrl;

	void downloadPlaylistSong();

	qint64 size;

	qint64 currPos;

};

#endif /* GOOGLEMUSICAPI_H_ */
