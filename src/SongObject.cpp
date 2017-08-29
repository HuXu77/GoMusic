/*
 * SongObject.cpp
 *
 *  Created on: Apr 12, 2013
 *      Author: nmmrc18
 */

#include "SongObject.h"
#include <bb/multimedia/NowPlayingConnection>
#include "GoDataSource.h"
#include "SyncTask.h"

using namespace bb::multimedia;

SongObject::SongObject()
: QObject(),
  songIsDownloading(false),
  artIsDownloading(false),
  api(new GoogleMusicApi),
  m_albumArt(new Image(QUrl("asset:///images/default_album.png"))),
  m_albumArtThumb(QUrl("/app/native/assets/images/default_album_thumb.png")),
  currentPercentDownloaded(0),
  local(false),
  allAccess(false)
  {
}

QVariantMap SongObject::metaData() const {
	if (m_song.isEmpty()) {
		return QVariantMap();
	}
	QVariantMap m_metaData;
	m_metaData[MetaData::Title] = song().value("title").toString();
	m_metaData[MetaData::Artist] = song().value("artist").toString();

	return m_metaData;
}

void SongObject::setIndexPath(QVariantList indexPath) {
	m_indexPath = new QVariantList(indexPath);
}

QVariantMap SongObject::song() const {
	return m_song;
}

void SongObject::setSong(QVariantMap song) {
	m_song = song;

	emit metaDataChanged();
	if (m_albumArt) {
		free(m_albumArt);
		m_albumArt = NULL;
	}
	m_albumArt = new Image(QUrl("asset:///images/default_album.png"));
	m_albumArtThumb = QUrl("/app/native/assets/images/default_album_thumb.png");
	songIsDownloading = false;
	local = false;

	//qDebug() << "GMA: Song: " << song.value("title").toString() << " duration: " << song.value("durationMillis").toString();
	//qDebug() << "GMA: Before imageThumbChanged";
	//emit imageThumbChanged();
	//qDebug() << "GMA: Before imageChanged";
	//emit imageChanged();
	emit textChanged();
}

bool SongObject::isSongDownloading() {
	return songIsDownloading;
}

bool SongObject::isAvailable() {
	if (m_song.isEmpty()) {
		return false;
	}
	return true;
}

