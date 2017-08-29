/*
 * GMusicDatasource.cpp
 *
 *  Created on: Jan 11, 2013
 *      Author: nmmrc18
 */

#include "GMusicDatasource.h"
#include <QtCore/qdir.h>
#include <QtCore/qfile.h>
#include <QDebug>
#include <bb/cascades/GroupDataModel>
#include <bb/cascades/DataModel>

using namespace bb::data;
using namespace bb::cascades;

const QVariantList GMusicDatasource::SONG_KEYS = QVariantList() << "albumArtUrl" << "durationMillis" << "title"
		<< "id" << "album" << "genre" << "artist" << "rating" << "estimatedSize"
		// below are keys needed for syncing metadata changes back to Google
		"composer" << "albumArtist" << "name" << "comment" << "matchedId" << "titleNorm" << "creationDate" <<
		"type" << "beatsPerMinute" << "recentTimestamp" << "lastModifiedTimestamp" << "source" << "playCount" << "deleted" << "subjectToCuration" <<
		// below are keys that are optional
		"track" <<
		// 'disc', 'year', 'track', 'totalTracks', 'totalDiscs', 'lastPlayed'
		// below are the keys I am using locally
		"shouldBeLocal" << "downloadCompleted" << "metaSynced";
const QVariantList GMusicDatasource::PLAYLIST_KEYS = QVariantList() << "title" << "playlistId" << "shouldBeLocal" << "lastModifiedTimestamp";

GMusicDatasource::GMusicDatasource(QString connectionName) :
	connectionName(connectionName)
{
	QString dataFolder = QDir::homePath();
	QString newFileName = dataFolder + "/sql/gmusic.db";
	QFile newFile(newFileName);
	dbUrl = newFileName;

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

	QFile dbFile(dbUrl);

	connection = new SqlConnection(dbUrl, connectionName);

	if (newFile.exists()) {
		//runQuery(QString("alter table song add column track numeric"));
	}
}

void GMusicDatasource::getAllSongs() {
    QSettings settings;
    bool showLocalOnly = settings.value("showLocalOnly", false).toBool();
    if (showLocalOnly) {
        QVariantList results = runQuery(QString("SELECT * FROM song WHERE deleted != 1 and shouldBeLocal = 1"));
        emit resultList(results);
    } else {
        QVariantList results = runQuery(QString("SELECT * FROM song WHERE deleted != 1"));
	    emit resultList(results);
    }
}

void GMusicDatasource::addSong(const QVariantMap& song) {

	// We will use ID to determine update or insert
	createInsert(song, QString("song"), SONG_KEYS);
}

void GMusicDatasource::addPlaylist(const QVariantMap& playlist) {

	createInsert(playlist, QString("playlist"), PLAYLIST_KEYS);
	// this updates our playlist db
	// now we have to do a seperate call for the playlist songs


	// This will assume that songs has been populated/updated
	/*QVariant songs = playlist.value("playlist");
	QVariantList songsList = songs.toList();
	sortNumber = 0;
	Q_FOREACH(QVariant song, songsList) {
		// Again I warn, we are assuming all songs in a playlist
		// already exist in our song DB
		/**
		 * So our playlistId will be const at this point so it would be
		 * INSERT OR REPLACE INTO song (id, *allOtherSongKeys*, playlistId)
		 * 	VALUES (
		 * 		(select *key* from song where id = *songId*),
		 * 		*playlistId*);
		 *
		 * 	So playlistId and songId are params
		 *//*
		QVariantMap songMap = song.toMap();
		QString playlistId = playlist.value("playlistId").toString();
		QString songId = songMap.value("id").toString();
		addSong(songMap);
		specialUpdate(playlistId, songId);
	}
	*/
}

QVariantList GMusicDatasource::getAllPlaylists() {
	QVariantList results = runQuery(QString("SELECT * FROM playlist WHERE 1=1"));
	playlistsMap = QMap<QString, GroupDataModel*>();
	Q_FOREACH(QVariant pl, results) {
		QVariantMap playL = pl.toMap();
		GroupDataModel *model = new GroupDataModel();
		QString playlistId(playL.value("playlistId").toString());
		QVariantList res = getPlaylistSql(playlistId);
		model->setSortingKeys(QStringList() << "sortOrder");
		model->setGrouping(ItemGrouping::None);
		model->insertList(res);
		playlistsMap.insert(playlistId, model);
	}
	return results;
}

