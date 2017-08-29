import bb.cascades 1.0
import bb.system 1.0

Page {
    
    titleBar: TitleBar { 
        title: "Crash Handler"
        kind: TitleBarKind.Default
    }
    
    ScrollView {
        
        Container {
            layout: StackLayout {}
            topPadding: 25
            leftPadding: 25
            rightPadding: 25
            bottomPadding: 50
            
            Label {
                multiline: true
                text: "GoMusic has detected the presence of a crash report, which indicates that GoMusic may have crashed."
            }
            
            Divider { }
            
            Button {
                text: "Email Crash Report"
                horizontalAlignment: HorizontalAlignment.Center
                onClicked: {
                    model.sendCrashReport();
                }
            }
            
            Divider { }
            
            Button {
                text: "Start Application"
                horizontalAlignment: HorizontalAlignment.Center
                onClicked: {
                    model.startApplication();
                }
            }
            
            Divider { }
        }
    }
}