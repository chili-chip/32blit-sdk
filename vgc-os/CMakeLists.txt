cmake_minimum_required(VERSION 3.15...3.31)
project (vgc-os)
find_package (32BLIT CONFIG REQUIRED PATHS ..)

blit_executable(vgc-os
    games.cpp home.cpp settings.cpp ui.cpp
)

target_link_libraries(vgc-os LauncherShared)
blit_metadata(vgc-os metadata.yml)

# factory image: the loader with the home screen already installed, flashed in one copy.
# Handler apps (players for .bitsy, .beat, ...) can be preinstalled after it.
set(VGC_OS_FACTORY_APPS "" CACHE STRING "Handler .blit files to preinstall in the factory image (;-separated)")
get_property(VGC_OS_PICO_BLIT TARGET vgc-os PROPERTY BLIT_PICO_BLIT)
if(VGC_OS_PICO_BLIT AND TARGET blit-loader AND PICO_BOARD STREQUAL "chilichip_vgc")
    find_package(Python3 REQUIRED COMPONENTS Interpreter)

    set(LOADER_UF2 $<TARGET_FILE_DIR:blit-loader>/blit-loader.uf2)
    set(VGC_OS_BLIT ${CMAKE_CURRENT_BINARY_DIR}/vgc-os.blit)
    set(FACTORY_UF2 ${CMAKE_CURRENT_BINARY_DIR}/vgc-os-factory.uf2)

    add_custom_command(
        OUTPUT ${FACTORY_UF2}
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/../tools/factory-uf2.py ${LOADER_UF2} ${VGC_OS_BLIT}@256 ${VGC_OS_FACTORY_APPS} -o ${FACTORY_UF2}
        DEPENDS blit-loader vgc-os ${LOADER_UF2} ${VGC_OS_BLIT} ${VGC_OS_FACTORY_APPS} ${CMAKE_CURRENT_LIST_DIR}/../tools/factory-uf2.py
        COMMENT "Building vgc-os-factory.uf2"
        VERBATIM
    )
    add_custom_target(vgc-os-factory ALL DEPENDS ${FACTORY_UF2})

    install(FILES ${FACTORY_UF2} DESTINATION bin)
endif()
