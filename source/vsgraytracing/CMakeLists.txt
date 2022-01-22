set(vsgraytracing_SOURCES
    vsgraytracing.cpp
)

add_executable(vsgraytracing ${vsgraytracing_SOURCES})
#target_include_directories(vsgraytracing PRIVATE source)
target_link_libraries(vsgraytracing vsg vsgXchange vsgImGui nlohmann_json)
set_property(TARGET vsgraytracing PROPERTY CXX_STANDARD 17)

#target_link_libraries(vsgraytracing vsg::vsg)

#if (vsgXchange_FOUND)
#    target_compile_definitions(vsgraytracing PRIVATE vsgXchange_FOUND)
#    target_link_libraries(vsgraytracing vsgXchange::vsgXchange)
#endif()

#install(TARGETS vsgraytracing RUNTIME DESTINATION bin)
