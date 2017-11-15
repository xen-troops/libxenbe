project(libxenbe)

################################################################################
# Includes
################################################################################

################################################################################
# Sources
################################################################################

set(SOURCES
	BackendBase.cpp
	FrontendHandlerBase.cpp
	RingBufferBase.cpp
	Utils.cpp
	XenCtrl.cpp
	XenEvtchn.cpp
	XenGnttab.cpp
	XenStat.cpp
	XenStore.cpp
)

################################################################################
# Targets
################################################################################

add_library(xenbe SHARED ${SOURCES})

if(WITH_TEST)
	add_library(xenbemock SHARED ${SOURCES})
endif()

################################################################################
# Libraries
################################################################################

target_link_libraries(xenbe
	xenstore
	xenevtchn
	xengnttab
	xenctrl
	pthread
)

if(WITH_TEST)
	target_link_libraries(xenbemock
		xenmock
		pthread
	)
endif()

install(TARGETS xenbe LIBRARY DESTINATION lib)
