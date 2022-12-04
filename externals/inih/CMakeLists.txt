# SPDX-FileCopyrightText: 2014 Gui Andrade <admin@archshift.com>
# SPDX-License-Identifier: GPL-2.0-or-later

add_library(inih
    inih/ini.c
    inih/ini.h
    inih/cpp/INIReader.cpp
    inih/cpp/INIReader.h
)

create_target_directory_groups(inih)
target_include_directories(inih INTERFACE inih/cpp)
add_library(inih::INIReader ALIAS inih)
