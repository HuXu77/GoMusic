import bb.cascades 1.0

Page {
    
    property bool shouldLoadLibrary: true
    
    Container {
        layout: DockLayout {}
        Container {
            layout: AbsoluteLayout {
                
            }
            ImageView {
                imageSource: "asset:///images/loginscreen.png"
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Top
                scalingMethod: ScalingMethod.AspectFit
            }
            
        }
        Container {
            id: loadingArea
            horizontalAlignment: HorizontalAlignment.Fill
            minHeight: 60
            visible: false
            background: Color.Black
            opacity: .5
        }
        Container {
            layout: StackLayout {
                orientation: LayoutOrientation.LeftToRight
            }
            horizontalAlignment: HorizontalAlignment.Fill
            ActivityIndicator {
                id: actIndicator
                running: true
                leftPadding: 50
                topPadding: 5
                minHeight: 60
                minWidth: 70
                visible: false
            }
            Label {
                id: loadingText
                text: qsTr("Loading library...")
                verticalAlignment: VerticalAlignment.Center
                textStyle {
                    base: SystemDefaults.TextStyles.BodyText
                    color: Color.White
                }
                visible: false
            }
            Container {
                id: downloadPArea
                layout: StackLayout {
                    orientation: LayoutOrientation.LeftToRight
                }
                topPadding: 10
                leftPadding: 10
                visible: false
            	Label {
                	id: typeLabel
                	text: qsTr("Library")
                	verticalAlignment: VerticalAlignment.Center
                	textStyle {
                    	base: SystemDefaults.TextStyles.SmallText
                    	color: Color.White
                    }
                    minWidth: 90
            	}
            	DownloadProgress {
                	id: downloadProgress
                	visible: false
                	leftPadding: 20
                 	horizontalAlignment: HorizontalAlignment.Center
                	verticalAlignment: VerticalAlignment.Center
                }
            }
        }
        Container {
            horizontalAlignment: HorizontalAlignment.Center
            verticalAlignment: VerticalAlignment.Top
            leftPadding: 50
            rightPadding: 50
            topPadding: 320
            TextField {
                id: email
                hintText: qsTr("Email")
                //text: "theclays0809@gmail.com"
                //text: "mitchellrclay@gmail.com"
                inputMode: TextFieldInputMode.EmailAddress
                onCreationCompleted: {
                    email.requestFocus();
                }
            }
            TextField {
                id: password
                //text: "levimitchell!13"
                //text: "@mb3r!shot"
                hintText: qsTr("Password")
                inputMode: TextFieldInputMode.Password
            }
            Button {
                id: loginButton
                horizontalAlignment: HorizontalAlignment.Center
                text: qsTr("Login")
                onClicked: {
                    _app.login(email.text, password.text, shouldLoadLibrary);
                    // TODO: change this text to be more meaningful
                    loginButton.enabled = false;
                    loginButton.text = qsTr("Logging in...");
                }
            }
            /*TextArea {
                id: jsonDump
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Bottom
            }*/
        }
    }
    onCreationCompleted: {
        _app.updatePlaylistDownload.connect(updateProgress);
        _app.loginRetry.connect(retry);
        _app.buildingPlaylists.connect(buildingPL);
        _app.showActIndicator.connect(showAct);
        //_app.jsonDump.connect(setJsonDump);
    }
    
    /*function setJsonDump(response) {
        jsonDump.text = response;
    }*/
    
    function showAct() {
        loadingText.visible = true;
        actIndicator.visible = true;
        loadingArea.visible = true;
    }
    
    function updateProgress(current, total) {
        if (total != 0) {
            downloadProgress.visible = true;
            actIndicator.visible = false;
            loadingText.visible = false;
            downloadPArea.visible = true;
        }
        downloadProgress.toValue = total;
        downloadProgress.value = current;
        downloadProgress.label = current + "/" + total;
    }
    
    function buildingPL() {
        typeLabel.text = qsTr("Playlists");
    }
    
    function retry() {
        loginButton.text = qsTr("Login")
        loginButton.enabled = true;
        downloadProgress.visible = false;
        loadingText.visible = false;
        loadingText.text = qsTr("Loading library...");
        password.text = "";
    }
}
