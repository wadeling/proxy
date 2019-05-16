
# Echo L4 network filter. This is primarily a simplistic example.
# Public docs: docs/root/configuration/network_filters/echo_filter.rst

load(
    "@envoy//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
)


envoy_cc_library(
    name = "echo2",
    srcs = ["echo2.cc"],
    hdrs = ["echo2.h"],
    repository = "@envoy",
    external_deps = ["http_parser"],
    visibility = ["//visibility:public"],
    deps = [
        "@envoy//source/exe:envoy_common_lib",
    ],
)

envoy_cc_library(
    name = "config_lib",
    srcs = ["config.cc"],
    visibility = ["//visibility:public"],
    repository = "@envoy",
    deps = [
        ":echo2",
        "@envoy//source/exe:envoy_common_lib",
    ],
)
