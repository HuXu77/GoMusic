/*
 * SyncTask.cpp
 *
 *  Created on: Jan 17, 2013
 *      Author: nmmrc18
 */

#include "SyncTask.h"

#include <bb/data/JsonDataAccess>
#include <QSettings>
#include <QDateTime>

using namespace bb::data;

SyncTask::SyncTask() :
gotRecentSongs(false),
gotEphenNumeral(false),
removeCookieConnect(false) {
}

SyncTask::SyncTask(QVariantMap & songToSync) {
	this->songToSync = QVariantMap();
	Q_FOREACH(QString key, songToSync.keys()) {
		//qDebug() << "GMA: Key: " << key;
		this->songToSync.insert(key, songToSync.value(key));
	}
}

void SyncTask::run() {
	api = new GoogleMusicApi;
	connect(api, SIGNAL(complete()), this, SLOT(syncLibrary()));
	api->updateUserConfig();
}

void SyncTask::syncLibrary() {
	dataSource = new GMusicDatasource("syncTask");
	songs = new QVariantList;
	connect(api, SIGNAL(complete(QString)), this, SLOT(updateSongDb(QString)), Qt::QueuedConnection);
	connect(api, SIGNAL(error(QString,QString)), this, SLOT(onError(QString, QString)), Qt::QueuedConnection);
	emit stillGettingLibrary();
	QSettings settings;
	int lastUpdated = settings.value("lastModifiedDate").toInt();
	if (lastUpdated != 0) {
		gotRecentSongs = true;
		api->getRecentSongs();
	} else {
		// this should only happen on a login. But due to upgrading, I have to do this
		dataSource->runQuery("delete from song where id like 'T%'");
		api->getAllSongs();
	}
}

void SyncTask::syncMetaData() {
	api = new GoogleMusicApi;
	connect(api, SIGNAL(complete()), this, SLOT(basicComplete()));
	api->syncMetaData(songToSync);
}

void SyncTask::basicComplete() {
	emit syncComplete();
}

void SyncTask::updateSongDb(QString response) {
	disconnect(api, SIGNAL(complete(QString)), this, SLOT(updateSongDb(QString)));
	if (removeCookieConnect) {
		disconnect(api, SIGNAL(complete()), api, SLOT(getAllSongs()));
		removeCookieConnect = false;
	}

	qDebug() << "GMA: Updating/adding songs..." << response;

	JsonDataAccess dataAccess;
	QVariant variant = dataAccess.loadFromBuffer(response);
	QVariantMap head = variant.toMap();
	QVariant suc = head.value("success");
	//qDebug() << "GMA: Response: " << response;
	if (suc.isNull()) {
		//QVariant playlistVar = head.value("playlist");
		QVariant dataVar = head.value("data");
		QVariant playlistVar = dataVar.toMap().value("items");
		QVariantList playlist = playlistVar.toList();

		songs->append(playlist);
		/*Q_FOREACH(QVariant song, playlist) {
			pos++;
			dataSource->addSong(song.toMap());
			emit updateProgress(pos,size);
		}
*/

		QVariant conTokenVar = head.value("nextPageToken");
		QString conToken = conTokenVar.toString();

		if (!conToken.isNull() && !conToken.isEmpty()) {
			emit stillGettingLibrary();
			connect(api, SIGNAL(complete(QString)), this, SLOT(updateSongDb(QString)), Qt::QueuedConnection);
			emit updateProgress(0, songs->size());
			api->getSongs(conToken);
			qDebug() << "GMA: Getting songs";
		} else if ((conToken.isNull() || conToken.isEmpty()) && !gotRecentSongs){
			// Next update the playlists
			gotRecentSongs = true;
			insertSongs();
			songs->clear();
			connect(api, SIGNAL(complete(QString)), this, SLOT(updateSongDb(QString)), Qt::QueuedConnection);
			qDebug() << "GMA: Getting recent";
			QSettings settings;
			// this basically means, if they don't have all access, they won't have
			// any ephennumerals
			gotEphenNumeral = !settings.value("Nautilus").toBool();
			api->getRecentSongs();
		} else if (gotRecentSongs && !gotEphenNumeral) {
			gotEphenNumeral = true;
			insertSongs();
			songs->clear();
			connect(api, SIGNAL(complete(QString)), this, SLOT(updateSongDb(QString)), Qt::QueuedConnection);
			qDebug() << "GMA: Getting ephennumeral";
			api->getEphenNumeral();
		} else {
			disconnect(api, SIGNAL(complete(QString)), this, SLOT(updateSongDb(QString)));
			emit buildingPL();
			disconnect(api, SIGNAL(error(QString,QString)), this, SLOT(onError(QString, QString)));

			connect(api, SIGNAL(error(QString,QString)), this, SLOT(onPlaylistError(QString, QString)), Qt::QueuedConnection);
			connect(api, SIGNAL(complete(QString)), this, SLOT(updatePlaylistDb(QString)), Qt::QueuedConnection);
			api->getAllPlaylists();

		}
	} else {
		// this will be for when the cookie expires and we need to get a new one, they seem only last about a month or so.
		qDebug() << "GMA: Renewing cookie";
		api->renewCookie();
		removeCookieConnect = true;
		connect(api, SIGNAL(complete(QString)), this, SLOT(updateSongDb(QString)), Qt::QueuedConnection);
		connect(api, SIGNAL(complete()), api, SLOT(getAllSongs()));
		api->getRecentSongs();
	}
}

