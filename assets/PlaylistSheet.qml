import bb.cascades 1.0
import mc.cascades 1.0

Page {
    Container {
        ListView {
            dataModel: listModel
            listItemComponents: [
                ListItemComponent {
                    type: "item"
                    content: StandardListItem {
                        title: ListItemData.title
                    }
                }
            ]
            onTriggered: {
                var chosenItem = dataModel.data(indexPath);
                _app.addCurrSongToPlaylist(chosenItem.playlistId);
            }
        }
    }
    attachedObjects: [
        BasicModel {
            id: listModel
        },
        GoDataSource {
            id: dataSource
            query: "select * from playlist"
            
            onDataLoaded: {
                listModel.clear();
                if (data.length > 0) {
                    listModel.insertList(data);
                }
            }
        }
    ]
}
