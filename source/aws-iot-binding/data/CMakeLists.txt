##################################################
# Config/Cert files
##################################################
PROJECT_TARGET_ADD(aws-iot-service-config)

  file(GLOB JSON_FILES "config/*.json")
  file(GLOB CERT_FILES "certs/*")

  add_custom_target(${TARGET_NAME}
     DEPENDS  ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}
  )

  add_custom_command(
     DEPENDS  ${JSON_FILES}
     OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}
     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
     COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}/config
     COMMAND touch ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}/config
     COMMAND cp -r ${JSON_FILES} ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}/config
     COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}/certs
     COMMAND touch ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}/certs
     COMMAND cp -r ${CERT_FILES} ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}/certs
  )

  SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES
   LABELS "DATA"
    OUTPUT_NAME ${TARGET_NAME}
  )
