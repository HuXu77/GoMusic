import bb.cascades 1.0
import mc.cascades 1.0
import bb.system 1.0
import "listitems"

Page {
    id: aPage
    
    property alias artistName: titleBar.title
    property alias query: dataSource.query
    property alias dataSource: dataSource
    property alias listModel: listModel
    property alias connectionName: dataSource.connectionName
    
    property bool showAlbum: true

    property string mapId: "artist"

    signal pushThis(variant apage)
    signal playPage(variant page)

    titleBar: TitleBar {
                  id: titleBar
                  title: "Artist"
              }
    Container {
        Container {
            id: playlistDownloadArea
            horizontalAlignment: HorizontalAlignment.Fill
            background: Color.Gray
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
        ListView {
            property alias mySelf: aPage
            
            id: listView
            dataModel: listModel
            listItemComponents: [
                    ListItemComponent {
                        	id: lic
                        	property alias silly: listView.mySelf
                            type: "item"
                            SongListItem {
                                //albumArtist: lic.silly.showAlbum ? "Hi" : "Boo"
                            }
                    }
            ]
            
            onTriggered: {
                var chosenItem = dataModel.data(indexPath);
                //goToNowPlaying.enabled = true;
                var playPage = null;
                if (!_app.largeButtonsDisabled) {
                    playPage = playPageSmallDef.createObject();
                } else {
                    playPage = playPageDef.createObject();
                }
                aPage.playPage(playPage);
                _app.selectedSong(indexPath, mapId, listView.dataModel);
                //typeOfPage = "playPage";
            }
        }
    }
    actions: [
        ActionItem {
            id: goToNowPlaying
            title: qsTr("Now Playing")
            imageSource: "asset:///images/nowPlay.png"
            enabled: _app.currSong.isSongDownloading() || _app.currSong.isAvailable()
            onTriggered: {
                var playPage = null;
                if (! _app.largeButtonsDisabled) {
                    playPage = playPageSmallDef.createObject();
                } else {
                    playPage = playPageDef.createObject();
                }
                aPage.playPage(playPage);
            }
        },
        ActionItem {
            title: qsTr("Download Album")
            imageSource: "asset:///images/download.png"
            enabled: ! dlPlCon.visible
            onTriggered: {
                locationDialog.show();
            }
        }
    ]
    
    onQueryChanged: {
        dataSource.runQuery();
    }
    
    attachedObjects: [
        ComponentDefinition {
            id: playPageDef
            source: "Play.qml"
        },
        ComponentDefinition {
            id: playPageSmallDef
            source: "PlaySmall.qml"
        },
        BasicModel {
            id: listModel
        },
        GoDataSource {
            id: dataSource
            connectionName: "artistSongs"

            onDataLoaded: {
                if (data.length > 0) {
                    listModel.insertList(data);
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
            cancelButton.label: qsTr("Cancel")
            cancelButton.enabled: true
            confirmButton.label: undefined
            dismissAutomatically: true
            onFinished: {
                if (result == SystemUiResult.CancelButtonSelection) {
                    // it will auto dismiss
                } else {
                    var button = locationDialog.buttonSelection();
                    if (button.label == qsTr("SD Card")) {
                        _app.downloadAlbum(query, "sdcard");
                    } else {
                        _app.downloadAlbum(query, "device");
                    }
                }
            }
        }
    ]
    
    onCreationCompleted: {
        _app.downloadPlaylistApi.downloadProgress.connect(updatePlaylistSongDownload);
    }
    
    function updatePlaylistSongDownload(curr, total) {
        songDownloadProgress.toValue = 100;
        var currPerc = (curr * 100) / total;
        songDownloadProgress.value = currPerc;
        if (currPerc > 0) {
            dlSCon.visible = true;
        }
        if (currPerc == 100) {
            dlSCon.visible = false;
        }
        songDownloadProgress.label = Math.round(currPerc) + "%";
    
    }
}
