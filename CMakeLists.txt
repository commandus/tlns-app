cmake_minimum_required(VERSION 2.6)

project(tlns-app)

include_directories(. third-party third-party/linenoise)

# build example
add_executable(tlns-app-cli
    tlns-app-cli.cpp
    lorawan/helper/aes-helper.cpp
    lorawan/file-helper.cpp
    lorawan/lorawan-conv.cpp
    lorawan/lorawan-date.cpp
    lorawan/lorawan-mac.cpp
    lorawan/lorawan-mic.cpp
    lorawan/lorawan-packet-storage.cpp
    lorawan/lorawan-string.cpp
    lorawan/lorawan-types.cpp
    lorawan/proto/payload2device/payload2device-parser.cpp
    lorawan/storage/network-identity.cpp
    third-party/base64/base64.cpp
    third-party/system/crypto/aes.c
    third-party/system/crypto/cmac.c
    third-party/linenoise/linenoise.c
)
