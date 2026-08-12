file(GLOB C_LIST "*.c")

set(DIR_BIN ${DIR_SDK}/bin)

target_sources(tassic_sdk PRIVATE
    ${C_LIST}
    ${DIR_BIN}/ptrng.c.obj
    ${DIR_BIN}/puf.c.obj
    ${DIR_BIN}/secclk.c.obj
)
