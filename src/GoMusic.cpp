// Navigation pane project template
#include "GoMusic.hpp"

#include <bb/cascades/Application>
#include <bb/cascades/QmlDocument>
#include <bb/cascades/AbstractPane>
#include <bb/cascades/TabbedPane>
#include <bb/cascades/Tab>

#include <bb/cascades/controls/container.h>
#include <bb/cascades/SceneCover>

#include <bb/data/JsonDataAccess>
#include <bb/cascades/Window>
#include <bps/virtualkeyboard.h>
#include <QSettings>
#include <bb/cascades/WebView>
#include <bb/cascades/WebStorage>
#include <bb/cascades/WebCookieJar>
#include <bb/system/InvokeRequest>
#include <bb/system/InvokeManager>
#include <bb/PpsObject>


#include "SyncTask.h"
#include "NetworkListener.h"
#include "GoDataSource.h"
#include "UpdateDBWithLocal.h"

#include <time.h>

#include <CrashReportModel.h>

using namespace bb::cascades;
using namespace bb::data;
using namespace bb::multimedia;

/**
 * This is going to be focused around login,
 * and music playing/stopping management as well as navigation
 */

GoMusic::GoMusic(bb::cascades::Application *app)
: QObject(app),
  mapOfOriginals(new QMap< QString, QObject*>),
  currentPlaylist(new GroupDataModel(this)),
  shufflePlaylist(new GroupDataModel(this)),
  buildOnOne(true),
  m_playlists(QVariantList()),
  m_currSong(new SongObject),
  m_nextSong(new SongObject),
  prevSong(new SongObject),
  shuffleBool(false),
  m_repeatAll(false),
  m_repeatOne(false),
  m_currTab("songs"),
  isLastSong(false),
  lastSongHasPlayed(false),
  isPiggyBacked(false),
  isRadioPlaylist(false),
  radios(new GroupDataModel(this))
{
	shufflePlaylist->setGrouping(ItemGrouping::None);
    // create scene document from main.qml asset
    // set parent to created document to ensure it exists for the whole application lifetime
	const char *uri = "mc.cascades";
	qmlRegisterType<SongObject>(uri, 1, 0, "SongObject");
	qmlRegisterType<GoogleMusicApi>(uri, 1, 0, "GoogleMusicApi");
	qmlRegisterType<GoDataSource>(uri, 1, 0, "GoDataSource");
	m_currSong = new SongObject();

	//fixDb();
	mRoot = 0;

	if (QFileInfo(CrashReportModel::coreFilePath()).exists())
	{
	    qDebug() << "Core file exists. Showing 'Crash Handler' UI.";
	    qDebug() << "  " << CrashReportModel::coreFilePath();

	    QmlDocument *qml = QmlDocument::create("asset:///crash.qml").parent(this);

	    CrashReportModel* model = new CrashReportModel(this);
	    qml->setContextProperty("model", model);

	    // Create root object for the UI
	    mRoot = qml->createRootObject<AbstractPane>();

	    // Set created root object as the application scene
	    app->setScene(mRoot);

	    mRoot->setParent(this);
	} else {
	        // The startApplication method should be implemented
	        // to start the application in the normal way.
	    startApplication();
	}
}

void GoMusic::startApplication() {
    QmlDocument *qml = QmlDocument::create("asset:///main.qml").parent(this);
    qml->setContextProperty("_app", this);

    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));

    QmlDocument *aFrame = QmlDocument::create("asset:///ActiveFrame.qml").parent(this);
    aFrame->setContextProperty("_app", this);
    Container *pane = aFrame->createRootObject<Container>();
    SceneCover *cover = SceneCover::create();
    cover->setContent(pane);
    Application::instance()->setCover(cover);

    data = new GoDataSource;
    data->setConnectionName("main");
    m_downloadPlaylistApi = new GoogleMusicApi;

    // create root object for the UI
    mRoot = qml->createRootObject<TabbedPane>();
    player = mRoot->findChild<MediaPlayer*>("player");
    //connect(player, SIGNAL(error(bb::multimedia::MediaError::Type, unsigned int)), this, SLOT(onMediaError(bb::multimedia::MediaError::Type, unsigned int)));
    listener = new HeadphoneListener();
    NetworkListener *netListen = new NetworkListener();
    connect(listener, SIGNAL(headphonesRemoved()), this, SLOT(headphonesRemoved()), Qt::QueuedConnection);
    QThread *listenThread = new QThread(this);
    listener->moveToThread(listenThread);
    listenThread->start();
    nowPlaying = mRoot->findChild<NowPlayingConnection*>("nowPlaying");
    dialog = mRoot->findChild<SystemDialog*>("alertDialog");
    nowPlaying->setOverlayStyle(OverlayStyle::Fancy);

    connect(player, SIGNAL(playbackCompleted()), this, SLOT(playbackCompleteCheck()), Qt::QueuedConnection);

    if (isLoggedIn()) {
        tabPane = (TabbedPane*)mRoot;
        refreshData();
        pushToMain();
    } else {
        virtualkeyboard_show();
    }

    connect(m_currSong, SIGNAL(textChanged()), this, SLOT(signalCurrChanged()));
    connect(m_currSong, SIGNAL(imageChanged()), this, SLOT(signalCurrChanged()));

    QSettings settings;
    QVariant var = settings.value("screenLock");
    if (!var.isNull()) {
        if (var.toBool()) {
            Application::instance()->mainWindow()->setScreenIdleMode(ScreenIdleMode::KeepAwake);
        }
    }

    QVariant only = settings.value("showOnlyLocal");
    if (var.isNull()) {
        settings.setValue("showOnlyLocal", false);
    }

    UpdateDBWithLocal *udbl = new UpdateDBWithLocal();
    QThread *updateThread = new QThread(this);
    udbl->moveToThread(updateThread);
    connect(updateThread, SIGNAL(started()), udbl, SLOT(runUpdate()), Qt::UniqueConnection);
    //connect(updateThread, SIGNAL(started()), udbl, SLOT(runUpdate()));
    connect(udbl, SIGNAL(complete()), updateThread, SLOT(deleteLater()));
    updateThread->start();

    /*
     * So what will be the plan, I want to have a setting for only showing
     * local songs
     * well my query will go from select * from song where deleted = 0 and source = 1 and downloadCompleted = 1 and shouldBeLocal = 1
     * in addition to this query I could spawn a process to go ahead and download the songs that should be completed but aren't
     * but usually if the user is requesting only local songs they want to not download anything so I should show TRUELY what is
     * on the device.  I can run this query as a quick change but I need a process that will take all the songs and find out
     * TRUELY what is on the device and what is not and update the SQL
     */


    // set created root object as a scene
    Application::instance()->setScene(mRoot);
}

