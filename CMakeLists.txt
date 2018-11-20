cmake_minimum_required(VERSION 3.12)
project(metronome)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)
include_directories(SYSTEM /usr/avr/include)


add_compile_definitions(F_CPU=8000000U)
add_compile_definitions(__AVR_ATmega328P__)

add_executable(metronome
        defines.h
        Metronome.cpp
        Metronome.h
        muxed_7seg.cpp
        muxed_7seg.h
        timers.cpp
        timers.h
        tone_gen.cpp
        tone_gen.h
        millis.h
        millis.cpp
        main.cpp
        SoftTimer.cpp
        SoftTimer.h)
