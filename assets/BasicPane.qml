import bb.cascades 1.0
import mc.cascades 1.0
import "listitems"

NavigationPane {
    id: navigationPane
    
    property alias query: dataSource.query
    property alias sorting: listModel.sortingKeys
    property alias connectionName: dataSource.connectionName
    property alias dataSource: dataSource
    property alias listView: listView
    property alias listLoading: listLoading
    property alias group: listModel.grouping
    property alias navigationPane: navigationPane
    
    property alias dlPlCon: dlPlCon
    
    property string mapId: "library"
    
    property alias actions: basicPage.actions

    property variant playPage: null
    
    signal filterList(string text);
    signal shuffleThis();

    Page {
        id: basicPage
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
                        text: qsTr("Library")
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
                rightPadding: 15
                background: Color.Gray
                Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    layout: StackLayout {
                        orientation: LayoutOrientation.LeftToRight
                    }
                	TextField {
                    	id: searchField
                    	verticalAlignment: VerticalAlignment.Center
                    	hintText: qsTr("Search")
                    	onTextChanging: {
                        	filterList(text);
                        }
                    }
                	ImageButton {
                     	defaultImageSource: "asset:///images/shuffleOff.png"
                     	horizontalAlignment: HorizontalAlignment.Right
                     	onClicked: {
                     	    shuffleThis();
                     	}
                     }
                }
                ActivityIndicator {
                    id: listLoading
                    running: true
                    visible: true
                }
            }
            Container {
                layout: DockLayout {
                }
                ListView {
                    id: listView
                    dataModel: listModel
                    listItemComponents: [
                        ListItemComponent {
                            type: "item"
                            content: SongListItem {
                            }
                        }
                    ]
                    onTriggered: {
                        
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
                    if (!_app.largeButtonsDisabled) {
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
    
    onPushTransitionEnded: {
        page.pushThis.connect(pushPage);
        page.playPage.connect(setPlayPage);
    }
    
    onPopTransitionEnded: {
        if (page != playPage) {
            page.destroy();
        }
        
    }
    attachedObjects: [
        BasicModel {
            id: listModel
        },
        /*SqlDataQuery {
            id: defaultQuery
            source: "sql/gmusic.db"
            query: "select * from song where deleted = 0 and source = 1"
            countQuery: "select count(*) from song where deleted = 0 and source = 1"
            
            keyColumn: "id"
        },*/
        GoDataSource {
            id: dataSource
            query: _app.showLocalOnly ? "select * from song where deleted = 0 and source = 1 and downloadCompleted = 1" : "select * from song where deleted = 0 and source = 1"
            
            onDataLoaded: {
            	listLoading.running = false;
            	listLoading.visible = false;
            	listModel.clear();
                if (data.length > 0) {
                    listModel.insertList(data);
                }
            }
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
    
    function setPlayPage(page) {
        if (playPage == null) {
            playPage = page;
        }
        pushPage(playPage);
    }
    
    function pushPage(page) {
        navigationPane.push(page);
    }

    function updatePlaylistSongDownload(curr, total) {
        songDownloadProgress.toValue = 100;
        var currPerc = (curr * 100) / total;
        songDownloadProgress.value = currPerc;
        if (currPerc > 0) {
            dlSCon.visible = true;
        }
        songDownloadProgress.label = Math.round(currPerc) + "%";

    }

    function updatePlaylistDownload(curr, total) {
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

    onCreationCompleted: {
        _app.downloadPlaylistApi.downloadProgress.connect(updatePlaylistSongDownload);
        _app.downloadPlaylistApi.updatePlaylistCount.connect(updatePlaylistDownload);
        _app.updatePlaylistDownload.connect(updatePlaylistDownload);
    }
}