void GoMusic::headphonesRemoved() {
	//emit onError("Headphones Removed", "Headphones Removed");
	player->pause();
}

void GoMusic::setCookieJar(QObject *view, const QUrl newUrl) {
    qDebug() << "URL: " << newUrl.toString();
    WebView * webView = qobject_cast<WebView *>(view);
    WebStorage * stor = webView->storage();
    WebCookieJar * cooks = stor->cookieJar();
    QStringList cookies = cooks->cookiesForUrl(QUrl("http://google.com"));
    Q_FOREACH(QString cook, cookies) {
        qDebug() << "COOOKIEEEE: " << cook;
    }
}

void GoMusic::signalCurrChanged() {
	emit currSongChanged();
}

void GoMusic::login(const QString &email, const QString &password, bool shouldLibraryLoad) {
	m_thread = new QThread(this);
	GoogleMusicApi *api = new GoogleMusicApi;

	api->email = new QString(email);
	api->password = new QString(password);

	api->moveToThread(m_thread);

	connect(m_thread, SIGNAL(started()), api, SLOT(login()));
	if (shouldLibraryLoad) {
		connect(api, SIGNAL(complete()), this, SLOT(refreshData()), Qt::QueuedConnection);
	} else {
		connect(api, SIGNAL(complete()), this, SLOT(pushToMain()), Qt::QueuedConnection);
	}
	connect(api, SIGNAL(error(QString,QString)), this, SLOT(onError(QString, QString)), Qt::QueuedConnection);
	connect(api, SIGNAL(failed()), api, SLOT(deleteLater()));
	connect(api, SIGNAL(complete()), api, SLOT(deleteLater()));

	m_thread->start();
	//qDebug() << "GMA: Started GMA thread and logging in...";
}

void GoMusic::setJsonDump(QString response) {
	emit jsonDump(response);
}

void GoMusic::logout() {
	nowPlaying->revoke();

	QSettings settings;
	settings.clear();

	// delete the database
	data->deleteEverything();

	// delete temp storage and I need to delete albumArt
	removeUnusedData();

	QDir tempStorage(QString(QDir::homePath()+"/albumArt/"));
	QDirIterator it(QDir::homePath()+"/albumArt/", QDirIterator::NoIteratorFlags);
	while (it.hasNext()) {
	    QString filename = it.next();
	    QString tempname = filename.replace(QDir::homePath()+"/albumArt/", "");
	    if (!tempname.startsWith(".") && !tempname.startsWith("..")) {
	    	if (tempStorage.remove(tempname)) {
	    		qDebug() << "GMA: Deleting file: " << tempname;
	    	} else {
	    		qDebug() << "GMA: Failed to delete file: " << tempname;
	    	}
	    }
	}
}

void GoMusic::reloginRequest() {
	nowPlaying->revoke();

	QSettings settings;
	settings.clear();

	emit refreshLogin();
}

void GoMusic::refreshData() {
	QThread *updateThread = new QThread(this);
	SyncTask *syncTask = new SyncTask;

	syncTask->moveToThread(updateThread);

	connect(updateThread, SIGNAL(started()), syncTask, SLOT(run()));
	connect(syncTask, SIGNAL(updateProgress(qint64, qint64)), this, SLOT(updatePlaylistDownloadProgress(qint64,qint64)), Qt::QueuedConnection);
	connect(syncTask, SIGNAL(buildingPL()), this, SLOT(fireBuildingPL()), Qt::QueuedConnection);
	connect(syncTask, SIGNAL(jsonDump(QString)), this, SLOT(setJsonDump(QString)), Qt::QueuedConnection);
	connect(syncTask, SIGNAL(stillGettingLibrary()), this, SLOT(showActivityIndicator()), Qt::QueuedConnection);

	connect(syncTask, SIGNAL(syncComplete()), this, SLOT(pushToMain()), Qt::QueuedConnection);
	connect(syncTask, SIGNAL(syncComplete()), syncTask, SLOT(deleteLater()));
	connect(syncTask, SIGNAL(syncComplete()), updateThread, SLOT(quit()));

	// if error occurs
	connect(syncTask, SIGNAL(error(QString, QString)), this, SLOT(onError(QString, QString)), Qt::QueuedConnection);
	connect(syncTask, SIGNAL(failed()), syncTask, SLOT(deleteLater()));
	connect(syncTask, SIGNAL(failed()), updateThread, SLOT(quit()));

	connect(updateThread, SIGNAL(finished()), updateThread, SLOT(deleteLater()));

	updateThread->start();

	//loadRadioStations();
	//qDebug() << "GMA: Starting updateThread...";
}

bool GoMusic::isLoggedIn() {
	QSettings settings;
	QVariant isLoggedIn = settings.value(GoogleMusicApi::IS_LOGGED_IN);
	if (isLoggedIn.isNull()) {
		return false;
	}
	return settings.value(GoogleMusicApi::IS_LOGGED_IN).toBool();
}

