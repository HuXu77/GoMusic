import bb.cascades 1.0

Container {
    property alias container: bodyContainer
    property alias status: header.text
    Button {
            id: header
            text: "header"
            preferredWidth: 768
        }
    Container {
        id: bodyContainer
    }
}
