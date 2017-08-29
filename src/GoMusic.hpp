// Default empty project template
#ifndef GoMusic_HPP_
#define GoMusic_HPP_

#include <QObject>
#include <QSet>
#include <bb/multimedia/MediaPlayer>
#include <bb/multimedia/NowPlayingConnection>
#include <bb/cascades/TabbedPane>
#include <bb/cascades/GroupDataModel>
#include <bb/cascades/ArrayDataModel>
#include <bb/cascades/Image>
#include <bb/system/SystemDialog>
#include <bb/cascades/WebView>

#include "SongObject.h"
#include "GoogleMusicApi.h"
#include "GMusicDataSource.h"
#include "GoDataSource.h"
#include "HeadphoneListener.h"
#include "slog2.h"

namespace bb { namespace cascades { class Application; }}
using namespace bb::multimedia;
using namespace bb::cascades;
using namespace bb::system;

/*!
 * @brief Application pane object
 *
 *Use this object to create and init app UI, to create context objects, to register the new meta types etc.
 */
class GoMusic : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList playlists READ playlists CONSTANT)

    Q_PROPERTY(SongObject *currSong READ currSong NOTIFY currSongChanged)
    Q_PROPERTY(SongObject *nextSong READ nextSong NOTIFY currSongChanged)

    Q_PROPERTY(bool isShuffled READ isShuffled CONSTANT)
    Q_PROPERTY(bool repeatAll READ repeatAll NOTIFY currSongChanged)
    Q_PROPERTY(bool repeatOne READ repeatOne NOTIFY currSongChanged)

    Q_PROPERTY(bb::cascades::DataModel *radioStations READ radioStations NOTIFY radioStationsLoaded)
    Q_PROPERTY(bool isRadioSong READ isRadioSong NOTIFY currSongChanged)

    Q_PROPERTY(GoogleMusicApi* downloadPlaylistApi READ downloadPlaylistApi CONSTANT)

    Q_PROPERTY(bool screenLockDisabled READ isScreenLockDisabled WRITE setScreenDisabled)

    Q_PROPERTY(bool largeButtonsDisabled READ largeButtonsDisabled WRITE setLargeButtonsDisabled)

    Q_PROPERTY(bool showLocalOnly READ showLocalOnly WRITE setShowLocalOnly NOTIFY showLocalChanged)
    //Q_PROPERTY(QString amountOfStorageAAUsing READ amountOfStorageAAUsing NOTIFY currSongChanged);

public:
    GoMusic(bb::cascades::Application *app);
    virtual ~GoMusic();
    Q_INVOKABLE bool isLoggedIn();
    Q_INVOKABLE void getNextSong();
    Q_INVOKABLE void shuffle(bool doAll);
    Q_INVOKABLE void shuffle(const QString mapId, bb::cascades::DataModel *modelToShuffle);
    Q_INVOKABLE void unshuffle();
    Q_INVOKABLE void toggleRepeat();
    Q_INVOKABLE void setCookieJar(QObject *webView, const QUrl newUrl);

    // actually sets the possibleplaylist
    Q_INVOKABLE void getPlaylist(QString id);
    Q_INVOKABLE void logout();
    Q_INVOKABLE void deleteAllAccessCache();

    Q_INVOKABLE void setOriginalModel(const QString key, bb::cascades::DataModel *model);

    Q_INVOKABLE void createRadioWithCurrSong();

    Q_INVOKABLE void startApplication();

Q_SIGNALS:
	void loginComplete();

	void updateProgressUI(int);
	void updatePlaylistDownload(qint64,qint64);

	void playlistsUpdated();

	void possibleUpdated();
	void thumbsUpdated();
	void artistsUpdated();
	void albumsUpdated();

	void currSongChanged();

	void radioStationsLoaded();

	void showLocalChanged();

	void error(QString, QString);
	void loginRetry();

	void showActIndicator();

	void buildingPlaylists();

	void jsonDump(QString);

	void refreshLogin();

