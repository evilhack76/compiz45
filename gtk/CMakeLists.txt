option (BUILD_GTK "Build gtk-window-decorator and Gnome related toold" 1)
option (BUILD_METACITY "Gtk-window-decorator metacity theme support" 1)
option (BUILD_GNOME "GNOME integration" 1)

compiz_set (USE_GTK ${BUILD_GTK})
compiz_set (USE_METACITY ${BUILD_METACITY})
compiz_set (USE_GNOME ${BUILD_GNOME})

if (USE_GTK)
    pkg_check_modules (GTK_WINDOW_DECORATOR
	xrender>=0.8.4
	gtk+-3.0
	libwnck-3.0
	pangocairo
    )
    if (GTK_WINDOW_DECORATOR_FOUND)
        if (USE_METACITY)
            pkg_check_modules (LIBMETACITY libmetacity>=3.22.0)

            if (LIBMETACITY_FOUND)
                compiz_pkg_check_modules (HAVE_METACITY_3_26_0 libmetacity>=3.25.1)

                set (METACITY_INCLUDE_DIRS ${LIBMETACITY_INCLUDE_DIRS})
                set (METACITY_LIBRARIES ${LIBMETACITY_LIBRARIES})
            else (LIBMETACITY_FOUND)
                compiz_set (USE_METACITY 0)
            endif (LIBMETACITY_FOUND)
        endif (USE_METACITY)

    else (GTK_WINDOW_DECORATOR_FOUND)
	compiz_set (USE_GTK 0)
    endif (GTK_WINDOW_DECORATOR_FOUND)
endif (USE_GTK)

if (NOT USE_GTK)
    compiz_set (USE_METACITY 0)
endif (NOT USE_GTK)

configure_file (
    ${CMAKE_CURRENT_SOURCE_DIR}/config.h.gtk.in
    ${CMAKE_CURRENT_BINARY_DIR}/config.h
)

add_subdirectory (window-decorator)
add_subdirectory (gnome)
