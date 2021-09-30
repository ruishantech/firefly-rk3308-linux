cmake_minimum_required(VERSION 3.0.2)

PROJECT (rkffplay)

set(SRC_FILES ffplay.c cmdutils.c buffers.c kmssink.c util/format.c util/kms.c util/pattern.c)

add_executable(rkffplay ${SRC_FILES})

if(${CONFIG_ROCKCHIP_PLATEFORM} MATCHES "rv1108")
	add_definitions(-DCONFIG_ROCKCHIP_RV1108)

	target_include_directories(rkffplay PUBLIC
		"${rkffplay_SOURCE_DIR}/../../../external/ffmpeg/")

	LINK_DIRECTORIES("${rkffplay_SOURCE_DIR}/../../../out/system/lib/")

	target_link_libraries(rkffplay
		avutil avcodec avformat swresample
		SDL2 rkfb ion rkrga m z mpp asound)
else()
	find_package(PkgConfig)
	pkg_check_modules (DRM libdrm REQUIRED)
	include_directories(${DRM_INCLUDE_DIRS})
	target_link_libraries (rkffplay ${DRM_LIBRARIES})
	target_include_directories(rkffplay PUBLIC
                "${rkffplay_SOURCE_DIR}/../../ffmpeg4-4.1.3")

        target_link_libraries(rkffplay
                avutil avcodec avformat swresample avfilter avdevice swscale
                SDL2 m z asound)
endif()

install(TARGETS rkffplay DESTINATION bin)
