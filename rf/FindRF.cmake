
SET(_SOURCES
	arctech.c
	crc.c
	everflourish.c
	fineoffset.c
	hasta.c
	mandolyn.c
	oregon.c
	oregonv3.c
	receive.c
	transmit.c
	x10.c
	pt2262.c 
)

FOREACH(_file ${_SOURCES})
	GET_FILENAME_COMPONENT(_abs_file ${CMAKE_CURRENT_LIST_DIR}/${_file} ABSOLUTE)
	SET_PROPERTY(
		SOURCE ${_abs_file}
		PROPERTY P1_PATH "RF"
	)
	
	LIST(APPEND RF_SOURCES ${_abs_file})
ENDFOREACH(_file)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_LIST_DIR})

