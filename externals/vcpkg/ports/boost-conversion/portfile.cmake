# Automatically generated by scripts/boost/generate-ports.ps1

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO boostorg/conversion
    REF boost-1.79.0
    SHA512 dc2ff45cd1e046039fddb4af2ea2651eabbacb047255e55be395e4d3a71bde719b6dcf5071143800f14b70522f8f9e5dde2acab871e1d0aa36c9be7d1f38ed3b
    HEAD_REF master
)

include(${CURRENT_INSTALLED_DIR}/share/boost-vcpkg-helpers/boost-modular-headers.cmake)
boost_modular_headers(SOURCE_PATH ${SOURCE_PATH})
