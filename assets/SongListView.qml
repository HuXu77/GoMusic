import bb.cascades 1.0
import "listitems"

ListView {
        dataModel: _app.model
        listItemComponents: [
                ListItemComponent {
                        type: "item"
                        SongListItem {}
                    }
            ]
        onTriggered: {
                var chosenItem = _app.model.data(indexPath);
                playPage = playPageDefinition.createObject();
                playPage.song = chosenItem.title;
                playPage.artist = chosenItem.artist;
                playPage.album = chosenItem.album;
                navigationPane.push(playPage);
                _app.selectedSong(chosenItem.id);
                navigationPane.backButtonsVisible = true;
            }
    }
