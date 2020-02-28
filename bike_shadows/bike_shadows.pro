QT += opengl

HEADERS += \
    objmodel.h \
    simplerenderwindow.h \
    shadowrenderwindow.h

SOURCES += \
    objmodel.cpp \
    main.cpp \
    shadowrenderwindow.cpp \
    simplerenderwindow.cpp

RESOURCES += \
    bike_shadows.qrc

DISTFILES += \
    platform.obj \
    scene_fragment.glsl \
    scene_vertex.glsl