void GoMusic::setOriginalModel(const QString key, bb::cascades::DataModel *model) {
	if (mapOfOriginals->value(key) == NULL) {
		GroupDataModel *copy = new GroupDataModel;
		GroupDataModel *cast = qobject_cast<GroupDataModel*>(model);
		copy->setGrouping(cast->grouping());
		copy->setSortingKeys(cast->sortingKeys());
		copy->insertList(cast->toListOfMaps());
		mapOfOriginals->insert(key, copy);
	}
}

void GoMusic::selectedSong(const QVariantList indexPath, const QString mapId, bb::cascades::DataModel *dataModel) {
	 //nowPlaying->revoke();  // stop the music if its playing
	 player->stop();
	 m_currSong->cancelDownloads();
	 isRadioPlaylist = false;

	 if (!mapId.isEmpty() && mapOfOriginals->value(mapId) != NULL) {
		 // we will use the original
		 currentPlaylist = qobject_cast<GroupDataModel*>(mapOfOriginals->value(mapId));
	 } else {
		 // we will use the passed in dataModel
		 GroupDataModel *cast = qobject_cast<GroupDataModel*>(dataModel);
		 currentPlaylist->clear();
		 currentPlaylist->setGrouping(cast->grouping());
		 currentPlaylist->setSortingKeys(cast->sortingKeys());
		 currentPlaylist->insertList(cast->toListOfMaps());
	 }

	 QVariantMap song = dataModel->data(indexPath).toMap();
	 m_currSong->setSong(song);
	 m_currSong->setIndexPath(currentPlaylist->find(song));

	 //onError("DEBUG OUTPUT", "Song ID: " + m_currSong->song().value("id").toString() + " and type: " + m_currSong->song().value("type").toString());

	 // I need to determine if this is a filtered datamodel by search
	 // so I can restore it and just have the searched for song be the starting
	 // point

	 /*if (currentPlaylist == m_thumbsUp) {
		isCurrPLThumbs = true;
	 } else {
	 	isCurrPLThumbs = false;
	 }*/

	 /*if (m_possibleBackupForFiltering->isEmpty()) {
		 currentPlaylist = qobject_cast<GroupDataModel*>(dataModel);
		 m_currSong->setSong(currentPlaylist->data(indexPath).toMap());
		 m_currSong->setIndexPath(indexPath);
	 } else {
		 // This means our indexPath is incorrect for finding the correct starting point
		 // so to find the correct indexPath we need to search for the map
		 GroupDataModel *model = qobject_cast<GroupDataModel*>(dataModel);
		 QVariant songVar = model->data(indexPath);
		 QVariantMap song(songVar.toMap());
		 QVariantList realIndexPath = m_possibleBackupForFiltering->find(song);
		 currentPlaylist = new GroupDataModel(this);
		 currentPlaylist->setGrouping(m_possibleBackupForFiltering->grouping());
		 currentPlaylist->setSortingKeys(m_possibleBackupForFiltering->sortingKeys());
		 currentPlaylist->insertList(m_possibleBackupForFiltering->toListOfMaps());

		 m_currSong->setSong(song);
		 m_currSong->setIndexPath(realIndexPath);
	 }*/

	 setNextAndPrev(true);
	 playCurrentSong();

	/**
	 * And this is where I check playlist
	 * if (new playlist) {
	 * 		set starting song reference and prepare the next song to download at -20 sec from end
	 * } else {
	 * 		if shuffled is selected
	 * 			reshuffle playlist
	 * }
	 *
	 * Maybe I could capture possible playlists?
	 * Cuz the normal flow is
	 * Library can be a playlist
	 * Artist can be a playlist
	 * Album can be a playlist
	 * Playlist can be a playlist
	 *
	 * So knowing that I just have to keep track of the type
	 * then by the type and maybe even an id I can keep
	 * a low profile record of possible playlists they might start.
	 *
	 * So while the user navigates I can be setting the possible playlist
	 * like
	 * setPossiblePlaylist(enum Type, null/or !null idForType)
	 */
}


void GoMusic::playCurrentSong() {
	connect(m_currSong, SIGNAL(imageThumbChanged()), this, SLOT(updateCurrThumbnail()), Qt::UniqueConnection);
	if (!m_currSong->readyToPlay()) {
		qDebug() << "GMA: Its not ready to play";
		connect(m_currSong, SIGNAL(songReadyToPlay()), this, SLOT(playSong()), Qt::UniqueConnection);
		connect(m_currSong, SIGNAL(error(QString,QString)), this, SLOT(onError(QString,QString)), Qt::UniqueConnection);
	} else {
		qDebug() << "GMA: We are going to play it";
		playSong();
	}
}

