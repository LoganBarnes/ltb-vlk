# ##############################################################################
# A Logan Thomas Barnes project
# ##############################################################################
file(
  GLOB_RECURSE LtbVlk_SHADERS
  LIST_DIRECTORIES false
  CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_LIST_DIR}/*.vert
  ${CMAKE_CURRENT_LIST_DIR}/*.geom
  ${CMAKE_CURRENT_LIST_DIR}/*.tesc
  ${CMAKE_CURRENT_LIST_DIR}/*.tese
  ${CMAKE_CURRENT_LIST_DIR}/*.frag
  ${CMAKE_CURRENT_LIST_DIR}/*.comp
)
file(
  GLOB_RECURSE LtbVlk_SHADER_INCLUDES
  LIST_DIRECTORIES false
  CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_LIST_DIR}/*.glsl
)

add_library(
  ltb_vlk_generate_spirv
  INTERFACE
)
add_library(
  LtbVlk::generate_spirv
  ALIAS
  ltb_vlk_generate_spirv
)

# Generate the SPIR-V file for each shader in the build directory (mirroring this
# directory's layout) so separate build directories don't overwrite each other.
foreach( SHADER_FILE ${LtbVlk_SHADERS} )
  file( RELATIVE_PATH SHADER_RELATIVE_PATH ${CMAKE_CURRENT_LIST_DIR} ${SHADER_FILE} )
  set( SPIRV_FILE ${LTB_VLK_SPIRV_DIR}/${SHADER_RELATIVE_PATH}.spv )
  get_filename_component( SPIRV_DIR ${SPIRV_FILE} DIRECTORY )

  add_custom_command(
    OUTPUT ${SPIRV_FILE}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SPIRV_DIR}
    COMMAND $<TARGET_FILE:Vulkan::glslc>
    ARGS ${SHADER_FILE} -o ${SPIRV_FILE}
    DEPENDS ${SHADER_FILE} ${LtbVlk_SHADER_INCLUDES}
    VERBATIM
  )

  set_source_files_properties(
    ${SPIRV_FILE}
    PROPERTIES
    GENERATED TRUE
  )
  target_sources(
    ltb_vlk_generate_spirv
    PRIVATE
    ${SPIRV_FILE}
  )
endforeach()
