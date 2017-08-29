import bb.cascades 1.0

StandardListItem {
    
    id: me
    property alias albumArtist: me.description
    
    title: ListItemData.title
    description: ListItemData.artist + " - " + ListItemData.album

    imageSpaceReserved: false
    
}
