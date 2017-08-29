import bb.cascades 1.0

    Container {
        layout: DockLayout {}
        verticalAlignment: VerticalAlignment.Fill
        ImageView {
            id: albumArt
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Top
            scalingMethod: ScalingMethod.AspectFill
            //imageSource: _app.currSong.albumArt
            image: _app.currSong.albumArt
            minHeight: 397
        }
        ImageView {
            verticalAlignment: VerticalAlignment.Center
            horizontalAlignment: HorizontalAlignment.Fill
            imageSource: "asset:///images/gradient.png"
        }
        Container {
            verticalAlignment: VerticalAlignment.Center
            leftPadding: 20
            topPadding: 50
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
        }
    }
