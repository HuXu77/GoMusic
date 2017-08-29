/*
 * SyncTask.h
 *
 *  Created on: Jan 17, 2013
 *      Author: nmmrc18
 */

#ifndef SYNCTASK_H_
#define SYNCTASK_H_

#include <QObject>
#include "GoogleMusicApi.h"
#include "GMusicDatasource.h"

class SyncTask : public QObject {
	Q_OBJECT
public:
	SyncTask();
	SyncTask(QVariantMap&);
	virtual ~SyncTask();
	QVariantMap songToSync;

public Q_SLOTS:
	void run();
	void syncMetaData();
	void basicComplete();
	void onError(QString, QString);
	void onPlaylistError(QString, QString);

private Q_SLOTS:
	void updateSongDb(QString response);
	void updatePlaylistDb(QString response);
	void updatePlaylistSongDb(QString response);
	void syncLibrary();

Q_SIGNALS:
	void error(QString, QString);
	void failed();
	void syncComplete();
	void updateProgress(qint64,qint64);
	void stillGettingLibrary();

	void buildingPL();

	void jsonDump(QString);

private:
	GoogleMusicApi *api;
	GMusicDatasource *dataSource;

	QVariantList *songs;
	bool gotRecentSongs;
	bool gotEphenNumeral;
	void insertSongs();

	bool removeCookieConnect;
};

#endif /* SYNCTASK_H_ */
