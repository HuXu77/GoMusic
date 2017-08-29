import bb.cascades 1.0
import bb.system 1.0

Tab {
    property string playlistId: ""
    imageSource: "asset:///images/userPlaylist.png"
    
    property alias dataSource: playlistPane.dataSource
    property bool queryRun: false
   
    BasicPane {
        id: playlistPane
        connectionName: playlistId
        sorting: [""]
        group: ItemGrouping.None
        query: _app.showLocalOnly ? "select * from song a, playlistSongs b where (a.id = b.sid or a.matchedId = b.sid) and b.pid = '" + playlistId + "' and a.downloadCompleted = 1 order by b.sortOrder desc" : 
        "select * from song a, playlistSongs b where (a.id = b.sid or a.matchedId = b.sid) and b.pid = '" + playlistId + "' order by b.sortOrder desc"
        
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
            var chosenItem = listView.dataModel.data(indexPath);
            //goToNowPlaying.enabled = true;
            _app.selectedSong(indexPath, playlistId, listView.dataModel);
            var playP = null;
            if (! _app.largeButtonsDisabled) {
                // logic is a little backwards here
                playP = playPageSmallDef.createObject();
            } else {
                playP = playPageDef.createObject();
            }

            setPlayPage(playP);

            navigationPane.backButtonsVisible = true;
            //searchField.text = "";
        }
        onCreationCompleted: {
            playlistPane.filterList.connect(filterPlaylist);
        }

        onShuffleThis: {
            _app.shuffle(playlistId, listView.dataModel);
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

        function filterPlaylist(text) {
            var fixed = text.replace("\'", "\'\'");
            var filterQuery = "select * from song a, playlistSongs b where a.id = b.sid and b.pid = '" + playlistId + "' and a.title like '%" + fixed + "%' order by b.sortOrder desc";
            if (_app.showLocalOnly) {
                filterQuery = "select * from song a, playlistSongs b where a.id = b.sid and b.pid = '" + playlistId + "' and a.title like '%" + fixed + "%' and a.downloadCompleted = 1 order by b.sortOrder desc";
            }
            playlistPane.query = filterQuery;
            _app.setOriginalModel(playlistId, listView.dataModel);
            listView.dataModel.clear();
            dataSource.runQuery();
        }
        
        attachedObjects: [
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
                            _app.downloadPlaylist(playlistId, "sdcard");
                        } else {
                            _app.downloadPlaylist(playlistId, "device");
                        }
                    }
                }
            }
        ]
    }
    
    function runQuery() {
        if (!queryRun) {
            // set activity indicator visible
            playlistPane.listLoading.visible = true;
            playlistPane.listLoading.running = true;
            playlistPane.dataSource.runQuery();
            queryRun = true;
        }
    }
}