void GoMusic::playSong() {
	// now this is specific to selecting and starting but what about
	// just caching?
	MediaError::Type error = player->setSourceUrl(m_currSong->getUrlLocation());
	if (error != MediaError::None) {
		qDebug() << "GMA: There was an issue: " << error;
	}
	//player->seekTime(0);

	//qDebug() << "GMA: URL " << m_currSong->getUrlLocation();
	//qDebug() << "GMA: Going to play song: " << m_currSong->song().value("title").toString();
	disconnect(m_nextSong, SIGNAL(songProgress(int)), m_currSong, SLOT(updateDownloadProgressMediator(int)));
	disconnect(prevSong, SIGNAL(songProgress(int)), m_currSong, SLOT(updateDownloadProgressMediator(int)));
	disconnect(m_currSong, SIGNAL(songReadyToPlay()), this, SLOT(playSong()));
	disconnect(m_currSong, SIGNAL(cancelDLS()), m_nextSong, SLOT(cancelDownloads()));
	disconnect(m_currSong, SIGNAL(cancelDLS()), prevSong, SLOT(cancelDownloads()));
	disconnect(m_nextSong, SIGNAL(songReadyToPlay()), this, SLOT(playSong()));
	disconnect(prevSong, SIGNAL(songReadyToPlay()), m_currSong, SLOT(notifyReady()));
	disconnect(m_nextSong, SIGNAL(imageChanged()), this, SLOT(signalCurrChanged()));
	//qDebug() << "GMA: Exiting piggy back mode";
	isPiggyBacked = false;

	connect(m_currSong, SIGNAL(updatePlaylist(QString,QVariantMap)), this, SLOT(rebuildPlaylist(QString,QVariantMap)));
	connect(m_currSong, SIGNAL(goToNextSong()), this, SLOT(playNextSong()));
	// Going forward we will assume the thumb has been downloaded
	nowPlaying->acquire();
	QString thumbP = m_currSong->albumArtThumb().toString();
	if (thumbP.contains("/accounts/")) {
		QUrl thumbPath = QUrl(QString("file://%1").arg(thumbP));
		//qDebug() << "GMA: Thumber " << thumbPath.toString();
		nowPlaying->setIconUrl(thumbPath);
	} else {
		QString fileLocation = QString("file://%1/%2").arg(QDir::currentPath()).arg(thumbP);
		QUrl thumbPath = QUrl(fileLocation);
		//qDebug() << "GMA: Thumber " << thumbPath.toString();
		nowPlaying->setIconUrl(thumbPath);
	}
	nowPlaying->setMetaData(m_currSong->metaData());
	//nowPlaying->setMetaData(metaData());
	if (m_currSong->getId() == m_nextSong->getId() && player->repeatMode() != RepeatMode::Track && isLastSong && lastSongHasPlayed) {
		// this is for when we get to the end of a playlist and we don't have repeatMode
		// the last song will have the same urlLocation as its nextSong and we will not be in repeat mode Track
	} else {
		if (isLastSong) {
			lastSongHasPlayed = true;
		} else {
			m_nextSong->readyToPlay();
		}
		qDebug() << "GMA: We are going to play!";
		error = player->play();
		if (error != MediaError::None) {
			qDebug() << "GMA: Source : " << player->sourceUrl();
			qDebug() << "GMA: There was an issue: " << error;
		}
	}
}

void GoMusic::updateCurrThumbnail() {
	QString thumbP = m_currSong->albumArtThumb().toString();
	if (thumbP.contains("/accounts/")) {
		QUrl thumbPath = QUrl(QString("file://%1").arg(thumbP));
		//qDebug() << "GMA: Thumber " << thumbPath.toString();
		nowPlaying->setIconUrl(thumbPath);
	} else {
		QString fileLocation = QString("file://%1/%2").arg(QDir::currentPath()).arg(m_currSong->albumArtThumb().toString());
		QUrl thumbPath = QUrl(fileLocation);
		//qDebug() << "GMA: Thumber " << thumbPath.toString();
		nowPlaying->setIconUrl(thumbPath);
	}
}

void GoMusic::getNextSong() {
	if (!m_nextSong->readyToPlay()) {
		// connect the download stuff up for next
	}
}

void GoMusic::setNextAndPrev(bool regeneratePlaylist) {
	/*	if (regeneratePlaylist) {
		generatePlaylist();
	}
	*/
	if (shuffleBool && regeneratePlaylist) {
		// regeneratePlaylist means we selected a different starting
		// song
		shuffle(false);
	}

	setupNext();

	setupPrevious();

	removeUnusedData();
}

void GoMusic::setupNext() {
	if (m_repeatOne) {
		m_nextSong->setIndexPath(*m_currSong->getIndexPath());
		m_nextSong->setSong(m_currSong->song());
		return;
	} else {
		QVariantList nextIndexPath;
		QVariantMap nextMap;
		if (shuffleBool) {
			nextIndexPath = shufflePlaylist->before(*m_currSong->getIndexPath());
		} else {
			nextIndexPath = currentPlaylist->after(*m_currSong->getIndexPath());
		}
		if (nextIndexPath.isEmpty() && m_repeatAll) {
			if (shuffleBool) {
				nextIndexPath = shufflePlaylist->last();
			} else {
				nextIndexPath = currentPlaylist->first();
			}
		} else if (nextIndexPath.isEmpty()) {
			// then the currSong = lastSong in PL
 			if (isRadioPlaylist) {
				loadMoreSongsForRadioStation(radioId, radioSeed);
			} else {
				m_nextSong->setIndexPath(*m_currSong->getIndexPath());
				m_nextSong->setSong(m_currSong->song());
				isLastSong = true;
			}
			return;
		}

		if (shuffleBool) {
			nextMap = shufflePlaylist->data(nextIndexPath).toMap();
		} else {
			nextMap = currentPlaylist->data(nextIndexPath).toMap();
		}

		isLastSong = false;
		m_nextSong->setIndexPath(nextIndexPath);
		m_nextSong->setSong(nextMap);
	}
}

void GoMusic::setupPrevious() {
	if (m_repeatOne) {
		prevSong->setIndexPath(*m_currSong->getIndexPath());
		prevSong->setSong(m_currSong->song());
		return;
	} else {

		QVariantList prevIndexPath;
		QVariantMap prevMap;
		if (shuffleBool) {
			prevIndexPath = shufflePlaylist->after(*m_currSong->getIndexPath());
		} else {
			prevIndexPath = currentPlaylist->before(*m_currSong->getIndexPath());
		}
		if (prevIndexPath.isEmpty() && m_repeatAll) {
			if (shuffleBool) {
				prevIndexPath = shufflePlaylist->first();
			} else {
				prevIndexPath = currentPlaylist->last();
			}
		} else if (prevIndexPath.isEmpty()) {
			prevSong->setIndexPath(*m_currSong->getIndexPath());
			prevSong->setSong(m_currSong->song());
			return;
		}

		if (shuffleBool) {
			prevMap = shufflePlaylist->data(prevIndexPath).toMap();
		} else {
			prevMap = currentPlaylist->data(prevIndexPath).toMap();
		}

		prevSong->setSong(prevMap);
		prevSong->setIndexPath(prevIndexPath);
	}
}

