if(NOT DEFINED CRT_FILE)
	set (CRT_FILE "crt.s")
endif()


target_sources(${SDK_TARGET} PRIVATE
	${CRT_FILE}
)
