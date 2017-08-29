import bb.cascades 1.0

Page {
    titleBar: TitleBar {
              title: qsTr("Settings")
              acceptAction: ActionItem {
                                title: qsTr("Close")
                                onTriggered: {
                                    settingsSheet.close();
                                }
                            }
              }
    Container {
        layout: StackLayout {
            orientation: LayoutOrientation.TopToBottom
        }
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Fill
        background: Color.Black
        Container {
            topPadding: 50
            leftPadding: 70
            bottomPadding: 50
            
            layout: StackLayout {
                        orientation: LayoutOrientation.LeftToRight
                    }
            ToggleButton {
                checked: _app.screenLockDisabled
                onCheckedChanged: {
                    _app.screenLockDisabled = checked;
                }
            }
            Label {
                text: qsTr("Disable screen lock")
                textStyle {
                    base: SystemDefaults.TextStyles.BodyText
                    color: Color.White
                }
            }
        }
        Container {
            bottomPadding: 50
            leftPadding: 70
            Container {
            	layout: StackLayout {
                	orientation: LayoutOrientation.LeftToRight
                }
            	ToggleButton {
            		checked: _app.largeButtonsDisabled
            		onCheckedChanged: {
                 	_app.largeButtonsDisabled = checked;
                 }
             	}
            	Label {
                	text: qsTr("Large Buttons*")
                	textStyle {
                    	base: SystemDefaults.TextStyles.BodyText
                    	color: Color.White
                    }
                }
            }
        	Container {
            	Label {
            		text: qsTr("*Requires App Restart")
            	
            		textStyle {
                		base: SystemDefaults.TextStyles.SmallText
                		color: Color.White
                	}
                }
            }
        }
        Container {
            leftPadding: 70
            bottomPadding: 50
            Container {
                layout: StackLayout {
                    orientation: LayoutOrientation.LeftToRight
                }
                ToggleButton {
                    checked: _app.showLocalOnly
                    onCheckedChanged: {
                        _app.showLocalOnly = checked;
                    }
                }
                Container {
                    Label {
                        text: qsTr("Show Only Local Songs")
                        textStyle {
                            base: SystemDefaults.TextStyles.BodyText
                            color: Color.White
                        }
                    }
                }
               
            }
            Container {
                Label {
                    text: qsTr("*MAY require App restart")
                    
                    textStyle {
                        base: SystemDefaults.TextStyles.SmallText
                        color: Color.White
                    }
                }
            }
            
        }
        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            layout: StackLayout {
                orientation: LayoutOrientation.TopToBottom
            }
        	Button {
            	text: qsTr("Logout")
            	horizontalAlignment: HorizontalAlignment.Center
            	onClicked: {
                	_app.logout();
                	settingsSheet.close();
                	loginView.open();
                	for (var plTab in tabPane.tabs) {
                    	if (plTab.playlistId != undefined && plTab.playlistId != "Thumbs Up") {
                        	tabPane.remove(plTab);
                        }
                    }
                }
            }
        	Button {
        	    horizontalAlignment: HorizontalAlignment.Center
            	text: qsTr("Delete Offline All Access Songs")
            	onClicked: {
                	_app.deleteAllAccessCache();
                }
            }
        }
        /*Label {
            text: _app.amountOfStorageAAUsing
            textStyle {
                base: SystemDefaults.TextStyles.BodyText
                color: Color.White
            }
        }*/
    }
}
