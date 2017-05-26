project(libxenbe)

set(SOURCES
	BackendBase.cpp
	FrontendHandlerBase.cpp
	RingBufferBase.cpp
	Log.cpp
	Utils.cpp
	XenCtrl.cpp
	XenEvtchn.cpp
	XenGnttab.cpp
	XenStat.cpp
	XenStore.cpp
)

add_library(xenbe SHARED ${SOURCES})

target_link_libraries(xenbe
	xenstore
	xenevtchn
	xengnttab
	xenctrl
	pthread
)

install(TARGETS xenbe LIBRARY DESTINATION lib)