bool SongObject::readyToPlay() {
	//qDebug() << "GMA: Testing if readyToPlay";
	if ((songIsDownloading && currentPercentDownloaded < 15) || m_song.isEmpty()) {
		return false;
	}
	QString songId(getId());

	QString tempLocation = QString(QDir::homePath() + "/temp/%1.mp3").arg(songId);
	QFile tempFile(tempLocation);

	QString title(song().value("title").toString());
	QString artist(song().value("artist").toString());
	QString album(song().value("album").toString());
	QString localFileName = QString("%1 - %2 - %3").arg(title).arg(artist).arg(album);
	if (localFileName.contains("/")) {
		localFileName = localFileName.replace("/", "-");
	}
	if (localFileName.contains("?")) {
		localFileName = localFileName.replace("?", "");
	}
	if (localFileName.contains("...")) {
		localFileName = localFileName.remove("...");
	}
	QString localLocation = QString(QDir::currentPath() + "/shared/music/%1.mp3").arg(localFileName);
	QFile localFile(localLocation);
	if (localFile.exists()) {
		qDebug() << "GMA: QFile can locate it";
	}

	QString local2Location = QString(QDir::currentPath() + "/../../removable/sdcard/music/%1.mp3").arg(localFileName);
	QFile local2File(local2Location);

	QString local3Location = QString(QDir::homePath() + "/allaccess/%1.mp3").arg(songId);
	QFile local3File(local3Location);

	getAlbumArts();

	if (m_song.value("estimatedSize").toLongLong() == 0) {
		// I haven't got the estimated size so I have to assume
		// its an incomplete download until I get the estimated size

		// So do I need to know where it is? Yes, if one currently exists
		// I should know its location so I can delete or keep it
		if (tempFile.exists()) {
			// then check on its download and don't worry about its size
			local = false;
			urlLocation = tempLocation;
			if (currentPercentDownloaded > 15) {
				// this means it is downloading to this location
				// there is no need to check the bytes
				if (currentPercentDownloaded == 100) {
					songIsDownloading = false;
				}
				return true;
			} else if (!songIsDownloading) {
				// if the song is not currently downloading
				// we need to check the bytes
				// which will be done at the lower levels
			}
		} else if (localFile.exists()) {
			// need to check the bytes
			urlLocation = localLocation;
		} else if (local2File.exists()) {
			urlLocation = local2Location;
		} else if (local3File.exists()) {
			urlLocation = local3Location;
		} else {
			urlLocation = tempLocation;
		}

		// if we get here, then we know the song needs to have its bytes checked
		emit needToDownload();
		//qDebug() << "GMA: Starting song download " << title;
		songIsDownloading = true;
		startDownload();
		return false;
	} else {
		// we have an estimated size, so we should check for
		// the file already existing
		if (tempFile.exists()) {
			// then check on its download and don't worry about its size
			local = false;
			urlLocation = tempLocation;
			if (currentPercentDownloaded > 15) {
				// this means it is downloading to this location
				// there is no need to check the bytes
				if (currentPercentDownloaded == 100) {
					songIsDownloading = false;
				}
				return true;
			} else if (!songIsDownloading) {
				// if the song is not currently downloading
				// we need to check the bytes
				if (isDownloadComplete(tempFile)) {
					// we will assume this is a complete download
					return true;
				} else {
					// incomplete download
					// need to have the lower levels fix it
					// or I may need to delete it
				}
			}
		} else if (localFile.exists()) {
			// need to check the bytes
			urlLocation = localLocation;
			if (isDownloadComplete(localFile)) {
				return true;
			}
		} else if (local2File.exists()) {
			urlLocation = local2Location;
			if (isDownloadComplete(local2File)) {
				return true;
			}
		} else if (local3File.exists()) {
			urlLocation = local3Location;
			if (isDownloadComplete(local3File)) {
				return true;
			}
		} else {
			urlLocation = tempLocation;
		}

		// if we get here then we need to download or redownload
		emit needToDownload();
		songIsDownloading = true;
		startDownload();
		return false;
	}

	/*if (tempFile.exists()) {
		if (currentPercentDownloaded > 15 && tempFile.size() > 0) {
			urlLocation = tempLocation;
			if (currentPercentDownloaded == 100) {
				songIsDownloading = false;
			}
			local = false;
			return true;
		} else if (!songIsDownloading && tempFile.size() > 0) {
			// need to check if it needs to be re-downloaded
			urlLocation = tempLocation;
			local = false;
			return true;
		} else {
			urlLocation = tempLocation;
		}
	} else if (localFile.exists()) {
		// need to check if it needs to be re-downloaded
		urlLocation = localLocation;
		songIsDownloading = false;
		local = true;
		emit textChanged();
		return true;
	} else if (local2File.exists()) {
		// need to check if it needs to be re-downloaded
		urlLocation = local2Location;
		songIsDownloading = false;
		local = true;
		emit textChanged();
		return true;
	} else if (local3File.exists()) {
		// need to check if it needs to be re-downloaded
		urlLocation = local3Location;
		songIsDownloading = false;
		local = true;
		emit textChanged();
		return true;
	}

	emit needToDownload();
	//qDebug() << "GMA: Starting song download " << title;
	songIsDownloading = true;
	startDownload();

	urlLocation = tempLocation;*/
	return false;
}

bool SongObject::isDownloadComplete(QFile &file) {
	qint64 currSize = file.size();
	qint64 estSize = m_song.value("estimatedSize").toLongLong();
	qint64 diff = estSize - currSize;
	if (diff < 2000) {
		return true;
	}
	return false;
}