QVariantList GMusicDatasource::getPlaylistSql(QString id) {
	QVariantList results;
	QVariantList returnResults;
	/**
	 * select sid from playlistSongs where pid = \"%1\" order by sortOrder asc
	 *
	 * select * from song where id in (select sid from playlistSongs where pid = \"%1\" order by sortOrder asc)
	 */
	QString queryForSongOrder = QString("select * from playlistSongs where pid = \"%1\" order by sortOrder asc").arg(id);
	results = runQuery(queryForSongOrder);
	QSettings settings;
	bool showLocalOnly = settings.value("showLocalOnly", false).toBool();
	Q_FOREACH(QVariant song, results) {
		QVariantMap songMap(song.toMap());

		QString queryForSong = QString("select * from song where id = \"%1\" and deleted = 0").arg(songMap.value("sid").toString());
		QVariantList res = runQuery(queryForSong);
		//qDebug() << "GMA: Result: " << res.size() << " for song id:" << songMap.value("sid").toString();
		QVariantMap mapToSave = res.value(0).toMap();
		mapToSave.insert("sortOrder", songMap.value("sortOrder"));
		returnResults.push_front(mapToSave);
	}

	QVariantMap firstSong = returnResults.value(0).toMap();
	//qDebug() << "GMA: First song in playlist: " << firstSong.value("title").toString() << " by " << firstSong.value("artist").toString();
	return returnResults;
}

GroupDataModel* GMusicDatasource::getPlaylist(QString id) {
	if (id.contains("Thumbs Up")) {
		QString query = QString("select * from song where rating = 5");
		QVariantList results = runQuery(query);
		GroupDataModel *gmodel = new GroupDataModel(this);
		gmodel->insertList(results);
		return gmodel;
	}
	GroupDataModel *gmodel = playlistsMap.value(id);
	return gmodel;
}

QVariantList GMusicDatasource::getPlaylistByArtist(QString artistName) {
	QVariantList results;
	if (artistName.contains("\'")) {
		artistName = artistName.replace("\'", "\'\'");
	}
	QString query = QString("select * from song where artist = '%1'").arg(artistName);
	results = runQuery(query);
	return results;
}

QVariantList GMusicDatasource::getPlaylistByAlbum(QString albumName, QString artistName) {
	QVariantList results;
	if (albumName.contains("\'")) {
		albumName = albumName.replace("\'", "\'\'");
	}
	if (artistName.contains("\'")) {
		artistName = artistName.replace("\'", "\'\'");
	}
	QString query = QString("select * from song where album = '%1' and artist = '%2'").arg(albumName).arg(artistName);
	results = runQuery(query);
	return results;
}

void GMusicDatasource::setPlaylistAsLocal(QString id) {
	QString query = QString("update playlist set shouldBeLocal = 1 where playlistId = \"%1\"").arg(id);
	runQuery(query);

	QVariantList songs = getPlaylistSql(id);
	Q_FOREACH(QVariant song, songs) {
		QVariantMap songMap = song.toMap();
		query = QString("update song set shouldBeLocal = 1 where id = \"%1\"").arg(songMap.value("id").toString());
		runQuery(query);
	}
}

void GMusicDatasource::setPlaylistAsLocalStatic(QString name) {
	if (name.contains("Thumbs Up")) {
		QString query = QString("update song set shouldBeLocal = 1 where rating = 5");
		runQuery(query);
	}
}

