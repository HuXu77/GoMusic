import bb.cascades 1.0

NavigationPane {
    property variant loading: loading
    
    id: navigationPane
    
    Page {
        titleBar: TitleBar {
            id: titleBar
            title: qsTr("Radio")
            kind: TitleBarKind.FreeForm
            kindProperties: FreeFormTitleBarKindProperties {
                Container {
                    id: con
                    layout: StackLayout {
                        orientation: LayoutOrientation.LeftToRight
                    }
                    leftPadding: 10
                    rightPadding: 10
                    Label {
                        text: qsTr("Radio")
                        verticalAlignment: VerticalAlignment.Center
                        layoutProperties: StackLayoutProperties {
                            spaceQuota: 1
                        }
                    }
                    ActivityIndicator {
                        id: loading
                        verticalAlignment: VerticalAlignment.Center
                        running: true
                        visible: true
                    }
                }
            }
        }
        Container {
            layout: DockLayout {
            }
            ListView {
                id: blah
                dataModel: _app.radioStations
                listItemComponents: [
                    ListItemComponent {
                        type: "item"
                        content: StandardListItem {
                            title: ListItemData.name
                        }
                    }
                ]
                onTriggered: {
                    var chosenItem = blah.dataModel.data(indexPath);
                    _app.loadRadioStation(chosenItem.id, chosenItem.seed);
                    var playPage = null;
                    if (!_app.largeButtonsDisabled) {
                        playPage = playPageSmallDef.createObject();
                    } else {
                        playPage = playPageDef.createObject();
                    }

                    navigationPane.push(playPage);
                }
            }
        }
    }
    
    onCreationCompleted: {
        _app.radioStationsLoaded.connect(toggleActivity)
    }
    
    function toggleActivity() {
        loading.visible = false;
    }
    
    attachedObjects: [
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
