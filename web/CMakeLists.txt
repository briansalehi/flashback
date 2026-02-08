cmake_minimum_required(VERSION 4.1)

file(GLOB html_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.html)
file(GLOB css_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} css/*.css)
file(GLOB js_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} js/*.js)
set(static_files ${html_files} ${css_files} ${js_files})

install(FILES ${static_files} COMPONENT web DESTINATION /var/www/html)