void GoMusic::shuffle(bool doAll) {
	// if currentPlaylist == null, then set currentPL = possiblePL
	// else we have a currentPlaylist to shuffle and
	// we have a song selected
	// but for shits and giggles we will do an
	// if selectedSong != null or whatever
	/*
	 * Our currSongIndexPath is our starting point
	 */
	shuffleBool = true;

	// shuffle
	QList<QVariantMap> data = currentPlaylist->toListOfMaps();
	if (!shufflePlaylist->isEmpty()) {
		shufflePlaylist->clear();
	}

	int size = currentPlaylist->size();

	unsigned int t = time(NULL);
	//qDebug() << "GMA: Shuffle Start: " << t;
	srand(t);
	if (!doAll) {
		int indexOfCurrSong = data.indexOf(m_currSong->song(), 0);

		data.swap(0, indexOfCurrSong);
	} else {
		int swap = rand() % (size - 1);
		data.swap(0, swap);
	}
	for (int position = size-1; position != 0; position--) {
		int swapPosition = rand() % position + 1; // I have thought this through
													// it doesnt make sense at first glance but it does in the end
		//qDebug() << "GMA: Position: " << position << " swap Position: " << swapPosition;
		data.swap(position, swapPosition);
	}

	shufflePlaylist->insertList(data);

	//qDebug() << "GMA: Shuffle End: " << time(NULL);
	QVariant var = shufflePlaylist->data(shufflePlaylist->last());
	QVariantMap songMap = var.toMap();

	m_currSong->setIndexPath(shufflePlaylist->last());
	if (doAll) {
		m_currSong->setSong(songMap);
	}

	setNextAndPrev(false);

	if (doAll) {
		playCurrentSong();
	}

	/**
	 * My algorithm will be weighing the songs by:
	 * - genre
	 * - rating
	 * - artist
	 *
	 * Also check this out, may be helpful
	 * http://keyj.emphy.de/balanced-shuffle/
	 */
}

void GoMusic::shuffle(const QString mapId, bb::cascades::DataModel *modelToShuffle) {
	player->stop();
	m_currSong->cancelDownloads();

	if (!mapId.isEmpty() && mapOfOriginals->value(mapId) != NULL) {
		// we will use the original
		currentPlaylist = qobject_cast<GroupDataModel*>(mapOfOriginals->value(mapId));
	} else {
		// we will use the passed in dataModel
		GroupDataModel *cast = qobject_cast<GroupDataModel*>(modelToShuffle);
		currentPlaylist->clear();
		currentPlaylist->setGrouping(cast->grouping());
		currentPlaylist->setSortingKeys(cast->sortingKeys());
		currentPlaylist->insertList(cast->toListOfMaps());
	}

	shuffle(true);
}



void GoMusic::unshuffle() {
	shuffleBool = false;
	QVariantList indexPath = currentPlaylist->find(m_currSong->song());
	m_currSong->setIndexPath(indexPath);

	setNextAndPrev(false);
}

bool GoMusic::isShuffled() const {
	return shuffleBool;
}

void GoMusic::toggleRepeat() {
	if (!m_repeatAll && !m_repeatOne && !isRadioPlaylist) {
		m_repeatAll = true;
		if (currentPlaylist->size() == 1) {
			player->setRepeatMode(RepeatMode::Track);
		}
	} else if (m_repeatAll || isRadioPlaylist){
		m_repeatAll = false;
		m_repeatOne = true;
		m_nextSong->cancelDownloads();
		player->setRepeatMode(RepeatMode::Track);
	} else {
		m_repeatOne = false;
		player->setRepeatMode(RepeatMode::None);
	}

	setNextAndPrev(false);
	emit currSongChanged();
}

bool GoMusic::repeatAll() const {
	return m_repeatAll;
}

bool GoMusic::repeatOne() const {
	return m_repeatOne;
}

void GoMusic::playPreviousSong() {
	if (player->position() > 5000) {
		player->seekTime(0);
	} else {
		playSibling(prevSong);
	}

}

void GoMusic::loadRadioStations() {
	GoogleMusicApi *radiosApi = new GoogleMusicApi;

	connect(radiosApi, SIGNAL(radioStations(QVariantList)), this, SLOT(setRadioStations(QVariantList)));
	connect(radiosApi, SIGNAL(error(QString,QString)), this, SLOT(onError(QString,QString)), Qt::QueuedConnection);

	radiosApi->loadRadios();
}

void GoMusic::setRadioStations(QVariantList listOfMaps) {
	radios->clear();
	radios->insertList(listOfMaps);
	emit radioStationsLoaded();
	sender()->deleteLater();
}

bb::cascades::DataModel* GoMusic::radioStations() const {
	return radios;
}

void GoMusic::loadRadioStation(QString id, QVariant seed) {
	GoogleMusicApi *radioApi = new GoogleMusicApi;

	connect(radioApi, SIGNAL(radioPlaylist(QVariantList)), this, SLOT(setRadioPlaylist(QVariantList)));
	connect(radioApi, SIGNAL(error(QString,QString)), this, SLOT(onError(QString,QString)), Qt::QueuedConnection);


	isRadioPlaylist = true;
	shuffleBool = false;
	// TODO: the correct logic for repeat stuff

	radioId = id;
	radioSeed = seed;

	radioApi->getRadio(id, seed, QList<QVariantMap>());
}

