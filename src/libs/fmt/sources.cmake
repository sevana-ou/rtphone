set (CD ${CMAKE_CURRENT_LIST_DIR})

set (SRC
    ${CD}/src/format.cc
    ${CD}/src/os.cc
    )

set (HDR
    ${CD}/include/fmt/args.h
    ${CD}/include/fmt/chrono.h
    ${CD}/include/fmt/color.h
    ${CD}/include/fmt/compile.h
    ${CD}/include/fmt/core.h
    ${CD}/include/fmt/format.h
    ${CD}/include/fmt/format-inl.h
    ${CD}/include/fmt/os.h
    ${CD}/include/fmt/ostream.h
    ${CD}/include/fmt/printf.h
    ${CD}/include/fmt/ranges.h
    ${CD}/include/fmt/std.h
    ${CD}/include/fmt/xchar.h
    )


set (SOURCES_FMT ${SRC} ${HDR})

