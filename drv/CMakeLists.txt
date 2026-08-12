file(GLOB C_LIST "*.c")

target_sources(tassic_sdk PRIVATE
    ${C_LIST}
)
