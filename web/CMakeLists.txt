cmake_minimum_required(VERSION 4.1)

find_package(Protobuf 3.19.0 REQUIRED)
find_program(NPM_EXECUTABLE NAMES npm REQUIRED)
find_program(NPX_EXECUTABLE NAMES npx REQUIRED)

file(GLOB html_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.html)
file(GLOB css_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} css/*.css)
file(GLOB js_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} js/*.js)
file(GLOB image_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} images/*.png)
set(icons favicon.ico)

set(PROTOC_GEN_GRPC_WEB_EXECUTABLE ${CMAKE_CURRENT_BINARY_DIR}/node_modules/protoc-gen-grpc-web/bin/protoc-gen-grpc-web)
set(PROTOC_GEN_JS_EXECUTABLE ${CMAKE_CURRENT_BINARY_DIR}/node_modules/protoc-gen-js/bin/protoc-gen-js)

set(client_sources
    ${CMAKE_CURRENT_BINARY_DIR}/types_pb.js
    ${CMAKE_CURRENT_BINARY_DIR}/requests_pb.js
    ${CMAKE_CURRENT_BINARY_DIR}/server_pb.js
    ${CMAKE_CURRENT_BINARY_DIR}/server_grpc_web_pb.js
)

file(GLOB proto_files ${CMAKE_SOURCE_DIR}/common/proto/*.proto)

set(configs
    ${CMAKE_CURRENT_SOURCE_DIR}/package.json
    ${CMAKE_CURRENT_SOURCE_DIR}/bundle.js
    ${CMAKE_CURRENT_SOURCE_DIR}/webpack.config.js
)

set(bundle ${CMAKE_CURRENT_BINARY_DIR}/flashback.js)

add_custom_command(
    OUTPUT ${PROTOC_GEN_GRPC_WEB_EXECUTABLE} ${PROTOC_GEN_JS_EXECUTABLE}
    COMMAND cp
    ARGS ${configs} ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${NPM_EXECUTABLE}
    ARGS install --loglevel silent --no-package-lock --save-dev
    COMMENT "Installing protoc-gen-grpc-web and protoc-gen-js for client"
)

add_custom_command(
    OUTPUT ${client_sources}
    COMMAND ${Protobuf_PROTOC_EXECUTABLE}
    ARGS
    -I ${CMAKE_SOURCE_DIR}/common/proto
    --js_out=import_style=commonjs:${CMAKE_CURRENT_BINARY_DIR}
    --grpc-web_out=import_style=commonjs,mode=grpcwebtext:${CMAKE_CURRENT_BINARY_DIR}
    --plugin=protoc-gen-grpc-web=${PROTOC_GEN_GRPC_WEB_EXECUTABLE}
    --plugin=protoc-gen-js=${PROTOC_GEN_JS_EXECUTABLE}
    ${proto_files}
    DEPENDS ${proto_files} ${PROTOC_GEN_GRPC_WEB_EXECUTABLE} ${PROTOC_GEN_JS_EXECUTABLE}
    COMMENT "Generating client stub"
)

add_custom_command(
    OUTPUT ${bundle}
    COMMAND ${NPX_EXECUTABLE}
    ARGS webpack --no-stats
    DEPENDS ${client_sources} ${configs}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating client bundle"
)

add_custom_target(generate-bundle ALL DEPENDS ${bundle} COMMENT "Generating flashback bundle")
add_dependencies(common generate-bundle)

install(FILES ${html_files} COMPONENT web DESTINATION /var/www/html)
install(FILES ${css_files} COMPONENT web DESTINATION /var/www/html/css)
install(FILES ${js_files} COMPONENT web DESTINATION /var/www/html/js)
install(FILES ${image_files} COMPONENT web DESTINATION /var/www/html/images)
install(FILES ${bundle} COMPONENT web DESTINATION /var/www/html/proto/)
install(FILES ${icons} COMPONENT web DESTINATION /var/www/html)
