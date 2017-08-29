// Navigation pane project template
import bb.cascades 1.0
import bb.multimedia 1.0
import bb.system 1.0
import mc.cascades 1.0
import "listitems"

TabbedPane {
    id: tabPane
    objectName: "tabPane"
    showTabsOnActionBar: true
    
    Menu.definition: MenuDefinition {
        actions: [
            ActionItem {
                title: qsTr("Prev")
                imageSource: "asset:///images/backwardsSmall.png"
                onTriggered: {
                    _app.playPreviousSong();
                }
            },
            ActionItem {
                title: qsTr("Settings")
                onTriggered: {
                    settingsSheet.open();
                }
            },
            ActionItem {
                title: qsTr("Play/Pause")
                imageSource: "asset:///images/playSmall.png"
                onTriggered: {
                    if (player.mediaState == MediaState.Started) {
                        nowPlaying.pause();
                    } else if (player.mediaState == MediaState.Paused) {
                        nowPlaying.play();
                    } else {
                        nowPlaying.aquire();
                    }
                }
            },
            ActionItem {
                title: qsTr("Next")
                imageSource: "asset:///images/forwardSmall.png"
                onTriggered: {
                    _app.playNextSong();
                }
            }
        ]
    }

    onActiveTabChanged: {
        if (activeTab == songs) {
            if (_app.currSong.isSongDownloading() || _app.currSong.isAvailable()) {
                activeTab.goToNowPlaying.enabled = true;
            }
        }
        if (activeTab != songs && activeTab != artists && activeTab != albums && activeTab != thumbsUp && activeTab != radio) {
            activeTab.runQuery();
        }
        
        if (activeTab == radio) {
            radioPane.loading.visible = true;
            _app.loadRadioStations();
        }
        var curTab = "";
        if (activeTab == songs) curTab = "songs";
        else if (activeTab == artists) curTab = "artists";
        else if (activeTab == albums) curTab = "albums";
        else if (activeTab == thumbsUp) curTab = "thumbs up";
        else curTab = activeTab.playlistId;
        _app.setCurrentTab(curTab);
    }

    Tab {
        id: songs
        title: qsTr("Songs")
        imageSource: "asset:///images/songs.png"
        
        property alias songsPane: songsPane

        BasicPane {
            id: songsPane
            connectionName: "songs"
            onCreationCompleted: {
                songsPane.filterList.connect(filterSongs);
                dataSource.runQuery();
            }
            listView.onTriggered: {
                if (indexPath.length > 1) {
                	var chosenItem = listView.dataModel.data(indexPath);
                    //goToNowPlaying.enabled = true;
                    _app.selectedSong(indexPath, "library", listView.dataModel);
                    var playP = null;
                    if (! _app.largeButtonsDisabled) {
                        // logic is a little backwards here
                        playP = playPageSmallDef.createObject();
                    } else {
                        playP = playPageDef.createObject();
                    }

                    setPlayPage(playP);
                	//pushPage(playP);
                    
                    navigationPane.backButtonsVisible = true;
                }
                //searchField.text = "";
            }
            onShuffleThis: {
                _app.shuffle("library", listView.dataModel);
                var playP = null;
                if (! _app.largeButtonsDisabled) {
                    // logic is a little backwards here
                    playP = playPageSmallDef.createObject();
                } else {
                    playP = playPageDef.createObject();
                }

                setPlayPage(playP);
                //pushPage(playP);
            }
            function filterSongs(text) {
                var fixed = text.replace("\'", "\'\'");
                var filterQuery = "select * from song where deleted = 0 and title like '%"+fixed+"%'";
                songsPane.query = filterQuery;
                _app.setOriginalModel("library", listView.dataModel);
                listView.dataModel.clear();
                dataSource.runQuery();
                if (text == "") {
                    //_app.removeFromMaps("library");
                }
            }
        }
    }

    Tab {
        id: artists
        title: qsTr("Artists")
        imageSource: "asset:///images/artists.png"
        
        property alias artistPane: artistPane

        BasicPane {
            id: artistPane
            listView.listItemComponents: [
                ListItemComponent {
                    type: "item"
                    ArtistListItem {}
                }
            ]
            listView.onTriggered: {
                var chosenArtist = listView.dataModel.data(indexPath);
                
                var artist = chosenArtist.artist.replace("\'", "\'\'");
                var query = "select distinct album from song where artist = '" + artist + "'";
                var results = generalData.runQuery(query, true);
                if (results.length > 2) {
                    var apage = artistAlbumPageDef.createObject(); // ArtistAlbumPage.qml
                    apage.artistName = chosenArtist.artist;
                    apage.dataSource.connectionName = "artist";
                    apage.dataSource.runQuery(query, true);
                    apage.listModel.grouping = ItemGrouping.None
                    apage.listModel.sortingKeys = ["sortOrder"];

                    apage.pushThis.connect(pushPage);
                    apage.playPage.connect(setPlayPage);
                    
                    pushPage(apage);
                } else {
                    var apage = artistPageDef.createObject();
                    apage.connectionName = "artist";
                    apage.artistName = chosenArtist.artist;
                    apage.query = "select * from song where artist = '" + artist + "'";
                    apage.listModel.grouping = ItemGrouping.None
                    apage.listModel.sortingKeys = ["track"];
                    apage.pushThis.connect(pushPage);
                    apage.playPage.connect(setPlayPage);
                    
                    pushPage(apage);
                }
                navigationPane.backButtonsVisible = true;
            }
            connectionName: "artists"
            query: "select distinct artist from song where deleted = 0"
            sorting: [
                "artist"
            ]
            onCreationCompleted: {
                artistPane.filterList.connect(filterArtists);
                dataSource.runQuery();
            }
            onShuffleThis: {
                _app.shuffle("library", songs.songsPane.listView.dataModel);
                var playP = null;
                if (! _app.largeButtonsDisabled) {
                    // logic is a little backwards here
                    playP = playPageSmallDef.createObject();
                } else {
                    playP = playPageDef.createObject();
                }

                setPlayPage(playP);
                //pushPage(playP);
            }
            function filterArtists(text) {
                var fixed = text.replace("\'", "\'\'");
                var filterQuery = "select distinct artist from song where deleted = 0 and artist like '%"+fixed+"%'";
                artistPane.query = filterQuery;
                listView.dataModel.clear();
                dataSource.runQuery();
            }
        }
    }

    Tab {
        id: albums
        title: qsTr("Albums")
        imageSource: "asset:///images/albums.png"
        
        property alias albumsPane: albumsPane

        BasicPane {
            id: albumsPane
            listView.listItemComponents: [
                ListItemComponent {
                    type: "item"
                    AlbumListItem {}
                }
            ]
            
            listView.onTriggered: {
                var chosenAlbum = listView.dataModel.data(indexPath);
                var apage = artistPageDef.createObject();
                apage.artistName = chosenAlbum.album;
                apage.connectionName = "album";
                apage.query = "select * from song where album = '" + chosenAlbum.album + "' and deleted = 0";
                apage.listModel.grouping = ItemGrouping.None
                apage.listModel.sortingKeys = [ "track" ];

                apage.pushThis.connect(pushPage);
                
                pushPage(apage);
            }
            connectionName: "albums"
            query: "select distinct album from song where deleted = 0"
            sorting: [
                "album"
            ]
            
            onCreationCompleted: {
                albumsPane.filterList.connect(filterAlbums);
                dataSource.runQuery();
            }
            onShuffleThis: {
                _app.shuffle("library", songs.songsPane.listView.dataModel);
                var playP = null;
                if (! _app.largeButtonsDisabled) {
                    // logic is a little backwards here
                    playP = playPageSmallDef.createObject();
                } else {
                    playP = playPageDef.createObject();
                }

                setPlayPage(playP);
            }
            function filterAlbums(text) {
                var fixed = text.replace("\'", "\'\'");
                var filterQuery = "select distinct album from song where deleted = 0 and album like '%"+fixed+"%'";
                albumsPane.query = filterQuery;
                listView.dataModel.clear();
                dataSource.runQuery();
            }
        }
    }
    
    Tab {
        id: radio
        title: qsTr("Radio")
        imageSource: "asset:///images/radio.png"
        
        RadioPane {
            id: radioPane
        }
    }

    Tab {
        id: thumbsUp
        title: qsTr("Thumbs Up")
        imageSource: "asset:///images/thumbsup.png"
        
        property alias thumbsPane: thumbsPane
        
        BasicPane {
            id: thumbsPane
            connectionName: "thumbsUp"
            query: "select * from song where rating = 5 and deleted = 0"
            actions: [
                ActionItem {
                    id: downloadPlaylist
                    title: qsTr("Download Playlist")
                    imageSource: "asset:///images/download.png"
                    enabled: ! dlPlCon.visible
                    onTriggered: {
                        locationDialog.show();
                    }
                }
            ]
            listView.onTriggered: {
                if (indexPath.length > 1) {
                    var chosenItem = listView.dataModel.data(indexPath);
                    //goToNowPlaying.enabled = true;
                    _app.selectedSong(indexPath, "thumbs", listView.dataModel);
                    var playP = null;
                    if (!_app.largeButtonsDisabled) {
                        // logic is a little backwards here
                        playP = playPageSmallDef.createObject();
                    } else {
                        playP = playPageDef.createObject();
                    }

                    setPlayPage(playP);
                    //pushPage(playP);

                    navigationPane.backButtonsVisible = true;
                    //searchField.text = "";
                }
            }
            onShuffleThis: {
                _app.shuffle("thumbs", listView.dataModel);
                var playP = null;
                if (! _app.largeButtonsDisabled) {
                    // logic is a little backwards here
                    playP = playPageSmallDef.createObject();
                } else {
                    playP = playPageDef.createObject();
                }

                setPlayPage(playP);
                //pushPage(playP);
            }

            onCreationCompleted: {
                //basicPage.addAction(downloadPlaylist);
                thumbsPane.filterList.connect(filterThumbs);
                dataSource.runQuery();
            }
            function filterThumbs(text) {
                var fixed = text.replace("\'", "\'\'");
                var filterQuery = "select * from song where deleted = 0 and title like '%" + fixed + "%' and rating = 5";
                thumbsPane.query = filterQuery;
                _app.setOriginalModel("thumbs", listView.dataModel);
                listView.dataModel.clear();
                dataSource.runQuery();
                if (text == "") {
                    //_app.removeFromMaps("thumbs");
                }
            }
        }
    }
    attachedObjects: [
        ComponentDefinition {
            id: artistPageDef
            source: "ArtistPage.qml"
        },
        ComponentDefinition {
            id: artistAlbumPageDef
            source: "ArtistAlbumPage.qml"
        },
        GoDataSource {
            id: generalData
            connectionName: "general"
        },
        ComponentDefinition {
            id: playPageDef
            source: "Play.qml"
        },
        ComponentDefinition {
            id: playPageSmallDef
            source: "PlaySmall.qml"
        },
        MediaPlayer {
            id: player
            objectName: "player"
        },
        NowPlayingConnection {
            id: nowPlaying
            objectName: "nowPlaying"

            duration: player.duration
            position: player.position
            mediaState: player.mediaState

            onAcquired: {
                //nowPlaying.setMetaData(metadata);
                nowPlaying.setMetaData(_app.currSong.metaData);
                //nowPlaying.setIconUrl(_app.currSong.albumArtThumb);
                nowPlaying.next.connect(_app.playNextSong);
                nowPlaying.previous.connect(_app.playPreviousSong);
            }

            onPause: {
                player.pause();
            }

            onPlay: {
                player.play();
            }

            onRevoked: {
                player.stop();
            }
        },
        ComponentDefinition {
            id: playlistTab
            source: "PlaylistTab.qml"
        },
        Sheet {
            id: settingsSheet
            SettingsSheet {
            }
        },
        Sheet {
            id: loginView
            LoginSheet {
                id: loginViewPage
            }
        },
        SystemDialog {
            id: alertDialog
            objectName: "alertDialog"
            title: qsTr("Download Failed")
            body: "Error code: 1"
            buttons: [
                SystemUiButton {
                    label: qsTr("Send Log To Dev")
                }
            ]
            confirmButton.label: undefined
            cancelButton.label: qsTr("Ok")
            cancelButton.enabled: true
            dismissAutomatically: true
            onFinished: {
                if (result != SystemUiResult.CancelButtonSelection) {
                    _app.sendLogFile();   
                }
            }
        },
        SystemDialog {
            id: locationDialog
            title: qsTr("Save Location")
            body: qsTr("Note: All Access songs can only be saved to the device.")
            buttons: [
                SystemUiButton {
                    label: qsTr("SD Card")
                },
                SystemUiButton {
                    label: qsTr("Device")
                }
            ]
            confirmButton.label: undefined
            cancelButton.label: qsTr("Cancel")
            cancelButton.enabled: true
            dismissAutomatically: true
            onFinished: {
                if (result == SystemUiResult.CancelButtonSelection) {
                    // it will auto dismiss
                } else {
                    var button = locationDialog.buttonSelection();
                    if (button.label == qsTr("SD Card")) {
                        _app.downloadPlaylist("Thumbs Up", "sdcard");
                    } else {
                        _app.downloadPlaylist("Thumbs Up", "device");
                    }
                }
            }
        }

    ]
    onCreationCompleted: {
        OrientationSupport.supportedDisplayOrientation = SupportedDisplayOrientation.DisplayPortrait;
        if (! _app.isLoggedIn()) {
            loginView.open();
        }
        _app.loginComplete.connect(loginView.close);
        _app.loginComplete.connect(refreshModels);
        _app.loginComplete.connect(onPlaylistChange);
        _app.playlistsUpdated.connect(onPlaylistChange);
        _app.playlistsUpdated.connect(refreshModels);
        _app.error.connect(onErrorMessage);
        _app.thumbsUpdated.connect(refreshThumbs);
        _app.refreshLogin.connect(loginView.open);
    }
    
    function openLoginViewForRefresh() {
        loginView.open();
        loginViewPage.shouldLoadLibrary = false;
    }
    
    function refreshModels() {
        songs.songsPane.dataSource.runQuery();
        artists.artistPane.dataSource.runQuery();
        albums.albumsPane.dataSource.runQuery();
        thumbsUp.thumbsPane.dataSource.runQuery();
    }
    
    function refreshThumbs() {
        thumbsUp.thumbsPane.dataSource.runQuery();
    }

    function onPlaylistChange() {
        var playlistSize = _app.playlists.length;
        var tabsSize = tabPane.tabs.length;

        while (tabPane.tabs.length != 5) {
            var tab = tabPane.at(5);
            tabPane.remove(tab);
        }

        for (var playlist in _app.playlists) {
            var newTab = playlistTab.createObject();
            newTab.title = _app.playlists[playlist].title;
            newTab.playlistId = _app.playlists[playlist].playlistId;
            tabPane.add(newTab);
        }
    }

    function onErrorMessage(title, message) {
        alertDialog.title = title;
        alertDialog.body = message;
        alertDialog.show();
    }
}
