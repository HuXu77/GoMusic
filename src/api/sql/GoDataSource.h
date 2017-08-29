/*
 * GoDataSource.h
 *
 *  Created on: Apr 22, 2013
 *      Author: nmmrc18
 */

#ifndef GODATASOURCE_H_
#define GODATASOURCE_H_

#include <QObject>

#include <bb/data/SqlConnection>
#include <bb/data/DataSource>

using namespace bb::data;

namespace bb {
	namespace data {
		class SqlConnection;
	}
}

class GoDataSource : public QObject {
	Q_OBJECT

	Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
	Q_PROPERTY(QString connectionName READ connectionName WRITE setConnectionName NOTIFY connectionNameChanged)
public:
	GoDataSource();
	~GoDataSource();

	void setQuery(const QString query);
	QString query();
	Q_INVOKABLE QVariantList runQuery(QString query, bool appendAll);

	void setConnectionName(const QString name);
	QString connectionName();

	void updateSong(QVariantMap songMap);
	QVariant getSong(QString id);

	Q_INVOKABLE void runQuery();
	QVariantList getAllPlaylists();
	bool isPlaylistEmpty(QString id);
	QVariantList getPlaylistSql(QString id);

	void setPlaylistAsLocal(QString id);
	void setPlaylistAsLocalStatic(QString name);

	void deleteEverything();

public:
	static const QVariantList SONG_KEYS;

Q_SIGNALS:
	void queryChanged();
	void connectionNameChanged();
	void dataLoaded(const QVariant &data);

private:
	bool checkConnection();
	void createInsert(const QVariantMap&, QString, const QVariantList&);
	void specialUpdate(QString playlistId, QString songId);

private:
	QString mConnectionName;

	QString mQuery;
	SqlConnection *mSqlConnector;
	DataSource *mDataSource;

	static const QVariantList PLAYLIST_KEYS;
};

#endif /* GODATASOURCE_H_ */