void SyncTask::updatePlaylistDb(QString response) {
	disconnect(api, SIGNAL(complete(QString)), this, SLOT(updatePlaylistDb(QString)));

	//emit jsonDump(response);

	JsonDataAccess dataAccess;
	QVariant variant = dataAccess.loadFromBuffer(response);
	QVariantMap head = variant.toMap();
	QVariant dataVar = head.value("data");
	QVariant playlistVar = dataVar.toMap().value("items");
	QVariantList playlists = playlistVar.toList();

	QSettings settings;
	int lastUpdated = settings.value("lastModifiedDate").toInt();
	if (lastUpdated != 0) {
	} else {
		dataSource->runQuery("delete from playlistSongs where 1=1");

		dataSource->runQuery("delete from playlist where 1=1");
		//int i = 0;

	}
	Q_FOREACH(QVariant playlist, playlists) {
		QVariantMap playlistMap = playlist.toMap();
		QString id = playlistMap.value("id").toString();
		qDebug() << "GMA: PL ID : " + id;
		QString name = playlistMap.value("name").toString();
		if (!id.startsWith("start") && !name.isEmpty()) {
			//emit updateProgress(i, playlists.size());
			if (!playlistMap.value("deleted").toBool()) {
				dataSource->addPlaylist(playlistMap);
			} else {
				dataSource->runQuery("delete from playlist where pid = '"+playlistMap.value("playlistId").toString()+"'");
			}
		}
		//i++;
	}

	// We now do a seperate call for the playlist feed
	connect(api, SIGNAL(complete(QString)), this, SLOT(updatePlaylistSongDb(QString)));
	api->getAllPlaylistSongs();

	/*
	QVariant playlistHeadVar = head.value("playlists");

	dataSource->runQuery("delete from playlistSongs where 1=1");
	// I do this, and its like starting over from scratch everytime
	QVariantList validPlaylists;
	int i = 0;
	Q_FOREACH(QVariant playlist, playlists) {
		emit updateProgress(i, playlists.size());
		QVariantMap pl = playlist.toMap();
		dataSource->addPlaylist(pl);
		validPlaylists.append(pl.value("playlistId").toString());
		i++;
	}

	emit updateProgress(playlists.size(), playlists.size());

	dataSource->runDeleteCheck(validPlaylists, "playlist");

	//qDebug() << "GMA: # Of playlists " << playlists.size();

	// special conditions for updating Thumbs Up (0 none, 5 up, 1 down), Recently Added, etc
	emit syncComplete();
	*/
}

