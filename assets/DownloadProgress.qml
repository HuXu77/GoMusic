import bb.cascades 1.0

Container {
    id: root
    
    property alias value: proI.value
    property alias label: percent.text
    property alias toValue: proI.toValue
    
    layout: StackLayout {
                orientation: LayoutOrientation.LeftToRight
            }
            
    ProgressIndicator {
            id: proI
            verticalAlignment: VerticalAlignment.Center
            preferredWidth: 500
            
            fromValue: 0
            toValue: 100
            value: 1
        }
    Label {
        id: percent
        verticalAlignment: VerticalAlignment.Center
            
        text: "999/1000"
        preferredWidth: 150
        textStyle {
            base: SystemDefaults.TextStyles.SmallText
            color: Color.White
        }
    }
        
    
}