void SongObject::startDownload() {
	GoogleMusicApi *api = new GoogleMusicApi;
	QThread *thread = new QThread(this);
	api->moveToThread(thread);

	connect(api, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(updateDownloadProgress(qint64,qint64)), Qt::QueuedConnection);
	connect(api, SIGNAL(error(QString,QString)), this, SLOT(onError(QString,QString)), Qt::QueuedConnection);
	connect(this, SIGNAL(cancelDLS()), api, SLOT(cancelDownloads()), Qt::QueuedConnection);
	connect(api, SIGNAL(complete()), this, SLOT(notifyReady()), Qt::QueuedConnection);
	connect(api, SIGNAL(urlLocation(QString)), this, SLOT(setUrlLocation(QString)), Qt::QueuedConnection);

	/** Standard Thread Clean Up **/
	connect(api, SIGNAL(complete()), thread, SLOT(quit()));
	connect(api, SIGNAL(failed()), thread, SLOT(quit()));
	connect(thread, SIGNAL(finished()), api, SLOT(deleteLater()));
	connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));

	//qDebug() << "GMA: Loading song: " << song().value("title").toString();

	api->songToDownload = song();

	connect(thread, SIGNAL(started()), api, SLOT(loadSong()));

	thread->start();
}

void SongObject::saveLocally(QString location) {
	GoDataSource *data = new GoDataSource;
	data->setConnectionName("saveLocally");
	m_song.insert("shouldBeLocal", true);
	data->updateSong(m_song);
	data->deleteLater();
	if (songIsDownloading) {
		toSaveLocally = location;
		connect(this, SIGNAL(songReadyToPlay()), this, SLOT(saveLocallyAfterDownloading()), Qt::QueuedConnection);
	} else if (!isLocal()) {
		QString songId(getId());

		QString tempLocation = QString(QDir::homePath() + "/temp/%1.mp3").arg(songId);
		QFile tempFile(tempLocation);

		QString fileLocation;
		QString title(m_song.value("title").toString());
		QString artist(m_song.value("artist").toString());
		QString album(m_song.value("album").toString());
		QString songNameToSave = QString("%1 - %2 - %3").arg(title).arg(artist).arg(album);
		QString songWithBaddys = QString(songNameToSave);
		GoogleMusicApi *zzz = new GoogleMusicApi;
		songNameToSave = zzz->fixLocalName(songNameToSave);

		if (tempFile.exists()) {
			if (allAccess) {
				fileLocation = QString(QDir::homePath() + "/allaccess/%1.mp3").arg(getId());
			} else {
				if (location.contains("sdcard")) {
					QDir sdcard = QDir(QDir::currentPath() + "/../../removable/sdcard");
					if (sdcard.exists()) {
						// we can save to SD Card because it exists
						fileLocation = QString(QDir::currentPath() + "/../../removable/sdcard/music/%1.mp3").arg(songNameToSave);
						QString baddyLocation = QString(QDir::currentPath() + "/../../removable/sdcard/music/%1.mp3").arg(songWithBaddys);
						QFile baddyFile(baddyLocation);
						if (baddyFile.exists()) {
							baddyFile.rename(songNameToSave);
						}
					} else {
						fileLocation = QString(QDir::currentPath()+"/shared/music/%1.mp3").arg(songNameToSave);
						QString baddyLocation = QString(QDir::currentPath()+"/shared/music/%1.mp3").arg(songWithBaddys);
						QFile baddyFile(baddyLocation);
						if (baddyFile.exists()) {
							baddyFile.rename(songNameToSave);
						}
					}
				} else {
					fileLocation = QString(QDir::currentPath()+"/shared/music/%1.mp3").arg(songNameToSave);
					QString baddyLocation = QString(QDir::currentPath()+"/shared/music/%1.mp3").arg(songWithBaddys);
					QFile baddyFile(baddyLocation);
					if (baddyFile.exists()) {
						baddyFile.rename(songNameToSave);
					}
				}
				tempFile.copy(fileLocation);
			}
		}
		local = true;
		emit textChanged();
	}
}

void SongObject::saveLocallyAfterDownloading() {
	saveLocally(toSaveLocally);
}

void SongObject::notifyReady() {
	sender()->deleteLater();
	songIsDownloading = false;
	GoDataSource *ds = new GoDataSource();
	ds->setConnectionName("updater");
	QString query = QString("update song set downloadCompleted = 1 where id = '%1'").arg(getId());
	ds->setQuery(query);
	ds->runQuery();
	// this query will just fail for radio songs.
	qDebug() << "GMA: Song ready to play: " << song().value("title").toString();
	emit songProgress(0);
	emit songReadyToPlay();
	//emit imageChanged();
	//emit imageThumbChanged();
}