void GoMusic::setRadioPlaylist(QVariantList playlist) {
	player->stop();
	m_currSong->cancelDownloads();

	currentPlaylist->clear();

	QVariantList pl;
	for (int i = 0; i < playlist.length(); i++) {
		QVariantMap songMap = playlist.at(i).toMap();
		songMap.insert("orderOfInsert", i);
		pl.append(songMap);
	}

	currentPlaylist->insertList(pl);
	currentPlaylist->setSortingKeys(QStringList() << "orderOfInsert");
	QList<QVariantMap> play = currentPlaylist->toListOfMaps();
	QString titles = "";
	Q_FOREACH(QVariant song, play) {
		QVariantMap songMap = song.toMap();
		titles.append(songMap.value("title").toString() + " ");
		QVariantList indexPath = currentPlaylist->find(songMap);
		int part1 = indexPath.at(0).toInt();
		int part2 = indexPath.at(1).toInt();
		titles.append(QString("(%1, %2), ").arg(part1).arg(part2));
	}
	qDebug() << "GMA: Current Playlist : " << titles;

	QVariantList indexPath = currentPlaylist->first();
	QVariantMap song = currentPlaylist->data(indexPath).toMap();
 	m_currSong->setSong(song);
	m_currSong->setIndexPath(indexPath);

	setNextAndPrev(true);
	playCurrentSong();
	sender()->deleteLater();
}

void GoMusic::loadMoreSongsForRadioStation(QString id, QVariant seed) {
	GoogleMusicApi *radioApi = new GoogleMusicApi;

	connect(radioApi, SIGNAL(radioPlaylist(QVariantList)), this, SLOT(addToCurrentRadioStation(QVariantList)));
	connect(radioApi, SIGNAL(error(QString,QString)), this, SLOT(onError(QString,QString)), Qt::QueuedConnection);

	radioApi->getRadio(id, seed, currentPlaylist->toListOfMaps());

	/*
	 * GetRadioTask* task = new GetRadioTask(id, seed, currentPlaylist->toListOfMaps());
	 *
	 * QThreadPool::globalInstance()->start(task);
	 */
}

void GoMusic::addToCurrentRadioStation(QVariantList playlist) {
	// Need to remove the trigger item
	QVariantList triggerIndexPath = currentPlaylist->first();
	QVariant triggerSong = currentPlaylist->data(triggerIndexPath);
	QVariantMap triggerMap = triggerSong.toMap();
	QString triggerSongId = triggerMap.value("nid").toString();
	QVariant songToRemove;
	int i = m_currSong->song().value("orderOfInsert").toInt() + 1;
	QVariantList play;
	Q_FOREACH(QVariant song, playlist) {
		QVariantMap songMap = song.toMap();
		songMap.insert("orderOfInsert", i);
		play.append(songMap);
		i++;
	}

	Q_FOREACH(QVariant song, play) {
		QVariantMap songMap = song.toMap();
		QString songId = songMap.value("nid").toString();
		if (songId.contains(triggerSongId)) {
			songToRemove = songMap;
			break;
		}
	}

	if (!songToRemove.isNull()) {
		play.removeOne(songToRemove);
	}

	currentPlaylist->insertList(play);
	QList<QVariantMap> pl = currentPlaylist->toListOfMaps();
	QString titles = "";
	Q_FOREACH(QVariant song, pl) {
		QVariantMap songMap = song.toMap();
		titles.append(songMap.value("title").toString() + " ");
		QVariantList indexPath = currentPlaylist->find(songMap);
		int part1 = indexPath.at(0).toInt();
		int part2 = indexPath.at(1).toInt();
		titles.append(QString("(%1, %2), ").arg(part1).arg(part2));
	}
	qDebug() << "GMA: Current Playlist : " << titles;

	setNextAndPrev(true);
	m_nextSong->readyToPlay();
	sender()->deleteLater();
}

void GoMusic::createRadioWithCurrSong() {
	//GoogleMusicApi *ass = new GoogleMusicApi;
	//ass->getRadio(m_currSong->getId());
}

bool GoMusic::isRadioSong() const {
	return isRadioPlaylist;
}

void GoMusic::addCurrSongToPlaylist(QString id) {
	/*
	 * To add any song to a playlist, these are the required fields:
	 * clientId
	 * creationTimestamp
	 * deleted
	 * lastModifiedTimestamp
	 * playlistId
	 * precedingEntryId
	 * source -> 1 library 2 for all access not in library
	 * trackId -> currSong.getId()
	 */
}

// This is for the player connection to the playbackCompleted()
void GoMusic::playNextSong() {
	if (!isPiggyBacked) {
		playNextSong(false);
	}
}

void GoMusic::playNextSong(bool checkIfStillPlaying) {
	// this condition would arise if the download is taking longer than 20secs
	qDebug("GMA: Play Next Song...");
	if (checkIfStillPlaying && !isPiggyBacked) {
		if (nowPlaying->position() == nowPlaying->duration()) {
			playSibling(m_nextSong);
		}
	} else if (!isPiggyBacked){
		playSibling(m_nextSong);
	}
}

void GoMusic::playSibling(SongObject* newCurr) {
	//nowPlaying->revoke();
	player->stop();
	//player->seekTime(0);
	qDebug() << "GMA: Stopped the player and seeked time to 0";
 	if (isPiggyBacked) {
		m_nextSong->cancelDownloads();
	}
	m_currSong->cancelDownloads();
	m_currSong->setIndexPath(*newCurr->getIndexPath());
	m_currSong->setSong(newCurr->song());

	if (newCurr->isSongDownloading()) {
		// This way I am piggy backing off the nextSong's thread until its complete
		isPiggyBacked = true;
		qDebug() << "GMA: Entering piggy back mode";
		connect(newCurr, SIGNAL(songProgress(int)), m_currSong, SLOT(updateDownloadProgressMediator(int)), Qt::UniqueConnection);
		connect(newCurr, SIGNAL(songReadyToPlay()), this, SLOT(playSong()), Qt::UniqueConnection);
		connect(newCurr, SIGNAL(imageChanged()), this, SLOT(signalCurrChanged()), Qt::UniqueConnection);
		connect(m_currSong, SIGNAL(cancelDLS()), newCurr, SLOT(cancelDownloads()), Qt::UniqueConnection);
		connect(m_currSong, SIGNAL(songReadyToPlay()), this, SLOT(playSong()));

		m_currSong->setUrlLocation(newCurr->getUrlLocation());
		m_currSong->getAlbumArts();
		m_currSong->songIsDownloading = true;
	}

	if (isLastSong && lastSongHasPlayed) {
		// don't play a thing
		isLastSong = false;
		lastSongHasPlayed = false;
		// reset the values to avoid issues
	} else {
		setNextAndPrev(false);
		playCurrentSong();
	}

}

