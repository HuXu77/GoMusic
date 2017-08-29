/*
 * GoDataSource.cpp
 *
 *  Created on: Apr 22, 2013
 *      Author: nmmrc18
 */

#include "GoDataSource.h"
#include <QVariant>

const QVariantList GoDataSource::SONG_KEYS = QVariantList() << "albumArtUrl" << "durationMillis" << "title"
		<< "id" << "album" << "genre" << "artist" << "rating" << "estimatedSize"
		// below are keys needed for syncing metadata changes back to Google
		"composer" << "albumArtist" << "name" << "comment" << "matchedId" << "titleNorm" << "creationDate" <<
		"type" << "beatsPerMinute" << "recentTimestamp" << "lastModifiedTimestamp" << "source" << "playCount" << "deleted" << "subjectToCuration" <<
		// below are keys that are optional
		"track" <<
		// 'disc', 'year', 'track', 'totalTracks', 'totalDiscs', 'lastPlayed'
		// below are the keys I am using locally
		"shouldBeLocal" << "downloadCompleted" << "metaSynced";
const QVariantList GoDataSource::PLAYLIST_KEYS = QVariantList() << "title" << "playlistId" << "lastModifiedTimestamp" << "shouldBeLocal";

GoDataSource::GoDataSource() {
	QString dataFolder = QDir::homePath();
	QString newFileName = dataFolder + "/sql/gmusic.db";
	QFile newFile(newFileName);

	if (!newFile.exists()) {
		QString appFolder(dataFolder);
		appFolder.chop(4);
		QString originalFileName = appFolder + "app/native/assets/sql/gmusic.db";
		QFile originalFile(originalFileName);

		if (originalFile.exists()) {
			QFileInfo fileInfo(newFileName);
			QDir().mkpath(fileInfo.dir().path());

			originalFile.copy(newFileName);
		}

		QString albumArtFolder = dataFolder + "/albumArt/";
		QDir albumArtFile(albumArtFolder);
		QDir().mkpath(albumArtFile.absolutePath());

		QString tempFolder = dataFolder + "/temp/";
		QDir tempFold(tempFolder);
		QDir().mkpath(tempFold.absolutePath());
	}

	QDir tempAllAccess(dataFolder + "/allaccess/");
	if (!tempAllAccess.exists()) {
		QDir().mkpath(tempAllAccess.absolutePath());
	}

	QFile dbFile(newFileName);

	mSqlConnector = new SqlConnection(newFileName, mConnectionName);

	if (newFile.exists()) {
		//runQuery(QString("alter table song add column track numeric"), false);
	}
}

bool GoDataSource::isPlaylistEmpty(QString id) {
    QSettings settings;
    bool showLocalOnly = settings.value("showLocalOnly").toBool();
	QString query("select * from song a, playlistSongs b where a.id = b.sid and b.pid = '" + id + "' order by b.sortOrder desc");
	if (showLocalOnly) {
	    query = "select * from song a, playlistSongs b where a.id = b.sid and b.pid = '" + id + "' a.downloadCompleted = 1 order by b.sortOrder desc";
	}
	QVariantList results = runQuery(query, false);
	return results.isEmpty();
}

void GoDataSource::setPlaylistAsLocal(QString id) {
	QString query = QString("update playlist set shouldBeLocal = 1 where playlistId = \"%1\"").arg(id);
	runQuery(query, false);

	QVariantList songs = getPlaylistSql(id);
	Q_FOREACH(QVariant song, songs) {
		QVariantMap songMap = song.toMap();
		query = QString("update song set shouldBeLocal = 1 where id = \"%1\"").arg(songMap.value("id").toString());
		runQuery(query, false);
	}
}

QVariantList GoDataSource::getAllPlaylists() {
	QVariantList results = runQuery(QString("SELECT * FROM playlist WHERE 1=1"), false);
	return results;
}

QVariantList GoDataSource::getPlaylistSql(QString id) {
	QVariantList results;
	QVariantList returnResults;
	/**
	 * select sid from playlistSongs where pid = \"%1\" order by sortOrder asc
	 *
	 * select * from song where id in (select sid from playlistSongs where pid = \"%1\" order by sortOrder asc)
	 */
	QString queryForSongOrder = QString("select * from playlistSongs where pid = \"%1\" order by sortOrder asc").arg(id);
	results = runQuery(queryForSongOrder, false);
	QSettings settings;
	bool showLocalOnly = settings.value("showLocalOnly", false).toBool();
	Q_FOREACH(QVariant song, results) {
		QVariantMap songMap(song.toMap());
		QString queryForSong = QString("select * from song where id = \"%1\" and deleted = 0").arg(songMap.value("sid").toString());
		if (showLocalOnly) {
		    queryForSong = QString("select * from song where id = \"%1\" and deleted = 0 and downloadCompleted = 1").arg(songMap.value("sid").toString());
		}
		QVariantList res = runQuery(queryForSong, false);
		//qDebug() << "GMA: Result: " << res.size() << " for song id:" << songMap.value("sid").toString();
		if (!res.isEmpty()) {
		    QVariantMap mapToSave = res.value(0).toMap();
		    mapToSave.insert("sortOrder", songMap.value("sortOrder"));
		    returnResults.push_front(mapToSave);
		}
	}

	//QVariantMap firstSong = returnResults.value(0).toMap();
	//qDebug() << "GMA: First song in playlist: " << firstSong.value("title").toString() << " by " << firstSong.value("artist").toString();
	return returnResults;
}