void SongObject::getAlbumArts() {
	QVariant url = song().value("albumArtUrl");
	if (artIsDownloading || (!url.isNull() && url.toString().contains("Unknown"))) {
		return;
	}

	artIsDownloading = true;
	bool isRadioUrl = false;

	if (url.isNull()) {
		isRadioUrl = !song().value("albumArtRef").isNull();
	}

	QDir tempStorage(QString(QDir::homePath() + "/albumArt/"));
	if (!tempStorage.exists())
		QDir().mkpath(tempStorage.absolutePath());

	QString artName = QString("%1 - %2").arg(song().value("artist").toString()).arg(song().value("album").toString());
	if (artName.contains("/")) {
		artName = artName.replace("/", "-");
	}
	QString fileLocation = QString(QDir::homePath() + "/albumArt/%1.png").arg(artName);
	QFile imageFile(fileLocation);

	if (!imageFile.exists()) {
		if (isRadioUrl) {
			url = song().value("albumArtRef").toList().at(0).toMap().value("url");
			GoogleMusicApi *largeApi = new GoogleMusicApi;
			QThread *largeThread = new QThread(this);
			largeApi->moveToThread(largeThread);
			largeApi->setAlbumArtUrl(url.toString());

									//qDebug() << "GMA: Getting large Album Art";
			largeApi->toPassToUrlCon = new QFile(fileLocation);
			connect(largeThread, SIGNAL(started()), largeApi, SLOT(getAlbumArt()));
			connect(largeApi, SIGNAL(albumArtData(QByteArray)), this, SLOT(onReceivedAlbumArt(QByteArray)), Qt::QueuedConnection);
			connect(this, SIGNAL(cancelDLS()), largeApi, SLOT(cancelDownloads()), Qt::QueuedConnection);
			connect(largeApi, SIGNAL(complete()), largeThread, SLOT(quit()));
			connect(largeApi, SIGNAL(failed()), this, SLOT(albumArtFailed()), Qt::QueuedConnection);
			connect(largeApi, SIGNAL(failed()), largeApi, SLOT(deleteLater()));
			connect(largeApi, SIGNAL(failed()), largeThread, SLOT(quit()));
			connect(largeThread, SIGNAL(finished()), largeThread, SLOT(deleteLater()));

			largeThread->start();
		} else {
			QString moddedUrl = url.toString().replace("=s130", "=s512");
			GoogleMusicApi *largeApi = new GoogleMusicApi;
			QThread *largeThread = new QThread(this);
			largeApi->moveToThread(largeThread);
			largeApi->setAlbumArtUrl(moddedUrl);

						//qDebug() << "GMA: Getting large Album Art";
			largeApi->toPassToUrlCon = new QFile(fileLocation);
			connect(largeThread, SIGNAL(started()), largeApi, SLOT(getAlbumArt()));
			connect(largeApi, SIGNAL(albumArtData(QByteArray)), this, SLOT(onReceivedAlbumArt(QByteArray)), Qt::QueuedConnection);
			connect(largeApi, SIGNAL(complete()), largeApi, SLOT(deleteLater()));
			connect(this, SIGNAL(cancelDLS()), largeApi, SLOT(cancelDownloads()), Qt::QueuedConnection);
			connect(largeApi, SIGNAL(complete()), largeThread, SLOT(quit()));
			connect(largeApi, SIGNAL(failed()), this, SLOT(albumArtFailed()), Qt::QueuedConnection);
			connect(largeApi, SIGNAL(failed()), largeApi, SLOT(deleteLater()));
			connect(largeApi, SIGNAL(failed()), largeThread, SLOT(quit()));
			connect(largeThread, SIGNAL(finished()), largeThread, SLOT(deleteLater()));

			largeThread->start();

			getAlbumArtThumb();
		}
	} else {
		imageFile.open(QIODevice::ReadOnly);
		if (m_albumArt) {
			free(m_albumArt);
			m_albumArt = NULL;
		}
		m_albumArt = new Image(imageFile.readAll());
		imageFile.close();
		artIsDownloading = false;
		emit imageChanged();
		if (isRadioUrl) {
			m_albumArtThumb = QUrl(fileLocation);
			emit imageThumbChanged();
		}
	}
}