// This is called after a login
void GoMusic::pushToMain() {
	// sent to QML
	QSettings settings;
	QVariant loggedIn = settings.value(GoogleMusicApi::IS_LOGGED_IN);
	m_playlists.clear();
	int i = 0;
	Q_FOREACH(QVariant playlist, data->getAllPlaylists()) {
		m_playlists.insert(i, playlist.toMap());
		i++;
	}
	if (loggedIn.isNull()) {
		settings.setValue(GoogleMusicApi::IS_LOGGED_IN, true);
		emit loginComplete();
	} else {
		emit playlistsUpdated();
		//refreshData();
	}


	// this is bad design
	emit loginRetry();
}

QVariantList GoMusic::playlists() const {
	return m_playlists;
}

SongObject* GoMusic::currSong() const {
	return m_currSong;
}

SongObject* GoMusic::nextSong() const {
	return m_nextSong;
}

QString GoMusic::amountOfStorageAAUsing() const {
	QDir artStorage(QString(QDir::homePath()+"/albumArt/"));
	QDirIterator it(QDir::homePath()+"/albumArt/", QDirIterator::NoIteratorFlags);
	qint64 total(0);
	while (it.hasNext()) {
	   	QString filename = it.next();
	   	QFile imageFile(filename);
	   	total += imageFile.bytesAvailable();
	}

	float totalRounded(0.0);
	if (total > 0) {
		totalRounded = total * 0.000001;
	}

	return QString("Total storage used by Album Art: %1 MB").arg(totalRounded);
}

void GoMusic::rebuildPlaylist(QString id, QVariantMap interestingMap) {
	if (id.contains("Thumbs Up")) {

		emit thumbsUpdated();

		// The point of this is to find out
		// if we need to update currSong and do a setNextAndPrev
		// to get correct indexPaths

	}
}

void GoMusic::getPlaylist(QString id){
	if (id.isEmpty()) {
	} else {
		if (data->isPlaylistEmpty(id)) {
			// if we get here we need to spawn a worker to
			// get the playlist information
			// I say it should be something similar to refresh

		}
	}
	emit possibleUpdated();
}


void GoMusic::downloadPlaylist(QString id, QString location) {
	// I need to consider multiple playlist downloads
	download_playlist_thread = new QThread(this);
	//m_downloadPlaylistApi = new GoogleMusicApi;
	m_downloadPlaylistApi->moveToThread(download_playlist_thread);

	if (id.contains("Thumbs Up")) {
		//m_downloadPlaylistApi->playlistToDownload = m_thumbsUp;
		QSettings settings;
		settings.setValue("ThumbsUpLocal", true);
		QVariantList playlist = data->runQuery("select * from song where rating = 5 and deleted = 0", false);
		m_downloadPlaylistApi->playlistToDownload = playlist;
	} else {
		QVariantList playlist = data->getPlaylistSql(id);
		if (playlist.size() == 0) {
			emit error("Failed To Get Playlist", "Failed to get the songs for this playlist.  If there are songs in the playlist and you get this error notify the developer.");
			return;
		}
		m_downloadPlaylistApi->playlistToDownload = playlist;
	}

	m_downloadPlaylistApi->saveLocationName = location;

	connect(download_playlist_thread, SIGNAL(started()), m_downloadPlaylistApi, SLOT(startPlaylistDownload()));
	connect(m_downloadPlaylistApi, SIGNAL(playlistDownloadComplete()), download_playlist_thread, SLOT(quit()));
	connect(m_downloadPlaylistApi, SIGNAL(error(QString,QString)), this, SLOT(onError(QString,QString)), Qt::QueuedConnection);

	download_playlist_thread->start();

	if (!id.contains("Thumbs Up")) {
		data->setPlaylistAsLocal(id);
	} else {
		data->setPlaylistAsLocalStatic(id);
	}
}

void GoMusic::downloadAlbum(QString query, QString location) {
	// I need to consider multiple playlist downloads
	download_playlist_thread = new QThread(this);
	//m_downloadPlaylistApi = new GoogleMusicApi;
	m_downloadPlaylistApi->moveToThread(download_playlist_thread);

	QVariantList playlist = data->runQuery(query, false);
	m_downloadPlaylistApi->playlistToDownload = playlist;

	m_downloadPlaylistApi->saveLocationName = location;

	connect(download_playlist_thread, SIGNAL(started()), m_downloadPlaylistApi, SLOT(startPlaylistDownload()));
	connect(m_downloadPlaylistApi, SIGNAL(playlistDownloadComplete()), download_playlist_thread, SLOT(quit()));
	connect(m_downloadPlaylistApi, SIGNAL(error(QString,QString)), this, SLOT(onError(QString,QString)), Qt::QueuedConnection);

	download_playlist_thread->start();
}

/** Data Stuff **/

void GoMusic::removeUnusedData() {
	QDir tempStorage(QString(QDir::homePath()+"/temp/"));
	QDirIterator it(QDir::homePath()+"/temp/", QDirIterator::NoIteratorFlags);
	while (it.hasNext()) {
	   	QString filename = it.next();
	    if (!filename.contains(prevSong->getId()) && !filename.contains(m_nextSong->getId()) &&
	    	!filename.contains(m_currSong->getId())) {
	    	QString tempname = filename.replace(QDir::homePath()+"/temp/", "");
	    	if (!tempname.startsWith(".") && !tempname.startsWith("..")) {
	    		if (tempStorage.remove(tempname)) {
	    			//qDebug() << "GMA: Deleting file: " << tempname;
	    		} else {
	    			qDebug() << "GMA: Failed to delete file: " << tempname;
	    		}
	    	}
	    }
	}
}

