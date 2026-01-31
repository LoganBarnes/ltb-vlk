# ##############################################################################
# A Logan Thomas Barnes project
# ##############################################################################
function(ltb_make_app directory)
  file(
    GLOB_RECURSE
    ${directory}_app_SOURCE
    LIST_DIRECTORIES
    false
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/${directory}/*.cpp
  )

  add_executable(
    ${directory}-app
    ${${directory}_app_SOURCE}
  )
  target_link_libraries(
    ${directory}-app
    PRIVATE
    LtbVlk::LtbVlk
  )
endfunction()

ltb_make_app(api)
ltb_make_app(headless)
ltb_make_app(objs)
ltb_make_app(particles)
ltb_make_app(particles2)
