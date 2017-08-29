/*
 * UpdateDBWithLocal.cpp
 *
 *  Created on: Oct 6, 2014
 *      Author: nmmrc18
 */

#include <UpdateDBWithLocal.h>
#include "GoogleMusicApi.h"
#include <bb/data/DataAccessReply>
#include <QVariant>

UpdateDBWithLocal::UpdateDBWithLocal()
{
    QString dataFolder = QDir::homePath();
    QString sqlFile = dataFolder + "/sql/gmusic.db";

    connection = new SqlConnection(sqlFile, "updateLocal");
}

void UpdateDBWithLocal::runUpdate() {
    /*
     * So what would be the most efficient manner to run this?
     *
     * I don't want to do it by looking locally first, I want to see what we have in the db first.
     * So lets get our list of songs and their values and then we loop through and check both SD Card and local
     * for the songs
     */
    DataAccessReply reply = connection->executeAndWait("select * from song where deleted = 0 and shouldBeLocal = 1");
    QVariantList results = reply.result().value<QVariantList>();
    GoogleMusicApi *api = new GoogleMusicApi();

    QDir sdcard = QDir(QDir::currentPath() + "/../../removable/sdcard");
    bool hasSdCard = false;
    if (sdcard.exists()) {
        hasSdCard = true;
    }

    Q_FOREACH(QVariant o, results) {
        QVariantMap res = o.toMap();
        QString title(res.value("title").toString());
        QString artist(res.value("artist").toString());
        QString album(res.value("album").toString());

        QString badName = QString("%1 - %2 - %3").arg(title).arg(artist).arg(album);
        QString fixedName = api->fixLocalName(badName);

        bool foundItOnSdCard = false;
        if (hasSdCard) {
            // we have an sd card
            QString fileLocation = QString(QDir::currentPath() + "/../../removable/sdcard/music/%1.mp3").arg(fixedName);
            QFile song(fileLocation);
            if (song.exists()) {
                foundItOnSdCard = true;
                //connection->executeAndWait("update song set downloadCompleted = 0 where sid = '%1'").arg(res.value("sid").toString());
            }
        }
        bool foundItOnInternal = false;
        if (!foundItOnSdCard) {
            QString fileLocation = QString(QDir::currentPath()+"/shared/music/%1.mp3").arg(fixedName);
            QFile song(fileLocation);
            if (song.exists()) {
                foundItOnInternal = true;
            }
        }

        if (!foundItOnSdCard && !foundItOnInternal) {
            connection->executeAndWait(QString("update song set downloadCompleted = 0 where id = '%1'").arg(res.value("id").toString()));
        } else {
            connection->executeAndWait(QString("update song set downloadCompleted = 1 where id = '%1'").arg(res.value("id").toString()));
        }
    }
    emit complete();
}

UpdateDBWithLocal::~UpdateDBWithLocal()
{
    // TODO Auto-generated destructor stub
}