void GoMusic::deleteAllAccessCache() {
	QDir allAccess(QDir::homePath() + "/allaccess/");
	QDirIterator it(QDir::homePath() + "/allaccess/", QDirIterator::NoIteratorFlags);
	while (it.hasNext()) {
		QString filename = it.next();
		QString exactname = filename.replace(QDir::homePath() + "/allaccess/", "");
		if (!exactname.startsWith(".") && !exactname.startsWith("..")) {
			allAccess.remove(exactname);
		}
	}
}

void GoMusic::updateProgress(qint64 currSize, qint64 actSize) {
	emit updateProgressUI((int)((currSize * 100) / actSize));
}

void GoMusic::updatePlaylistDownloadProgress(qint64 current, qint64 total) {
	emit updatePlaylistDownload(current, total);
}

void GoMusic::setScreenDisabled(bool checked) {
	QSettings settings;
	settings.setValue("screenLock", checked);
	if (checked) {
		Application::instance()->mainWindow()->setScreenIdleMode(ScreenIdleMode::KeepAwake);
	} else {
		Application::instance()->mainWindow()->setScreenIdleMode(ScreenIdleMode::Normal);
	}
}

void GoMusic::setCurrentTab(QString tab) {
	m_currTab = tab;
}

void GoMusic::setSelectedAlbum(QString album) {
	m_currAlbum = album;
}

void GoMusic::setSelectedArtist(QString artist) {
	m_currArtist = artist;
}

void GoMusic::playbackCompleteCheck() {
	int length = m_currSong->song().value("durationMillis").toInt();
	int position = nowPlaying->position();
	int difference = length - position;
	qDebug() << "GMA: Position: " << QString("%1").arg(position) << " Length: " << QString("%1").arg(length);
	qDebug() << "GMA: Playback Check: " << QString("%1").arg(difference);
	if (difference < 3000) {
		playNextSong();
	} else {
		if (!m_currSong->isSongDownloading()) {
			player->seekTime(position-2);
			player->play();
		} else {
			player->pause();
			player->seekTime(position-2);
		}
	}
}

bool GoMusic::isScreenLockDisabled() const {
	QSettings settings;
	QVariant var = settings.value("screenLock");
	if (var.isNull()) {
		return false;
	}
	return var.toBool();
}

bool GoMusic::largeButtonsDisabled() const {
	QSettings settings;
	QVariant var = settings.value("largeButtons");
	if (var.isNull()) {
		return false;
	}
	return var.toBool();
}

void GoMusic::setLargeButtonsDisabled(bool toggle) {
	QSettings settings;
	settings.setValue("largeButtons", toggle);
}

bool GoMusic::showLocalOnly() const {
    QSettings settings;
    QVariant var = settings.value("showLocalOnly", false);
    return var.toBool();
}

void GoMusic::setShowLocalOnly(bool toggle) {
    QSettings settings;
    settings.setValue("showLocalOnly", toggle);
    emit showLocalChanged();
    // this is to refresh the views
    emit loginComplete();
}

void GoMusic::showActivityIndicator() {
	emit showActIndicator();
}

void GoMusic::onMediaError(MediaError::Type type, unsigned int pos) {
	qDebug() << "GMA: Media error: " << type << " : " << pos;
}

GoogleMusicApi* GoMusic::downloadPlaylistApi() const {
	return m_downloadPlaylistApi;
}

void GoMusic::fireBuildingPL() {
	emit buildingPlaylists();
}

void GoMusic::fixDb() {
	/*
	 * New Fixes
	 * nameChanges
	 * we want Id, Nid, clientId and storeId
*/
	GoDataSource *gds = new GoDataSource();
	gds->setConnectionName("fixer");
	gds->runQuery("alter table song add column estimatedSize numeric", false);
	gds->runQuery("alter table playlist add column lastModifiedTimestamp numeric", false);
	gds->runQuery("alter table song add column lastModifiedTimestamp numeric", false);
	gds->runQuery("alter table playlistSongs add column lastModifiedTimestamp numeric", false);
	gds->runQuery("alter table song add column source numeric", false);
	gds->deleteLater();
	/*
	 * Change the type to trackType
	 * track to trackNumber
	 */
}

void GoMusic::onError(QString title, QString message) {
	if (!isLoggedIn()) {
		QSettings settings;
		settings.clear();
		data->deleteEverything();
		emit loginRetry();
	} else {
		if (title.contains("401")) {
			reloginRequest();
		}
	}
	QString theFile = QDir::currentPath() + QDir::separator() + QString("logs/");
	QString fileName = QString("debugLog.txt");
	theFile.append(fileName);

	FILE *file = fopen(theFile.toStdString().c_str(), "w");
	slog2_dump_logs_to_file(file, 0);
	emit error(title, message);
}

void GoMusic::sendLogFile() {
        InvokeRequest request;
        request.setAction("bb.action.COMPOSE");
        request.setMimeType("message/rfc822");
        QVariantMap data;
        data["to"] = (QVariantList() << "mitchellrclay@gmail.com");
        data["subject"] = "GoMusic Log";
        QString fileName = QString("debugLog.txt");
        QString filePath = QDir::currentPath() + QDir::separator() + QString("logs/%1").arg(fileName);
        QString logpath = QDir::currentPath() + QDir::separator() + QString("shared/misc/%1").arg(fileName);
        QFile::remove(logpath);
        QFile::copy(filePath,logpath);
        QString logpathEncoded = QString(QUrl(logpath).toEncoded());
        data["attachment"] = (QVariantList() << logpathEncoded);
        QVariantMap moreData;
        moreData["data"] = data;
        bool ok;
        request.setData(bb::PpsObject::encode(moreData, &ok));
        InvokeManager manager;
        manager.invoke(request);
}

GoMusic::~GoMusic() {
	listener->deleteLater();
	nowPlaying->revoke();
}
