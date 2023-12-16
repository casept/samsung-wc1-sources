# For i18n of error page.
SET(PO_DIR "${WEBKIT2_DIR}/UIProcess/efl/po_tizen")
SET(MSGFMT "/usr/bin/msgfmt")
SET(WEBKIT2_POFILE_DIR share/)

FILE(GLOB PO_FILES ${PO_DIR}/*.po)

SET(PO_GENERATED_FILES "")

FOREACH(pofile ${PO_FILES})
    GET_FILENAME_COMPONENT(absPofile ${pofile} ABSOLUTE)
    GET_FILENAME_COMPONENT(lang ${absPofile} NAME_WE)

    SET(_target_dir ${CMAKE_BINARY_DIR}/WebKit2/locale/${lang}/LC_MESSAGES)
    SET(_moFile ${_target_dir}/${PROJECT_NAME}.mo)

    FILE(MAKE_DIRECTORY ${_target_dir})
    ADD_CUSTOM_COMMAND(
        OUTPUT ${_moFile}
        COMMAND ${MSGFMT} -o ${_moFile} ${absPofile}
        DEPENDS ${absPofile}
    )
    LIST(APPEND PO_GENERATED_FILES ${_moFile})
ENDFOREACH(pofile)

INSTALL(DIRECTORY ${CMAKE_BINARY_DIR}/WebKit2/locale DESTINATION ${WEBKIT2_POFILE_DIR})

ADD_CUSTOM_TARGET(po ALL DEPENDS ${PO_GENERATED_FILES})
