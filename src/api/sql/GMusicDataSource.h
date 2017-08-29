/*
 * GMusicDatasource.h
 *
 *  Created on: Jan 11, 2013
 *      Author: nmmrc18
 */

#ifndef GMUSICDATASOURCE_H_
#define GMUSICDATASOURCE_H_

#include <QObject>
#include <bb/data/SqlConnection>
#include <bb/data/DataAccessReply>
#include <bb/cascades/GroupDataModel>

using namespace bb::data;
using namespace bb::cascades;

class GMusicDatasource : public QObject {
	Q_OBJECT
public:
	GMusicDatasource(QString);
	virtual ~GMusicDatasource();

	void getAllSongs();
	void addSong(const QVariantMap& song);

	void addPlaylist(const QVariantMap& playlist);
	QVariantList getAllPlaylists();

	GroupDataModel* getPlaylist(QString id);
	QVariantList getPlaylistByArtist(QString artistName);
	QVariantList getPlaylistByAlbum(QString albumName, QString artistName);

	QVariant getSong(QString id);

	void updateSong(QVariantMap songMap);

	void setPlaylistAsLocal(QString id);
	void setPlaylistAsLocalStatic(QString name);

	void deleteEverything();

	static const QVariantList SONG_KEYS;

	QString doesSongExist(QVariantMap songMap);
	bool doesSongExistInPlaylistAlready(QVariantMap song, QString pid, QString sid);

	QVariantList runQuery(QString query);

	void specialUpdate(QString playlistId, int sortOrder, QString songId, qint64 timeStamp);

	void runDeleteCheck(QVariantList, QString);


Q_SIGNALS:
	void resultList(QVariantList list);

private:
	SqlConnection *connection;
	QString dbUrl;
	QString connectionName;

	QVariantList getPlaylistSql(QString id);

	bool checkConnection();

	void createInsert(const QVariantMap&, QString, const QVariantList&);

	static const QVariantList PLAYLIST_KEYS;

	int sortNumber;

	QMap<QString, GroupDataModel*> playlistsMap;

};

#endif /* GMUSICDATASOURCE_H_ */
