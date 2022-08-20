load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

cc_binary(
    name = "main",
    srcs = ["main.cpp"],
    deps = [
        "//marketPacketProcessor:marketPacketProcessor",
        "//marketPacketGenerator:marketPacketGenerator",
    ],
)