void GoDataSource::setPlaylistAsLocalStatic(QString name) {
	if (name.contains("Thumbs Up")) {
		QString query = QString("update song set shouldBeLocal = 1 where rating = 5");
		runQuery(query, false);
	}
}

void GoDataSource::setQuery(const QString query) {
	//qDebug() << "GMA: Setting Query: " << query;
	mQuery = query;
	queryChanged();
}

void GoDataSource::updateSong(QVariantMap songMap) {
	QString query = QString("update song set ");
	Q_FOREACH(QVariant key, GoDataSource::SONG_KEYS) {
		QString keyStr = key.toString();
		if (!keyStr.contains("id")) {
			// These are because of the new API
			QVariant value = songMap.value(keyStr);
			if (!songMap.value("albumArtRef").isNull()) {
				if (keyStr.startsWith("albumArtUrl")) {
					QVariantList urls = songMap.value("albumArtRef").toList();
					QVariantMap urlsMap = urls.at(0).toMap();
					value = urlsMap.value("url");
				} else if (keyStr.startsWith("track")) {
					value = songMap.value("trackType");
				}
			}
			query = query + QString("%1 = ").arg(keyStr);
			QByteArray byteArray(value.toByteArray());
			QString valueStr = QString::fromUtf8(byteArray.constData(), byteArray.size());
			if (value.type() == QVariant::String) {
				if (valueStr.contains("\'")) {
					query = query + QString("\'%1\'").arg(valueStr.replace("\'", "\'\'"));
				} else {
					query = query + QString("\'%1\'").arg(valueStr);
				}
			} else {
				query = query + QString("%1").arg(valueStr);
			}
			if (!keyStr.contains("metaSynced")) {
				query = query + QString(",");
			}
		}
	}
	query = query + QString(" where id = \"%1\"").arg(songMap.value("id").toString());
	setQuery(query);
	runQuery();
}

QString GoDataSource::query() {
	return mQuery;
}

QVariant GoDataSource::getSong(QString id) {
	QString query = QString("select * from song where id = \"%1\"").arg(id);
	QVariantList results = runQuery(query, false);
	return results.value(0);
}

void GoDataSource::setConnectionName(const QString name) {
	mConnectionName = name;
	QString dataFolder = QDir::homePath();
	QString newFileName = dataFolder + "/sql/gmusic.db";
	QFile dbFile(newFileName);

	mSqlConnector = new SqlConnection(newFileName, mConnectionName);

	//runQuery(QString("alter table song add column track numeric"), false);
}

QString GoDataSource::connectionName() {
	return mConnectionName;
}

bool GoDataSource::checkConnection() {
	if (mSqlConnector) {
		return true;
	} else {
		QString dbUrl = QDir::homePath() + "/sql/gmusic.db";
		QFile newFile(dbUrl);

		if (newFile.exists()) {
			if (mSqlConnector) {
				delete mSqlConnector;
			}

			mSqlConnector = new SqlConnection(dbUrl, mConnectionName);
			return true;
		}
	}
	return false;
}

void GoDataSource::runQuery() {
	runQuery(mQuery, false);
}

QVariantList GoDataSource::runQuery(QString query, bool appendAll) {
	QVariantList results;
	if (checkConnection()) {
		DataAccessReply reply = mSqlConnector->executeAndWait(query, 0);
		if (reply.hasError() && !query.contains("alter")) {
			qDebug() << "GMA: Error in SQL: " << reply.errorMessage();
			qDebug() << "GMA: Query errored: " << query;
		} else {
			QVariantList tempResults = reply.result().value<QVariantList>();
			if (appendAll) {

				Q_FOREACH(QVariant o, tempResults) {
					QVariantMap res = o.toMap();
					res.insert("sortOrder", 1);
					results.append(res);
				}
				QVariantMap allMap;
				allMap.insert("album", "All Songs");
				allMap.insert("sortOrder", 0);
				results.append(allMap);
			} else {
				results = tempResults;
			}
			emit dataLoaded(results);
		}
	}
	return results;
}

void GoDataSource::deleteEverything() {
	QString dataFolder = QDir::homePath();
	QString newFileName = dataFolder + "/sql/gmusic.db";
	QFile newFile(newFileName);
	if (newFile.exists()) {
		newFile.remove();
	}
}

GoDataSource::~GoDataSource() {
	if (mSqlConnector) {
		//mSqlConnector->
	}
}

