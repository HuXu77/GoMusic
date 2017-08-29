import bb.cascades 1.0
import bb.multimedia 1.0
import bb.system 1.0

Page {
    
    Container {
        layout: DockLayout {
                }
        verticalAlignment: VerticalAlignment.Fill
        horizontalAlignment: HorizontalAlignment.Fill
        ImageView {
                id: albumArt
                horizontalAlignment: HorizontalAlignment.Fill
                scalingMethod: ScalingMethod.AspectFill
                image: _app.currSong.albumArt
                loadEffect: ImageViewLoadEffect.FadeZoom
                //imageSource: "asset:///images/default_album.png"
                minWidth: 1440
                preferredHeight: 980
        }
        Container {
            layout: StackLayout {
                orientation: LayoutOrientation.TopToBottom
            }
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Bottom
            ImageView {
                horizontalAlignment: HorizontalAlignment.Fill
                imageSource: "asset:///images/gradient.png"
                translationY: 10
            }
            Container {
                background: Color.Black
                horizontalAlignment: HorizontalAlignment.Fill
                minHeight: 400
            }
        }
        
        Container {
        verticalAlignment: VerticalAlignment.Fill
        horizontalAlignment: HorizontalAlignment.Fill
       // This holds the player and controls at the bottom of the screen
        Container {
            id: topSection
            // This section holds song info and playlist/song controls
            // e.g. shuffle, repeat, thumbs up or down
            topPadding: 660
            layout: DockLayout {
                    }
            verticalAlignment: VerticalAlignment.Bottom
            horizontalAlignment: HorizontalAlignment.Fill
                Container {
                    topPadding: 15
                    verticalAlignment: VerticalAlignment.Top
                    ImageView {
                        imageSource: "asset:///images/localIndicator.png"
                        visible: _app.currSong.isLocal
                    }
                }
                Container {
                // This container will hold the song info
                leftPadding: 100
                verticalAlignment: VerticalAlignment.Bottom
                horizontalAlignment: HorizontalAlignment.Fill
                
                Label {
                    id: songTitle
                    //text: "Song Title"
                    text: _app.currSong.song.title
                    textStyle {
                        base: SystemDefaults.TextStyles.BigText
                        color: Color.White
                        }
                    }
                Label {
                    id: artistName
                    //text: "Artist"
                    text: _app.currSong.song.artist
                    textStyle {
                        color: Color.White
                    }
                }
                Container {
                    layout: DockLayout {
                    }
                    horizontalAlignment: HorizontalAlignment.Fill
                    rightPadding: 50
                	Label {
                    	id: albumName
                    	//text: "Album"
                    	text: _app.currSong.song.album
                    	textStyle {
                        	color: Color.White
                        }
                    }
                        ImageView {
                            id: ratingImage
                            imageSource: _app.currSong.song.rating == 5 ? "asset:///images/playThumbsUp.png" :
                                          _app.currSong.song.rating == 1 ? "asset:///images/playThumbsDown.png" :
                                            ""
                            scalingMethod: ScalingMethod.AspectFit
                            minHeight: 60
                            //imageSource: "asset:///images/playThumbsUp.png"
                            horizontalAlignment: HorizontalAlignment.Right
                            verticalAlignment: VerticalAlignment.Bottom
                        }

                    }
                
                Container {
                    layout: DockLayout {
                        
                    }
                    topPadding: 20
                    horizontalAlignment: HorizontalAlignment.Fill
                    //background: Color.Black
                    ActivityIndicator {
                        id: gettingSongUrl
                        visible: true
                        running: true
                    }
                    Container {
                        leftPadding: 60
                	   Label {
                	    	id: downloadProgress
                	        text: "100%"
                	        visible: false
                	    }
                	}

                    }
                    
                }
        }
        
        Container {
            id: bottomSection
                layout: DockLayout {
                        }
                rightPadding: 50
                leftPadding: 50
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                minHeight: 200
                preferredHeight: 290
                //background: Color.Black
                
                Container {
                    layout: DockLayout {
                    }
                   
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                Container {
                    verticalAlignment: VerticalAlignment.Bottom
                    horizontalAlignment: HorizontalAlignment.Left
                    leftPadding: 200
                    layout: DockLayout {
                    }

                    ImageButton {
                        id: repeatButton
                        horizontalAlignment: HorizontalAlignment.Right
                        defaultImageSource: "asset:///images/repeatOff.png"
                        onClicked: {
                            _app.toggleRepeat();
                            if (! _app.repeatAll && ! _app.repeatOne) {
                                repeatButton.setDefaultImageSource("asset:///images/repeatOff.png");
                            } else if (_app.repeatAll && ! _app.isRadioSong) {
                                repeatButton.setDefaultImageSource("asset:///images/repeatOn.png");
                            } else {
                                repeatButton.setDefaultImageSource("asset:///images/repeatOne.png");
                            }
                        }
                        onCreationCompleted: {
                            if (! _app.repeatAll && ! _app.repeatOne) {
                                repeatButton.setDefaultImageSource("asset:///images/repeatOff.png");
                            } else if (_app.repeatAll && !_app.isRadioSong) {
                                repeatButton.setDefaultImageSource("asset:///images/repeatOn.png");
                            } else {
                                repeatButton.setDefaultImageSource("asset:///images/repeatOne.png");
                            }
                        }
                    }
                }
                Container {
                    verticalAlignment: VerticalAlignment.Bottom
                    horizontalAlignment: HorizontalAlignment.Right
                    rightPadding: 200
                    ImageToggleButton {
                        visible: !_app.isRadioSong
                        horizontalAlignment: HorizontalAlignment.Right
                        imageSourceDefault: "asset:///images/shuffleOff.png"
                        imageSourceChecked: "asset:///images/shuffleOn.png"
                        onCheckedChanged: {
                            if (checked) {
                                _app.shuffle(false);
                            } else {
                                _app.unshuffle();
                            }
                        }
                        checked: _app.isShuffled
                    }
                }
            }
                Progress {
                    id: songProgress
                    horizontalAlignment: HorizontalAlignment.Left
                    verticalAlignment: VerticalAlignment.Top

                    duration: _app.currSong.song.durationMillis
                    position: nowPlaying.position
                }
            }
    }
    Container {
        // This is to show the next song
        minHeight: 50
        preferredHeight: 20
        translationY: -50
        layout: DockLayout {
            
        }
        horizontalAlignment: HorizontalAlignment.Fill
        Container {
            background: Color.Black
            opacity: 0.6
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
        }
        
        Container {
            layout: DockLayout {
            }
            leftPadding: 20
            verticalAlignment: VerticalAlignment.Top
            horizontalAlignment: HorizontalAlignment.Fill
        	Label {
            	id: nextTitle
            	text: _app.nextSong.nextSongLabel
            	verticalAlignment: VerticalAlignment.Center
            	//text: "Song Title - Song Artist"
            	textStyle {
                	color: Color.White
                	base: SystemDefaults.TextStyles.SubtitleText
                }
            }
        	
        	ImageView {
        	    imageSource: "asset:///images/nextsmall.png"
        	    scalingMethod: ScalingMethod.AspectFit
        	    horizontalAlignment: HorizontalAlignment.Right
         	}
         }

            animations: [
            	TranslateTransition {
                	id: showNext
                	toY: 0
                	duration: 600
                },
            	TranslateTransition {
                	id: hideNext
                 	toY: -50
                	duration: 600
                }
            ]
        	
        }
        
    }
    
    actions: [
        ActionItem {
            title: qsTr("Previous")
            imageSource: "asset:///images/backwards.png"
            ActionBar.placement: ActionBarPlacement.OnBar
            onTriggered: {
                _app.playPreviousSong();
                hideNext.play();
            }
            
            shortcuts: [
            	SystemShortcut {
            	    type: SystemShortcuts.PreviousSection
            	    onTriggered: {
                        _app.playPreviousSong();
                        hideNext.play();
                    }
            	}    
            ]
        },
        ActionItem {
            title: qsTr("Play/Pause")
            imageSource: player.mediaState == MediaState.Started ? "asset:///images/pause.png" : 
                                 player.mediaState == MediaState.Paused ? "asset:///images/play.png" :
                                                                         "asset:///images/play.png"
            ActionBar.placement: ActionBarPlacement.OnBar
            onTriggered: {
                if (player.mediaState == MediaState.Started) nowPlaying.pause();
                else if (player.mediaState == MediaState.Paused) nowPlaying.play();
                else nowPlaying.aquire()
            }
        },
        ActionItem {
            title: qsTr("Next")
            imageSource: "asset:///images/forward.png"
            ActionBar.placement: ActionBarPlacement.OnBar
            onTriggered: {
                _app.playNextSong();
                hideNext.play();
            }
            
            shortcuts: [
                SystemShortcut {
                    type: SystemShortcuts.NextSection
                    onTriggered: {
                        _app.playNextSong(false);
                        hideNext.play();
                    }
                }
            ]
        },
        ActionItem {
            title: qsTr("Save Locally")
            imageSource: "asset:///images/download.png"
            enabled: !_app.isRadioSong
            onTriggered: {
                locationDialog.show();
            }
        },
        ActionItem {
            title: qsTr("Thumbs Up")
            imageSource: "asset:///images/thumbsup.png"
            onTriggered: {
                _app.currSong.thumbUp();
            }
        },
        ActionItem {
            title: qsTr("Thumbs Down")
            imageSource: "asset:///images/thumbsdown.png"
            onTriggered: {
                _app.currSong.thumbDown();
            }
        },
        ActionItem {
            title: qsTr("Remove Thumb")
            imageSource: "asset:///images/removethumb.png"
            onTriggered: {
                _app.currSong.removeThumb();
            }
        },
        ActionItem {
            title: qsTr("Refresh Album Art")
            imageSource: "asset:///images/refreshart.png"
            onTriggered: {
                _app.currSong.retryAlbumArts();
            }
        }
    ]
    
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
                        _app.currSong.saveLocally("sdcard");
                    } else {
                        _app.currSong.saveLocally("device");
                    }
                }
            }
        }
    ]
        
    onCreationCompleted: {
        _app.currSong.songProgress.connect(updateProgress);
        _app.currSongChanged.connect(hideNext.play);
        _app.currSong.needToDownload.connect(showIndicator);
        _app.nextSong.songProgress.connect(updateNextProgress);
        nowPlaying.positionChanged.connect(playerPositionChanged);
    }
    
    function playerPositionChanged(newValue) {
        gettingSongUrl.visible = false;
        if (newValue < 1000) {
            hideNext.play();
        }
        var difference = _app.currSong.song.durationMillis - newValue;
        if (difference < 30000 && difference > 2000) {
            showNext.play();
            //nowPlaying.pause();
            //_app.getNextSong();
        }
        if (difference < 2000) {
            if (!_app.nextSong.isSongDownloading()) {
                hideNext.play();
            }
            // check if next is downloading
            // if its not, then close it
        }
    }
    
    function updateProgress(newValue) {
        downloadProgress.text = newValue + "%";
        if (newValue == 0 || newValue == 100) {
            downloadProgress.visible = false;
            gettingSongUrl.visible = false;
        } else {
            gettingSongUrl.visible = true;
            downloadProgress.visible = true;
        }
    }
    function updateNextProgress(newValue) {
        if (newValue == 0 || newValue == 100) {
            if (newValue == 100) {
                var difference = _app.currSong.song.durationMillis - nowPlaying.position;
                if (difference < 5000) {
                    hideNext.play();
                    _app.playNextSong(true);
                }
            }
        }
    }
    
    function showIndicator() {
        gettingSongUrl.visible = true;
    }
    
    function disconnectSignals() {
        console.log("GMA: Signals Disconnecting");
        _app.currSongChanged.disconnect(albumArt.imageSourceChanged);
        albumArt.image = null;
        _app.currSongChanged.disconnect(songTitle.textChanged);
        songTitle.text = "";
        _app.currSongChanged.disconnect(artistName.textChanged);
        artistName.text = "";
        _app.currSongChanged.disconnect(albumName.textChanged);
        albumName.text = "";
        _app.currSongChanged.disconnect(ratingImage.imageSourceChanged);
        ratingImage.imageSource = "";
        _app.currSongChanged.disconnect(nextTitle.textChanged);
        nextTitle.text = "";
        _app.currSongChanged.disconnect(nextArtist.textChanged);
        nextArtist.text = "";

        _app.currSong.needToDownload.disconnect(showIndicator);
        _app.currSong.songProgress.disconnect(updateProgress);
        _app.nextSong.songProgress.disconnect(updateNextProgress);

        console.log("GMA: Done Disconnecting");
    }
    
}