void GMusicDatasource::updateSong(QVariantMap songMap) {
	QString query = QString("update song set ");
	Q_FOREACH(QVariant key, SONG_KEYS) {
		QString keyStr = key.toString();
		if (!keyStr.contains("id")) {
			// These are because of the new API
			QVariant value = songMap.value(keyStr);
			if (keyStr.contains("type")) {
				value = songMap.value("trackType");
			} else if (keyStr.startsWith("albumArtUrl")) {
				QVariantList urls = songMap.value("albumArtRef").toList();
				if (urls.size() != 0) {
					QVariantMap urlsMap = urls.at(0).toMap();
					value = urlsMap.value("url");
				}
			} else if (keyStr.startsWith("track")) {
				value = songMap.value("trackType");
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
	runQuery(query);
}

void GMusicDatasource::specialUpdate(QString playlistId, int sortOrder, QString songId, qint64 timeStamp) {
	QString insert = QString("INSERT OR REPLACE INTO playlistSongs (pid, sortOrder, sid, lastModifiedTimestamp");
	QString values = QString(") VALUES ('%1', %2, '%3', %4)").arg(playlistId).arg(sortOrder).arg(songId).arg(timeStamp);

	insert.append(values);

	runQuery(insert);
}

QString GMusicDatasource::doesSongExist(QVariantMap songMap) {
	QString search = QString("SELECT * from song where matchedId = '%1'");
	QString storeId = songMap.value("storeId").toString();
	QString query = search.arg(storeId);
	QVariantList results = runQuery(query);
	if (results.size() > 0) {
		// means its in the library
		QVariantMap resSong = results.at(0).toMap();
		return resSong.value("id").toString();
	}
	return QString::null;
}

bool GMusicDatasource::doesSongExistInPlaylistAlready(QVariantMap song, QString pid, QString sid) {
	int order = song.value("absolutePosition").toInt();
	QString search = QString("select * from playlistSongs where sid = '%1' and pid = '%2' and sortOrder = %3").arg(sid).arg(pid).arg(order);
	QVariantList results = runQuery(search);
	if (results.size() > 0) {
		// we have already added the song
		return true;
	}
	return false;
}

QVariant GMusicDatasource::getSong(QString id) {
	QString query = QString("select * from song where id = \"%1\"").arg(id);
	QVariantList results = runQuery(query);
	return results.value(0);
}

void GMusicDatasource::createInsert(const QVariantMap& object, QString table, const QVariantList& keys) {
	/*INSERT OR REPLACE INTO Employee (id,name,role)
	  VALUES (  1,
	            'Susan Bar',
	            (select role from Employee where id = 1)
	          );*/
	QString insert = QString("INSERT OR REPLACE INTO %1 (").arg(table);
	QString values = QString(") VALUES (");

	QVariantMap tempList;
	Q_FOREACH(QVariant key, keys) {
		QString keyString = key.toString();
		QVariant value = object.value(keyString);
		if (table.startsWith("playlist") && !table.contains("Songs")) {
			if (keyString.startsWith("title")) {
				value = object.value("name");
			}
		}
		if (keyString.contains("type")) {
			value = object.value("trackType");
		} else if (keyString.contains("albumArtUrl")) {
			QVariantList urls = object.value("albumArtRef").toList();
			if (urls.size() != 0) {
				QVariantMap urlsMap = urls.at(0).toMap();
				value = urlsMap.value("url");
			}
		} else if (keyString.startsWith("track")) {
			value = object.value("trackNumber");
		} else if (keyString.startsWith("id")) {
			if (object.value("id").isNull()) {
				value = object.value("nid");
			} else {
				value = object.value("id");
			}
		} else if (keyString.startsWith("playlistId")) {
			value = object.value("id");
		}
		if (keyString.startsWith("source")) {
			if (object.value(keyString).isNull()) {
				value = 1;
			}
		}
		if (keyString.startsWith("matchedId")) {
			if (!object.value("storeId").isNull()) {
				value = object.value("storeId").toString();
			}
		}
		/*
		 * if (keyStr.contains("type")) {
				value = songMap.value("trackType");
			} else if (keyStr.startsWith("albumArtUrl")) {
			QVariantList urls = songMap.value("albumArtRef").toList();
			QVariantMap urlsMap = urls.at(0).toMap();
			value = urlsMap.value("url");
			} else if (keyStr.startsWith("track")) {
				value = songMap.value("trackType");
			}
		 */

		QByteArray byteArray(value.toByteArray());
		QString valueString = QString::fromUtf8(byteArray.constData(), byteArray.size());
		if (!value.isNull() && !value.toString().isEmpty()) {
			tempList.insert(keyString, value);
		} else if (keyString.contains("album") || keyString.contains("artist") ||
				keyString.contains("genre")) {
			tempList.insert(keyString, QString("Unknown"));
		} else if (keyString.contains("shouldBeLocal") || keyString.contains("downloadCompleted")) {
			// TODO: query for existing value first, then default to this
			if (!object.value("shouldBeLocal").isNull()) {
				tempList.insert(keyString, 1);
			} else {
				tempList.insert(keyString, 0);
			}
		} else if (keyString.contains("metaSynced")) {
			tempList.insert(keyString, 1);
		}
	}

	int i = 0;
	int size = tempList.keys().size();
	Q_FOREACH(QString key, tempList.keys()) {
		QVariant value = tempList.value(key);
		QString query;
		insert.append(key);
		if (i+1 < size) {
			insert.append(",");
		}

		QString valueQuery;
		QByteArray byteArray(value.toByteArray());
		QString valueStr = QString::fromUtf8(byteArray.constData(), byteArray.size());
		if (value.type() == QVariant::String) {
			if (valueStr.contains("\'")) {
				valueQuery = QString("\'%1\'").arg(valueStr.replace("\'", "\'\'"));
			} else {
				valueQuery = QString("\'%1\'").arg(valueStr);
			}
		} else if (value.type() == QVariant::LongLong || value.type() == QVariant::Int ||
				value.type() == QVariant::ULongLong) {
			valueQuery = QString("%1").arg(value.toString());
		} else if (value.type() == QVariant::Bool) {
			bool boo = value.toBool();
			if (boo) {
				valueQuery = QString("1");
			} else {
				valueQuery = QString("0");
			}
		}
		values.append(valueQuery);
		if (i+1 < size) {
			values.append(",");
		}
		i++;
	}

	values.append(")");
	insert.append(values);

	//qDebug() << "GMA: Executing insert: " << insert;
	runQuery(insert);
}

void GMusicDatasource::runDeleteCheck(QVariantList keys, QString table) {
	QString query("");
	if (table.contains("songs")) {
		query.append("delete from song where deleted = 1");
	} else if (table.contains("playlist")) {
		query.append("delete from playlist where playlistId not in (");
		for (int i = 0; i < keys.length(); i++) {
			QVariant var = keys.at(i);
			QString songId(var.toString());
			QString idQuoted(QString("'%1'").arg(songId));
			query.append(idQuoted);
			if (i+1 != keys.length()) {
				query.append(", ");
			}
		}
			query.append(")");
	}


	runQuery(query);
}

void GMusicDatasource::deleteEverything() {
	QString deleteSongs = QString("delete from song where 1=1");
	QString deletePlaylists = QString("delete from playlist where 1=1");
	QString deletePlaylistMapper = QString("delete from playlistSongs where 1=1");
	runQuery(deleteSongs);
	runQuery(deletePlaylists);
	runQuery(deletePlaylistMapper);
}

bool GMusicDatasource::checkConnection() {
	if (connection) {
		return true;
	} else {
		QFile newFile(dbUrl);

		if (newFile.exists()) {
			if (connection) {
				delete connection;
			}

			connection = new SqlConnection(dbUrl, connectionName);
			return true;
		}
	}
	return false;
}

QVariantList GMusicDatasource::runQuery(QString query) {
	QVariantList results;
	if (checkConnection()) {
		//qDebug() << "GMA: Running query: " << query;
		DataAccessReply reply = connection->executeAndWait(query, 0);
		if (reply.hasError()) {
			//qDebug() << "GMA: Error in SQL: " << reply.errorMessage();
			qDebug() << "GMA: Query errored: " << query;
		} else {
			results = reply.result().value<QVariantList>();
		}
	}
	return results;
}

GMusicDatasource::~GMusicDatasource() {
	delete connection;
}

