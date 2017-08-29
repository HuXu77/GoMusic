import bb.cascades 1.0
import "listitems"

NavigationPane {
    id: navigationPane
    objectName: "navigationPane"
            
    property alias dataModel: songListView.dataModel
    property alias actions: songPage.actions
    backButtonsVisible: false
    
    property variant playPage: null
    property variant songPage: songPage
            
    Page {
        id: songPage
        Container {
            
            Container {
                id: playlistDownloadArea
                horizontalAlignment: HorizontalAlignment.Fill
                background: Color.Gray
                Container {
                    id: dlPlCon
                    leftPadding: 5
                    visible: false
                    layout: StackLayout {
                                orientation: LayoutOrientation.LeftToRight
                            }
                    horizontalAlignment: HorizontalAlignment.Fill
                    Label {
                        text: qsTr("Playlist")
                        textStyle {
                            base: SystemDefaults.TextStyles.SmallText
                            color: Color.White
                        }
                    }
                    DownloadProgress {
                        id: playlistDownloadProgress
                        horizontalAlignment: HorizontalAlignment.Center
                    }
                }
                Container {
                    id: dlSCon
                    leftPadding: 5
                    visible: false
                    horizontalAlignment: HorizontalAlignment.Fill
                    layout: StackLayout {
                                orientation: LayoutOrientation.LeftToRight
                            }
                    Label {
                        text: qsTr("Song     ")
                        textStyle {
                            base: SystemDefaults.TextStyles.SmallText
                            color: Color.White
                        }
                    }
                    DownloadProgress {
                        id: songDownloadProgress
                        horizontalAlignment: HorizontalAlignment.Center
                    }
                }
            }
            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                bottomPadding: 5
                topPadding: 5
                leftPadding: 5
                rightPadding: 5
                background: Color.Gray
                TextField {
                    id: searchField
                    hintText: qsTr("Search")
                    onTextChanging: {
                        _app.filterPossible(text);
                    }
                }
            }
            Container {
                layout: DockLayout {}
                ListView {
                    id: songListView
                    dataModel: _app.possiblePlaylist
                    listItemComponents: [
                        ListItemComponent {
                                    type: "item"
                                    SongListItem {}
                                }
                    ]
                    onTriggered: {
                        var chosenItem = songListView.dataModel.data(indexPath);
                        goToNowPlaying.enabled = true;
                        _app.selectedSong(indexPath, songListView.dataModel);
                        playPage = playPageDefinition.createObject();
                                    
                        navigationPane.push(playPage);
                        navigationPane.backButtonsVisible = true;
                        searchField.text = "";
                    }
                }
            }
        }
        actions: [
            ActionItem {
                title: qsTr("Refresh Library")
                imageSource: "asset:///images/refresh.png"
                enabled: !dlPlCon.visible
                onTriggered: {
                    _app.refreshData();
                }
            },
            ActionItem {
                id: goToNowPlaying
                title: qsTr("Go to Now Playing")
                imageSource: "asset:///images/nowPlay.png"
                enabled: _app.currSong.isSongDownloading() || _app.currSong.isAvailable()
                onTriggered: {
                    playPage = playPageDefinition.createObject();
                    navigationPane.push(playPage);
                    navigationPane.backButtonsVisible = true;
                    searchField.text = "";
                }
            }
        ]
    }
    onPopTransitionEnded: {
        playPage.disconnectSignals();
        playPage.destroy();
        navigationPane.backButtonsVisible = false;
    }
    onCreationCompleted: {
        _app.downloadPlaylistApi.downloadProgress.connect(updatePlaylistSongDownload);
        _app.downloadPlaylistApi.updatePlaylistCount.connect(updatePlaylistDownload);
        _app.updatePlaylistDownload.connect(updatePlaylistDownload);
    }
    attachedObjects: [
        ComponentDefinition {
            id: playPageDefinition
            source: "Play.qml"
        }
    ]
    
    function updatePlaylistSongDownload(curr, total) {
        songDownloadProgress.toValue = 100;
        var currPerc = (curr * 100) / total;
        songDownloadProgress.value = currPerc;
        if (currPerc > 0) {
            dlSCon.visible = true;
        }
        songDownloadProgress.label = Math.round(currPerc) + "%";
        
    }
    
    function updatePlaylistDownload(curr,total) {
        if (curr != total) {
            dlPlCon.visible = true;
        } else {
            dlPlCon.visible = false;
            dlSCon.visible = false;
        }
        playlistDownloadProgress.toValue = total;
        playlistDownloadProgress.value = curr;
        playlistDownloadProgress.label = curr + "/" + total;    
    }

    
}