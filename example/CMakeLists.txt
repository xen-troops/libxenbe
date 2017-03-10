project(libxenbe)

set(SOURCES
	ExampleBackend.cpp
)

add_executable(example_be ${SOURCES})

target_link_libraries(example_be
	xenbe
	xenctrl
	xenstore
	xengnttab
	xenevtchn
)
