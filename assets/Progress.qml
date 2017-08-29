import bb.cascades 1.0

Container {
    id: root

    property int duration: 1
    property int position: 1
    

    layout: DockLayout {
    }
    
    minHeight: 165
    
    Label {
        id: starting
        
        textStyle {
            base: SystemDefaults.TextStyles.SmallText
            color: Color.Gray
        }
        verticalAlignment: VerticalAlignment.Bottom
    }
    

    Slider {
        id: slider
        
        verticalAlignment: VerticalAlignment.Center
        preferredWidth: 600

        fromValue: 0
        toValue: root.duration
        value: root.position
        
        onTouch: {
            if (event.touchType == TouchType.Down) {
            	player.pause();
            } else if (event.touchType == TouchType.Up) {
                if (!_app.currSong.isSongDownloading()) {
                    player.setSourceUrl("");
                    player.setSourceUrl(_app.currSong.getUrlLocation());
                    player.play();
                    //console.log("Curr Pos: " + root.position + " Slider : " + slider.immediateValue);
                    player.seekTime(slider.immediateValue);
                }
            }
        }
        
        onImmediateValueChanged: {
            var minutes = Math.floor(value / 1000 / 60);
            var seconds = Math.floor(value / 1000 % 60);
            starting.text = qsTr("%1:%2").arg(minutes < 10 ? "0" + minutes : "" + minutes).arg(seconds < 10 ? "0" + seconds : "" + seconds);
        }
    }

    Label {
        verticalAlignment: VerticalAlignment.Bottom
        horizontalAlignment: HorizontalAlignment.Right

        property int finalMinutes: Math.floor(root.duration/1000/60)
        property int finalSeconds: Math.floor(root.duration/1000%60)

        text: qsTr("%1:%2").arg(finalMinutes < 10 ? "0" + finalMinutes : "" + finalMinutes)
                            .arg(finalSeconds < 10 ? "0" + finalSeconds : "" + finalSeconds)
        textStyle {
            base: SystemDefaults.TextStyles.SmallText
            color: Color.Gray
        }
    }
}