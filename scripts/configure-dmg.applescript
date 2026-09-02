on run arguments
    set mountPath to item 1 of arguments
    set mountedFolder to (POSIX file mountPath) as alias
    set backgroundFile to (POSIX file (mountPath & "/.background/background.png")) as alias

    tell application "Finder"
        open mountedFolder
        set installerWindow to container window of mountedFolder
        set current view of installerWindow to icon view
        set toolbar visible of installerWindow to false
        set statusbar visible of installerWindow to false
        set pathbar visible of installerWindow to false
        set bounds of installerWindow to {100, 100, 760, 528}

        set iconOptions to icon view options of installerWindow
        set arrangement of iconOptions to not arranged
        set icon size of iconOptions to 128
        set text size of iconOptions to 14
        set shows icon preview of iconOptions to true
        set background picture of iconOptions to backgroundFile

        set position of item "VeyloPlayer.app" of mountedFolder to {160, 205}
        set position of item "Applications" of mountedFolder to {500, 205}

        update mountedFolder without registering applications
        delay 2
        close installerWindow
        -- Finder writes the view metadata after the window closes. Give that
        -- asynchronous write time to reach the writable disk image before it
        -- is detached and converted to the final read-only DMG.
        delay 3
    end tell
end run
