APP_NAME = GoMusic

CONFIG += qt warn_on cascades10

LIBS += -lbbdata -lbbmultimedia -lbbsystem -lbb -lbps -lhuapi -lbbdevice -lslog2 -lbbpim

include(config.pri)

device {
    CONFIG(debug, debug|release) {
        # Device-Debug custom configuration
    }

    CONFIG(release, debug|release) {
        # Device-Release custom configuration
        # to remove debug from release: /Applications/Momentics.app/target_10_2_0_1155/qnx6/usr/share/qt4/mkspecs/features/qt.prf
    }
}

simulator {
    CONFIG(debug, debug|release) {
        # Simulator-Debug custom configuration
    }
}
