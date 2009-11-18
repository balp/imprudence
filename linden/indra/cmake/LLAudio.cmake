# -*- cmake -*-

include(Audio)
include(OPENAL)

set(LLAUDIO_INCLUDE_DIRS
    ${LIBS_OPEN_DIR}/llaudio
    ${OPENAL_INCLUDE_DIRS}
    )

set(LLAUDIO_LIBRARIES
    llaudio
    ${VORBISENC_LIBRARIES}
    ${VORBISFILE_LIBRARIES}
    ${VORBIS_LIBRARIES}
    ${OGG_LIBRARIES}
    ${OPENAL_LIBRARIES}
    )
