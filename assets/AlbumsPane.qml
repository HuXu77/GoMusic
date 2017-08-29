import bb.cascades 1.0
import "listitems"

NavigationPane {
    id: navigationPane
    backButtonsVisible: false
    
    property variant albumPage: null
    property variant playPage: null
    
    Page {
        Container {
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
            ListView {
                id: albumListView
                dataModel: _app.albums
                listItemComponents: [
                    ListItemComponent {
                        type: "item"
                        AlbumListItem {}
                    }
                ]
                onTriggered: {
                    var chosenAlbum = _app.albums.data(indexPath);
                    albumPage = albumPageDef.createObject();
                    albumPage.artistName = chosenAlbum.album;
                    _app.getPlaylistByAlbum(chosenAlbum);
                    
                    navigationPane.push(albumPage);
                    navigationPane.backButtonsVisible = true;
                    //searchField.text = "";
                }
            }
        }
        }
        actions: [
            ActionItem {
                title: qsTr("Refresh Library")
                imageSource: "asset:///images/refresh.png"
                onTriggered: {
                    _app.refreshData();
                }
            },
            ActionItem {
                id: goToNowPlaying
                title: qsTr("Now Playing")
                imageSource: "asset:///images/nowPlay.png"
                enabled: _app.currSong.isSongDownloading() || _app.currSong.isAvailable()
                onTriggered: {
                    if (! _app.largeButtonsDisabled) {
                        playPage = playPageSmallDef.createObject();
                    } else {
                        playPage = playPageDef.createObject();
                    }
                    navigationPane.push(playPage);
                    navigationPane.backButtonsVisible = true;
                    searchField.text = "";
                }
            }
        ]
    }
    
    onPopTransitionEnded: {
        //albumPage.destroy();
        playPage.disconnectSignals();
        //page.destroy();
        _app.setSelectedAlbum("");
        searchField.text = "";
        if (page == albumPage) {
            navigationPane.backButtonsVisible = true;
        }
    }
    
    attachedObjects: [
        ComponentDefinition {
            id: albumPageDef
            source: "ArtistPage.qml"
        },
        ComponentDefinition {
            id: playPageDef
            source: "Play.qml"
        },
        ComponentDefinition {
            id: playPageSmallDef
            source: "PlaySmall.qml"
        }
    ]
}
