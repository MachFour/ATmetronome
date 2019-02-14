cmake_minimum_required(VERSION 3.12)
project(metronome)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)
include_directories(SYSTEM /usr/avr/include)


add_compile_definitions(F_CPU=8000000U)
add_compile_definitions(__AVR_ATmega328P__)

add_executable(metronome
        byte_ops.h
        Metronome.cpp
        Metronome.h
        SevenSeg.cpp
        timers.h
        ToneGen.cpp
        ToneGen.h
        millis.h
        millis.cpp
        main.cpp
        SoftTimer.cpp
        SoftTimer.h
        pindefs.h
        SevenSeg.cpp
        SevenSeg.h
        bitops.h
        )
