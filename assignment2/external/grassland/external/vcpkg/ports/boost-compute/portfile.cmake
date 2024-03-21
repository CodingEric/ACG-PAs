# Automatically generated by scripts/boost/generate-ports.ps1

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO boostorg/compute
    REF boost-1.83.0
    SHA512 bb555cffc7761a1dba3b010aa45782af22d4bac049ecfa9d26695a5eb3c5722b29f5f7b30143d17698b2098f81dcc7eaef2425482c5a6495e29324b1f8f324b2
    HEAD_REF master
)

include(${CURRENT_INSTALLED_DIR}/share/boost-vcpkg-helpers/boost-modular-headers.cmake)
boost_modular_headers(SOURCE_PATH ${SOURCE_PATH})