void SongObject::getAlbumArtThumb() {
	QVariant url = song().value("albumArtUrl");
	if (!url.isNull() && !artIsDownloading) {
		QString artName = QString("%1 - %2").arg(song().value("artist").toString()).arg(song().value("album").toString());
		if (artName.contains("/")) {
			artName = artName.replace("/", "-");
		}
		QString fileLocation = QString(QDir::homePath() + "/albumArt/%1_thumb.png").arg(artName);
		QFile imageFile(fileLocation);

		artIsDownloading = true;

		if (!imageFile.exists()) {
			QString albumArtUrl = QString("http:%1").arg(url.toString());
			GoogleMusicApi *smallApi = new GoogleMusicApi;
			QThread *smallThread = new QThread(this);
			smallApi->moveToThread(smallThread);
			smallApi->setAlbumArtUrl(albumArtUrl);

			smallApi->toPassToUrlCon = new QFile(fileLocation);

			//qDebug() << "GMA: Getting small album art";

			connect(smallThread, SIGNAL(started()), smallApi, SLOT(getAlbumArt()));
			connect(smallApi, SIGNAL(albumArtData(QByteArray)), this, SLOT(onReceivedAlbumArtThumb(QByteArray)), Qt::QueuedConnection);
			connect(this, SIGNAL(cancelDLS()), smallApi, SLOT(cancelDownloads()), Qt::QueuedConnection);
			connect(smallApi, SIGNAL(complete()), smallThread, SLOT(quit()));
			connect(smallApi, SIGNAL(failed()), this, SLOT(albumArtFailed()), Qt::QueuedConnection);
			connect(smallApi, SIGNAL(failed()), smallApi, SLOT(deleteLater()));
			connect(smallApi, SIGNAL(failed()), smallThread, SLOT(quit()));
			connect(smallThread, SIGNAL(finished()), smallThread, SLOT(deleteLater()));

			smallThread->start();
		} else {
			m_albumArtThumb = QUrl(fileLocation);
			artIsDownloading = false;
			emit imageThumbChanged();
		}
	}
}

void SongObject::retryAlbumArts() {
	if (!artIsDownloading) {
		QString artName = QString("%1 - %2").arg(song().value("artist").toString()).arg(song().value("album").toString());
		if (artName.contains("/")) {
			artName = artName.replace("/", "-");
		}
		QString fileLocation = QString(QDir::homePath() + "/albumArt/%1.png").arg(artName);
		QFile imageFile(fileLocation);
		if (imageFile.exists()) {
			imageFile.remove();
		}
		fileLocation = QString(QDir::homePath() + "/albumArt/%1_thumb.png").arg(artName);
		QFile thumbFile(fileLocation);
		if (thumbFile.exists()) {
			thumbFile.remove();
		}

		getAlbumArts();
	}
}

void SongObject::onReceivedAlbumArt(const QByteArray &data) {
	Q_UNUSED(data);
	if (m_albumArt) {
		free(m_albumArt);
		m_albumArt = NULL;
	}
	QString artName = QString("%1 - %2").arg(song().value("artist").toString()).arg(song().value("album").toString());
	if (artName.contains("/")) {
		artName = artName.replace("/", "-");
	}
	QString fileLocation = QString(QDir::homePath() + "/albumArt/%1.png").arg(artName);
	QFile imageFile(fileLocation);
	imageFile.open(QIODevice::ReadOnly);
	m_albumArt = new Image(imageFile.readAll());
	imageFile.close();
	artIsDownloading = false;
	emit imageChanged();
}

void SongObject::onReceivedAlbumArtThumb(const QByteArray &data) {
	Q_UNUSED(data);
	QString artName = QString("%1 - %2").arg(song().value("artist").toString()).arg(song().value("album").toString());
	if (artName.contains("/")) {
		artName = artName.replace("/", "-");
	}
	QString fileLocation = QString(QDir::homePath() + "/albumArt/%1_thumb.png").arg(artName);
	m_albumArtThumb = QUrl(fileLocation);
	artIsDownloading = false;
	emit imageThumbChanged();
	sender()->deleteLater();
}

void SongObject::albumArtFailed() {
	artIsDownloading = false;
}

void SongObject::thumbUp() {
	// 5
	m_song.insert("rating", 5);
	QSettings settings;
	if (!settings.value("ThumbsUpLocal").isNull()) {
		m_song.insert("shouldBeLocal", 1);
	}
	emit textChanged();
	metaSync();
}

