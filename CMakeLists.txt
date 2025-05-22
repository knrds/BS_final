cmake_minimum_required(VERSION 3.20)
project(Betriebssysteme C)

#== Funktionen einbinden ==#
include(${CMAKE_CURRENT_SOURCE_DIR}/.cmake/functions.cmake)

#== ASAN ==#
# Ändern Sie den Wert von "OFF" zu "ON", um AddressSanitizer zu aktivieren
option(USE_ASAN "AddressSanitizer aktivieren" OFF)
compile_with_asan(${USE_ASAN})

#== Compiler Flags ==#
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_COMPILER gcc)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic -Wall -Werror -Wextra -Wconversion -g")

# Hier definieren wir das Feature-Test-Makro, damit z.B. ftruncate sichtbar ist.
add_compile_definitions(_XOPEN_SOURCE=700)

#== Add Library target ==#
set(LIBRARY_SRC_FILES
        src/osmpLibrary/osmpLib.c
        src/osmpLibrary/osmpLib.h
        src/osmpLibrary/OSMP.h
        src/osmpLibrary/barrier.h
        src/osmpLibrary/barrier.c
)
add_library(libosmp STATIC ${LIBRARY_SRC_FILES})

#== Define Libraries ALL executables link against ==#
set(LIBRARIES
        pthread
        libosmp
)

#== Executables ==#
set(EXECUTABLES
        NAME osmprun
        SOURCES src/osmpRunner/osmpRun.c
        NAME echoAll
        SOURCES src/osmpExecutables/echoAll.c
        NAME echoAllBase
        SOURCES src/osmpExecutables/echoAll_base.c
        NAME testSendRec
        SOURCES src/osmpExecutables/testSendRec.c
        NAME testSendRecMulti
        SOURCES src/osmpExecutables/testSendRecMulti.c
        NAME testSendRecBroadcast
        SOURCES src/osmpExecutables/testSendRecBroadcast.c
        NAME testSendRecBlock
        SOURCES src/osmpExecutables/testSendRecBlock.c
        NAME testSendRecRace
        SOURCES src/osmpExecutables/testSendRecRace.c
        NAME testBarrier
        SOURCES src/osmpExecutables/testBarrier.c
        NAME testBarrierMulti
        SOURCES src/osmpExecutables/testBarrierMulti.c
        NAME testGather
        SOURCES src/osmpExecutables/testGather.c
        NAME testGatherABC
        SOURCES src/osmpExecutables/testGatherABC.c
        NAME testSendRank
        SOURCES src/osmpExecutables/testSendRank.c
        NAME testAllRoot
        SOURCES src/osmpExecutables/testAllRoot.c
        # Neue Executables hier einfügen
        # NAME <name>
        # SOURCES <source1> <source2> ...
)

validate_executables()
create_executables()