public Q_SLOTS:
	void setJsonDump(QString);

	void login(const QString &email, const QString &password, bool shouldLibraryLoad);
	void refreshData();
	void pushToMain();

	void selectedSong(const QVariantList, const QString mapId, bb::cascades::DataModel *dataModel);
	void signalCurrChanged();
	void playSong();

	void playPreviousSong();
	void playNextSong();
	void playNextSong(bool);

	void updateCurrThumbnail();

	void updateProgress(qint64,qint64);
	void updatePlaylistDownloadProgress(qint64,qint64);

	void downloadPlaylist(QString id, QString location);
	void downloadAlbum(QString query, QString location);

	void rebuildPlaylist(QString id, QVariantMap interestingMap);

	void onError(QString, QString);

	void onMediaError(bb::multimedia::MediaError::Type type, unsigned int pos);

	void setScreenDisabled(bool checked);
	bool isScreenLockDisabled() const;

	void setLargeButtonsDisabled(bool toggle);
	bool largeButtonsDisabled() const;

	void setShowLocalOnly(bool checked);
	bool showLocalOnly() const;

	bool isRadioSong() const;

	void reloginRequest();

	void setCurrentTab(QString);
	void setSelectedArtist(QString);
	void setSelectedAlbum(QString);

	void showActivityIndicator();

	void fireBuildingPL();

	void loadRadioStations();
	void loadRadioStation(QString, QVariant);
	void loadMoreSongsForRadioStation(QString, QVariant);

	void setRadioStations(QVariantList);
	void setRadioPlaylist(QVariantList);
	void addToCurrentRadioStation(QVariantList);

	void addCurrSongToPlaylist(QString);

	void playbackCompleteCheck();

	void headphonesRemoved();

	void sendLogFile();

private:
    GoDataSource *data;

    // Having C++ access to these is so I can manually manage which
    // song comes next
    MediaPlayer *player;
    NowPlayingConnection *nowPlaying;
    TabbedPane *tabPane;
    SystemDialog *dialog;

    // This thread will have a GoogleMusicApi instance
    // that will only run a refresh on the data
    QThread *m_thread;
    // I need to have dedicated threads for each
    // of my network calls, so I can cancel them at any moment

    QThread *refresh_thread;
    GoogleMusicApi *refreshApi;
    QThread *download_playlist_thread;
    GoogleMusicApi *m_downloadPlaylistApi;
    GoogleMusicApi *downloadPlaylistApi() const;

    QMap< QString, QObject*> *mapOfOriginals;

    // This is the current playlist we will use
    // to move through the songs in the run time
    GroupDataModel *currentPlaylist;
    GroupDataModel *shufflePlaylist;
    bool buildOnOne;

    // This data could or could not change during run time, if it does
    // change it will automatically be updated on the screen
    QVariantList playlists() const;
    QVariantList m_playlists;
    bool isCurrPLThumbs;

    QString possiblePlayList;
    QString playlistId;
    QString albumName;
    QString artistName;

    SongObject *m_currSong;
    SongObject *currSong() const;
    SongObject *m_nextSong;
    SongObject *nextSong() const;
    SongObject *prevSong;

    // Shuffle/Repeat Mode
    bool shuffleBool;

    bool m_repeatAll;
    bool repeatAll() const;
    bool m_repeatOne;
    bool repeatOne() const;

    bool isShuffled() const;

    QString m_currTab;
    QString m_currArtist;
    QString m_currAlbum;

    bool isLastSong;
    bool lastSongHasPlayed;

    bool isPiggyBacked;
    bool isRadioPlaylist;
    QString radioId;
    QVariant radioSeed;

    DataModel* radioStations() const;
    GroupDataModel* radios;

    QString amountOfStorageAAUsing() const;

    HeadphoneListener *listener;

    AbstractPane *mRoot;

private:
    void setNextAndPrev(bool);
    void playCurrentSong();
    bool songIsDownloading(QString id);

    void removeUnusedData();

    void playSibling(SongObject*);

    void setupNext();
    void setupPrevious();

    void fixDb();
};

/*
 * Ok lets have a discussion:
 * First topic will be the Up Next box
 * It gets
 *  - nextSongMeta
 *  - nextSongAlbumArt (small) if I want
 *  - checksOnExistingData
 *   - downloads it if missing or size = 0
 *  - has a timer for when to slide off or slides off after download completes
 *
 * Current Song
 *  - currSongMeta
 *  - currSongAlbumArt (small and large)
 *  - checksOnExistingData
 *   - downlaods it if missing or size = 0, otherwise it plays it
 *  - auto advances at the end of song
 *
 * Prev Song
 *  - prevSongMeta, prevSongAlbumArt (small and large) will be transfered to curr stuff
 *  - checksOnExistingData
 *   - downloads it if missing or size = 0, otherwise it plays it
 *  - auto advances at the end of song
 *
 * So I think we should have some Song Objects.
 * These objects will do the checksOnExisting data stuff
 * and initialize downloads if need be.
 * They will have the signals for download progress and metaData changes.
 *
 * This type of functionality requires that they have
 * a pointer to the currentPlaylist
 * a setIndexPath the will emit that the metaData has changed
 * a single instance of the GoogleMusicApi to do downloads
 * a thread to be used for downloading the GoogleMusicApi?
 * a bool to say if its currently downloading
 * a way to cancel the download (which will go into the Api and interfere with the UrlCon more that likely)
 *
 * Another thing I need to figure out is a list of lists
 */

#endif /* ApplicationUI_HPP_ */
