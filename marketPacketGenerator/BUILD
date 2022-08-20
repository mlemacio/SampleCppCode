load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "marketPacketGenerator",
    srcs = ["marketPacketGenerator.cpp"],
    hdrs = ["marketPacketGenerator.h"],
    deps = [
        "//marketPacketHelpers:marketPacketHelpers",
    ],
    visibility = ["//main:__pkg__",
                  "//marketPacketProcessor/test:__pkg__",
                  "//marketPacketGenerator/test:__pkg__",
    ]
)