void SyncTask::updatePlaylistSongDb(QString response) {
	disconnect(api, SIGNAL(complete(QString)), this, SLOT(updatePlaylistSongDb(QString)));

	JsonDataAccess dataAccess;
	QVariant variant = dataAccess.loadFromBuffer(response);
	QVariantMap head = variant.toMap();
	QVariant dataVar = head.value("data");
	QVariant playlistVar = dataVar.toMap().value("items");
	QVariantList playlistEntries = playlistVar.toList();

	int i = 0;
	bool alreadyAdded = false;
	Q_FOREACH(QVariant entry, playlistEntries) {
		QVariantMap entryMap = entry.toMap();

		QString pid = entryMap.value("playlistId").toString();
		if (!pid.startsWith("start")) {
			QString sid = entryMap.value("trackId").toString();
			if (entryMap.value("source").toString() != "1") {
				// its All Access content, that MIGHT need to be added to library
				QVariantMap songMap = entryMap.value("track").toMap();

				if (songMap.value("deleted").toBool()) {
					songMap.insert("deleted", 1);
				} else {
					songMap.insert("deleted", 0);
				}

				QString tempid = dataSource->doesSongExist(songMap);
				if (!tempid.isNull()) {
					sid = tempid;
				} else {
					sid = songMap.value("storeId").toString();
					songMap.insert("id", sid);
					dataSource->addSong(songMap);
				}
			} else {
				// its USER owned content
			}

			alreadyAdded = dataSource->doesSongExistInPlaylistAlready(entryMap, pid, sid);

			if (!alreadyAdded) {
				dataSource->specialUpdate(pid, entryMap.value("absolutePosition").toInt(), sid, entryMap.value("lastModifiedTimestamp").toLongLong());
			}
		}
		i++;
		emit updateProgress(i, playlistEntries.size());
	}

	emit updateProgress(playlistEntries.size(), playlistEntries.size());

	dataSource->runDeleteCheck(GMusicDatasource::SONG_KEYS, "songs");

	QSettings settings;
	QDateTime today = QDateTime::currentDateTime();
	int todays = today.toTime_t();
	settings.setValue("lastModifiedDate", todays);

	emit syncComplete();

}

void SyncTask::insertSongs() {
	qDebug() << "GMA: Saving songs";
	qint64 size = songs->length();
				// this is where I can build a set to remove deleted songs
	QVariantList validSongs;
	QSettings settings;
	bool setAsLocalForThumbsUp(false);
	if (!settings.value("ThumbsUpLocal").isNull()) {
		setAsLocalForThumbsUp = true;
	}
	int i = 0;
	Q_FOREACH(QVariant songvar, *songs) {
		QVariantMap song = songvar.toMap();
		if (setAsLocalForThumbsUp) {
			int rating = song.value("rating").toInt();
			if (rating == 5) {
				song.insert("shouldBeLocal", 1);
			}
		}
		dataSource->addSong(song);
		emit updateProgress(i, size);
		i++;
	/*if (song.value("trackType").isNull()) {
		// then its an uploaded song
			validSongs.append(song.value("id"));
		} else {
			validSongs.append(song.value("nid"));
		}*/
	}
	emit updateProgress(size, size);
	dataSource->runDeleteCheck(validSongs, "songs");
}

void SyncTask::onPlaylistError(QString title, QString message) {
	if (title.contains("502")) {
		/*QFile *temp = new QFile(QDir::homePath() + "/temp/playlistscrape.html");
		QString html = QString(temp->readAll());
		QRegExp start = QRegExp(".*[,,");
		html.remove(start);
		qDebug() << "GMA: Start: " << start;*/

		QString tit = QString("Unable to retreive playlists");
		QString mess = QString("Unable to retreive playlists because you have over 1000!  Google's servers are not sending your playlists.  This will be fixed in a future update.  But until then see if you can consolidate some playlists.");
		emit syncComplete();
		onError(tit, mess);
	} else {
		onError(title, message);
	}
}

void SyncTask::onError(QString title, QString message) {
	emit error(title, message);
	//if
	emit failed();
}

SyncTask::~SyncTask() {
}

