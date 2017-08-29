import bb.cascades 1.0
import mc.cascades 1.0
import "listitems"

Page {
    id: aaPage
    
    property alias artistName: titleBar.title
    property alias dataSource: dataSource
    property alias listModel: listModel
    
    signal pushThis(variant apage)
    signal playPage(variant ppage)
    
    titleBar: TitleBar {
        id: titleBar
        title: "Artist"
    }
    Container {
        ListView {
            id: albumsForArtist
            dataModel: listModel
            listItemComponents: [
                ListItemComponent {
                    type: "item"
                    AlbumListItem {}
                }
            ]
            onTriggered: {
                var chosenAlbum = albumsForArtist.dataModel.data(indexPath);
                if (chosenAlbum.album == "All Songs") {
                    var albumPage = albumPageDef.createObject();
                    albumPage.artistName = titleBar.title;
                    albumPage.connectionName = titleBar.title;
                    albumPage.showAlbum = true;

					var artty = titleBar.title.replace("\'", "\'\'");
					if (_app.showLocalOnly) {
                        albumPage.query = "select * from song where artist = '"+artty+"' and downloadCompleted = 1";
					} else {
                        albumPage.query = "select * from song where artist = '"+artty+"'";
					}
                    
                    
                    albumPage.playPage.connect(setPlayPage);
                    
                    aaPage.pushThis(albumPage);
                } else {
                	var albumPage = albumPageDef.createObject();
                	albumPage.artistName = chosenAlbum.album;
                	albumPage.connectionName = titleBar.title + chosenAlbum.album;
                    albumPage.showAlbum = false;
                    var artty = titleBar.title.replace("\'", "\'\'");
                    var albuy = chosenAlbum.album.replace("\'", "\'\'");
                    if (_app.showLocalOnly) {
                        albumPage.query = "select * from song where artist = '"+artty+"' and album = '"+ albuy +"' and downloadCompleted = 1 order by track asc";
                    } else {
                        albumPage.query = "select * from song where artist = '"+artty+"' and album = '"+ albuy +"' order by track asc";
                    }
                	albumPage.listModel.grouping = ItemGrouping.None
                	albumPage.listModel.sortingKeys = ["track"];

                    albumPage.playPage.connect(setPlayPage);
                    
                    aaPage.pushThis(albumPage);
                }
            }
        }
    }

    attachedObjects: [
        ComponentDefinition {
            id: albumPageDef
            source: "ArtistPage.qml"
        },
        BasicModel {
            id: listModel
            sortingKeys: [
                "sortOrder"
            ]
        },
        GoDataSource {
            id: dataSource
            connectionName: "artistAlbums"

            onDataLoaded: {
                if (data.length > 0) {
                    listModel.insertList(data);
                }
            }
        }
    ]
    
    function setPlayPage(page) {
        aaPage.playPage(page);
    }
}