void SongObject::thumbDown() {
	// 1
	m_song.insert("rating", 1);
	// emit goToNextSong and start playing, after the delay of stuff: set it as the start
	// I only need to worry about updating nextSong's index path when the currPlaylist
	// is thumbsUp
	QSettings settings;
	if (!settings.value("ThumbsUpLocal").isNull()) {
		m_song.insert("shouldBeLocal", 0);
	}
	if (m_song.value("shouldBeLocal").toBool()) {
		m_song.insert("shouldBeLocal", 0);
	}
	emit goToNextSong();
	metaSync();
}

void SongObject::removeThumb() {
	// 0
	m_song.insert("rating", 0);
	emit textChanged();
	metaSync();
}

void SongObject::metaSync() {
	QThread *metaThread = new QThread(this);
	SyncTask *metaApi = new SyncTask(m_song);

	metaApi->moveToThread(metaThread);

	//QtConcurrent::run(metaApi, &GoogleMusicApi::syncMetaData, m_song);

	connect(metaThread, SIGNAL(started()), metaApi, SLOT(syncMetaData()));
	connect(metaApi, SIGNAL(syncComplete()), metaApi, SLOT(deleteLater()));
	connect(metaApi, SIGNAL(syncComplete()), metaThread, SLOT(quit()));
	connect(metaThread, SIGNAL(finished()), metaThread, SLOT(deleteLater()));

	int size = m_song.size();

	metaThread->start();

	GoDataSource *data = new GoDataSource();
	data->setConnectionName("songObj");
	m_song.insert("metaSynced", 0);
	data->updateSong(m_song);

	data->deleteLater();

	emit updatePlaylist("Thumbs Up", song());
}


void SongObject::updateDownloadProgress(qint64 currSize, qint64 finalSize) {
	if (currSize != 0 && finalSize != 0) {
		currentPercentDownloaded = (int)((currSize * 100) / finalSize);
		if (currentPercentDownloaded > 15) {
			//qDebug() << "GMA: This damn song " << song().value("title").toString() << "better start!";
			emit songReadyToPlay();
		}
		emit songProgress(currentPercentDownloaded);
	}
}

void SongObject::updateDownloadProgressMediator(int perc) {
	emit songProgress(perc);
}

Image SongObject::albumArt() const {
	return *m_albumArt;
}

QUrl SongObject::albumArtThumb() const {
	return m_albumArtThumb;
}

QString SongObject::getId() {
	if (!song().value("nid").isNull()) {
		return song().value("nid").toString();
	}
	return song().value("id").toString();
}

QString SongObject::nextSongLabel() {
	return QString("%1 - %2").arg(song().value("title").toString()).arg(song().value("artist").toString());
}

bool SongObject::isLocal() const {
	return local;
}

void SongObject::isAllAccess(bool value) {
	allAccess = value;
}

QVariantList *SongObject::getIndexPath() {
	return m_indexPath;
}

QString SongObject::getUrlLocation() {
	//qDebug() << "GMA: URL Location: " << urlLocation;
	if (urlLocation.contains("?")) {
		urlLocation = urlLocation.replace("?", "");
	}
	return urlLocation;
}

void SongObject::setUrlLocation(QString loc) {
	urlLocation = QString(loc);
}

void SongObject::onError(QString title, QString message) {
	emit error(title, message);
}

void SongObject::cancelDownloads() {
	//qDebug() << "GMA: SongObject cancel downloads: " << song().value("title").toString();
	if (songIsDownloading) {
		emit cancelDLS();
		QDir tempStorage(QString(QDir::homePath() + "/temp/"));
		QDirIterator it(QDir::homePath() + "/temp/", QDirIterator::NoIteratorFlags);
		while (it.hasNext()) {
			QString filename = it.next();
			if (filename.contains(getId())) {
				QString tempname = filename.replace(QDir::homePath() + "/temp/", "");
				tempStorage.remove(tempname);
			}
		}
	}
	emit songProgress(0);
	songIsDownloading = false;
}

SongObject::~SongObject() {
	if (apiThread) {
		apiThread->quit();
	}
	emit cancelDLS();
	delete m_albumArt;
}

