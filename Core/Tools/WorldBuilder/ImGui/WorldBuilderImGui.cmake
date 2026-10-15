include(${CMAKE_SOURCE_DIR}/cmake/imgui.cmake)

set(WORLDBUILDER_PRECOMPILED_HEADERS
    [["Utility/CppMacros.h"]]
    [["WWLib/always.h"]]
    [["Common/STLTypedefs.h"]]
    [["StdAfx.h"]]
    [["WWLib/WWCommon.h"]]
)

set(WORLDBUILDER_IMGUI_DIR ${CMAKE_CURRENT_LIST_DIR})

set(WORLDBUILDER_USER32_SRC
    compat/CommCtrl.h
    compat/CommDlg.h
    compat/RichEdit.h
    compat/Win32Api.h
    compat/WinBaseExtras.h
    compat/WinGdi.h
    compat/WinUser.h
    compat/WinUserMessages.h
    compat/WinUserStyles.h
    user32/Win32Class.cpp
    user32/Win32Class.h
    user32/Win32ColorDialog.cpp
    user32/Win32CommonDialogs.cpp
    user32/Win32CommonDialogs.h
    user32/Win32Control.cpp
    user32/Win32Control.h
    user32/Win32DefWindowProc.cpp
    user32/Win32Dialog.cpp
    user32/Win32Dialog.h
    user32/Win32DialogParser.cpp
    user32/Win32FileDialogs.cpp
    user32/Win32Focus.cpp
    user32/Win32Gdi.h
    user32/Win32GdiDraw.cpp
    user32/Win32GdiObjects.cpp
    user32/Win32GdiRegions.cpp
    user32/Win32Geometry.cpp
    user32/Win32Images.cpp
    user32/Win32Input.cpp
    user32/Win32Input.h
    user32/Win32InputQueue.cpp
    user32/Win32InputQueue.h
    user32/Win32Menu.cpp
    user32/Win32Menu.h
    user32/Win32Messages.cpp
    user32/Win32Messages.h
    user32/Win32Metrics.cpp
    user32/Win32Metrics.h
    user32/Win32Paths.cpp
    user32/Win32Paths.h
    user32/Win32Platform.h
    user32/Win32Profile.cpp
    user32/Win32Profile.h
    user32/Win32PropertySheet.cpp
    user32/Win32PropertySheet.h
    user32/Win32PropertySheetPages.cpp
    user32/Win32ResourceParser.cpp
    user32/Win32ResourceParser.h
    user32/Win32Resources.cpp
    user32/Win32Resources.h
    user32/Win32ResourceScript.cpp
    user32/Win32ResourceScript.h
    user32/Win32ScrollBars.cpp
    user32/Win32System.cpp
    user32/Win32SystemColors.cpp
    user32/Win32SystemColors.h
    user32/Win32Window.cpp
    user32/Win32Window.h
    user32/Win32WindowApi.cpp
    user32/controls/Win32Button.cpp
    user32/controls/Win32Button.h
    user32/controls/Win32ButtonDraw.cpp
    user32/controls/Win32ColorPicker.cpp
    user32/controls/Win32ColorPicker.h
    user32/controls/Win32ComboBox.cpp
    user32/controls/Win32ComboBox.h
    user32/controls/Win32ComboBoxDraw.cpp
    user32/controls/Win32ControlDrawing.cpp
    user32/controls/Win32ControlDrawing.h
    user32/controls/Win32Controls.cpp
    user32/controls/Win32Controls.h
    user32/controls/Win32Edit.cpp
    user32/controls/Win32Edit.h
    user32/controls/Win32EditMessages.cpp
    user32/controls/Win32ImageList.cpp
    user32/controls/Win32ImageList.h
    user32/controls/Win32ItemList.cpp
    user32/controls/Win32ItemList.h
    user32/controls/Win32ListBox.cpp
    user32/controls/Win32ListBox.h
    user32/controls/Win32ListView.h
    user32/controls/Win32ListViewItems.cpp
    user32/controls/Win32ListViewMessages.cpp
    user32/controls/Win32RichEdit.cpp
    user32/controls/Win32RichEdit.h
    user32/controls/Win32RichEditInput.cpp
    user32/controls/Win32Static.cpp
    user32/controls/Win32Static.h
    user32/controls/Win32StatusBar.cpp
    user32/controls/Win32StatusBar.h
    user32/controls/Win32TabControl.cpp
    user32/controls/Win32TabControl.h
    user32/controls/Win32Toolbar.cpp
    user32/controls/Win32Toolbar.h
    user32/controls/Win32ToolbarDraw.cpp
    user32/controls/Win32Trackbar.cpp
    user32/controls/Win32Trackbar.h
    user32/controls/Win32TreeView.h
    user32/controls/Win32TreeViewDraw.cpp
    user32/controls/Win32TreeViewInput.cpp
    user32/controls/Win32TreeViewItems.cpp
    user32/controls/Win32TreeViewMessages.cpp
    user32/presentation/Win32KeyboardRouter.cpp
    user32/presentation/Win32KeyboardRouter.h
    user32/presentation/Win32MenuPresenter.cpp
    user32/presentation/Win32MenuPresenter.h
    user32/presentation/Win32MouseRouter.cpp
    user32/presentation/Win32MouseRouter.h
    user32/presentation/Win32Renderer.cpp
    user32/presentation/Win32Renderer.h
    user32/presentation/Win32WindowPresenter.cpp
    user32/presentation/Win32WindowPresenter.h
    d3dx/D3DXFont.cpp
)

list(TRANSFORM WORLDBUILDER_USER32_SRC PREPEND ${WORLDBUILDER_IMGUI_DIR}/)

list(APPEND WORLDBUILDER_USER32_SRC
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/ImGui/imgui_impl_dx8wrapper.cpp
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/ImGui/imgui_impl_dx8wrapper.h
)

set(WORLDBUILDER_MFC_SRC
    compat/afx.h
    compat/afxcmn.h
    compat/afxdlgs.h
    compat/afxdtctl.h
    compat/afxext.h
    compat/afxres.h
    compat/afxwin.h
    compat/d3dx8.h
    compat/PrSht.h
    compat/mfc/AfxApp.h
    compat/mfc/AfxBars.h
    compat/mfc/AfxCmdTarget.h
    compat/mfc/AfxCommonControls.h
    compat/mfc/AfxControls.h
    compat/mfc/AfxCore.h
    compat/mfc/AfxDialog.h
    compat/mfc/AfxDialogs.h
    compat/mfc/AfxDocument.h
    compat/mfc/AfxFile.h
    compat/mfc/AfxFrame.h
    compat/mfc/AfxGdi.h
    compat/mfc/AfxGeometry.h
    compat/mfc/AfxMenu.h
    compat/mfc/AfxMessageMacros.h
    compat/mfc/AfxStandardResources.h
    compat/mfc/AfxString.h
    compat/mfc/AfxView.h
    compat/mfc/AfxWnd.h
    mfc/AfxApp.cpp
    mfc/AfxCmdTarget.cpp
    mfc/AfxCommonControls.cpp
    mfc/AfxCommonDialogs.cpp
    mfc/AfxPropertySheet.cpp
    mfc/AfxControlBar.cpp
    mfc/AfxControls.cpp
    mfc/AfxCore.cpp
    mfc/AfxDialog.cpp
    mfc/AfxDialogBar.cpp
    mfc/AfxDocManager.cpp
    mfc/AfxDocTemplate.cpp
    mfc/AfxDocument.cpp
    mfc/AfxFile.cpp
    mfc/AfxFrame.cpp
    mfc/AfxFrameBars.cpp
    mfc/AfxFrameCommands.cpp
    mfc/AfxGdi.cpp
    mfc/AfxGeometry.cpp
    mfc/AfxInternal.h
    mfc/AfxLayout.cpp
    mfc/AfxMenu.cpp
    mfc/AfxRecentFileList.cpp
    mfc/AfxStandardResources.cpp
    mfc/AfxState.cpp
    mfc/AfxStatusBar.cpp
    mfc/AfxString.cpp
    mfc/AfxToolBar.cpp
    mfc/AfxView.cpp
    mfc/AfxViewPrint.cpp
    mfc/AfxWnd.cpp
    mfc/AfxWndApi.cpp
    mfc/AfxWndDialogSupport.cpp
    mfc/AfxWndMessages.cpp
)

list(TRANSFORM WORLDBUILDER_MFC_SRC PREPEND ${WORLDBUILDER_IMGUI_DIR}/)

set(WORLDBUILDER_MACOS_DIR ${CMAKE_SOURCE_DIR}/Platform/MacOS/Source/WorldBuilder)
set(WORLDBUILDER_ICONS_DIR ${CMAKE_SOURCE_DIR}/Platform/MacOS/Launcher/assets)

set(WORLDBUILDER_MACOS_OBJCXX_SRC
    ${WORLDBUILDER_MACOS_DIR}/WBMacFramePump.mm
    ${WORLDBUILDER_MACOS_DIR}/WBMacIdentity.mm
    ${WORLDBUILDER_MACOS_DIR}/WBMacMain.mm
    ${WORLDBUILDER_MACOS_DIR}/WBMacMessageBox.mm
    ${WORLDBUILDER_MACOS_DIR}/WBMacPlatform.mm
    ${WORLDBUILDER_MACOS_DIR}/WBMacSplash.mm
)

set(WORLDBUILDER_MACOS_MANUAL_RETAIN_SRC
    ${WORLDBUILDER_MACOS_DIR}/WBMacIterationPool.mm
)

set(WORLDBUILDER_MACOS_SRC
    ${WORLDBUILDER_MACOS_OBJCXX_SRC}
    ${WORLDBUILDER_MACOS_MANUAL_RETAIN_SRC}
    ${WORLDBUILDER_MACOS_DIR}/WBMacCocoa.h
    ${WORLDBUILDER_MACOS_DIR}/WBMacFramePump.h
    ${WORLDBUILDER_MACOS_DIR}/WBMacIdentity.h
    ${WORLDBUILDER_MACOS_DIR}/WBMacIterationPool.h
    ${WORLDBUILDER_MACOS_DIR}/WBMacMessageBox.h
    ${WORLDBUILDER_MACOS_DIR}/WBMacModName.cpp
    ${WORLDBUILDER_MACOS_DIR}/WBMacModName.h
    ${WORLDBUILDER_MACOS_DIR}/WBMacPlatform.h
    ${WORLDBUILDER_MACOS_DIR}/WBMacSplash.h
    ${WORLDBUILDER_MACOS_DIR}/WBMacWorldBuilderPlatform.cpp
)

# The layer draws through DX8Wrapper, whose headers differ between the games, so each game
# compiles its own copy of the layer against its own engine, as the engine glue does.
# game_prefix names the game's targets: z for Zero Hour (z_gameengine, zi_always), g for Generals.
# The game libraries are linked ahead of the layer, as in the Windows target, so the game's own
# headers win over Core: Generals keeps its own Common/GameCommon.h.
# GAME_NAME and ICON tell the editors apart in the window title and the Dock; MODS lets an
# editor open a mod given with -mod, under the mod's name and the MOD_ICON badged with its medallion.
function(worldbuilder_add_macos_app game_prefix)
    cmake_parse_arguments(PARSE_ARGV 1 arg "MODS" "OUTPUT_NAME;BUNDLE_IDENTIFIER;GAME_NAME;ICON;MOD_ICON" "")
    set(output_name ${arg_OUTPUT_NAME})
    set(app_target ${game_prefix}_worldbuilder)
    set(user32_target ${game_prefix}_worldbuilder_user32)
    set(mfc_target ${game_prefix}_worldbuilder_mfc)
    set(probe_target ${game_prefix}_worldbuilder_probe)
    set(game_engine ${game_prefix}_gameengine)
    set(game_engine_device ${game_prefix}_gameenginedevice)
    set(game_always ${game_prefix}i_always)

    add_library(${user32_target} STATIC EXCLUDE_FROM_ALL ${WORLDBUILDER_USER32_SRC})

    target_include_directories(${user32_target} BEFORE PUBLIC
        ${WORLDBUILDER_IMGUI_DIR}/compat
    )

    target_include_directories(${user32_target} PUBLIC
        ${WORLDBUILDER_IMGUI_DIR}/user32
        ${WORLDBUILDER_IMGUI_DIR}/user32/controls
        ${WORLDBUILDER_IMGUI_DIR}/user32/presentation
        ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/ImGui
    )

    target_link_libraries(${user32_target} PUBLIC
        ${game_engine_device}
        ${game_always}
        imgui
        macos_platform
    )

    add_library(${mfc_target} STATIC EXCLUDE_FROM_ALL ${WORLDBUILDER_MFC_SRC})

    target_include_directories(${mfc_target} PRIVATE
        ${WORLDBUILDER_IMGUI_DIR}/mfc
    )

    target_compile_options(${mfc_target} PUBLIC -fno-delete-null-pointer-checks)

    target_link_libraries(${mfc_target} PUBLIC
        ${user32_target}
    )

    file(GLOB bundle_resources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/res/*)

    set(bundle_icons ${WORLDBUILDER_ICONS_DIR}/${arg_ICON}.png)
    if(arg_MODS)
        list(APPEND bundle_icons ${WORLDBUILDER_ICONS_DIR}/${arg_MOD_ICON}.png)
    endif()

    add_executable(${app_target} MACOSX_BUNDLE)

    target_sources(${app_target} PRIVATE
        ${WORLDBUILDER_SRC}
        ${WORLDBUILDER_MACOS_SRC}
        ${bundle_resources}
        ${bundle_icons}
        ${CMAKE_BINARY_DIR}/Platform/MacOS/default.metallib
    )

    set_source_files_properties(${CMAKE_BINARY_DIR}/Platform/MacOS/default.metallib ${bundle_icons} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_source_files_properties(${bundle_resources} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/${output_name} HEADER_FILE_ONLY ON)
    set_source_files_properties(${WORLDBUILDER_MACOS_OBJCXX_SRC} PROPERTIES
        SKIP_PRECOMPILE_HEADERS ON
        COMPILE_OPTIONS "-fobjc-arc"
    )
    set_source_files_properties(${WORLDBUILDER_MACOS_MANUAL_RETAIN_SRC} PROPERTIES
        SKIP_PRECOMPILE_HEADERS ON
        COMPILE_OPTIONS "-fno-objc-arc"
    )

    target_compile_definitions(${app_target} PRIVATE
        WORLDBUILDER_RESOURCE_DIRECTORY="${output_name}"
        WORLDBUILDER_GAME_NAME="${arg_GAME_NAME}"
        WORLDBUILDER_ICON_NAME="${arg_ICON}"
    )

    if(arg_MODS)
        target_compile_definitions(${app_target} PRIVATE
            WORLDBUILDER_MODS=1
            WORLDBUILDER_MOD_ICON_NAME="${arg_MOD_ICON}"
        )
    endif()

    target_include_directories(${app_target} BEFORE PRIVATE
        ${WORLDBUILDER_IMGUI_DIR}/compat
    )

    target_include_directories(${app_target} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/res
        ${WORLDBUILDER_MACOS_DIR}
    )

    target_precompile_headers(${app_target} PRIVATE ${WORLDBUILDER_PRECOMPILED_HEADERS})

    target_link_libraries(${app_target} PRIVATE
        ${game_engine}
        ${game_engine_device}
        ${game_always}
        ${mfc_target}
        imgui
        macos_platform
        "-framework UniformTypeIdentifiers"
    )

    macos_platform_add_engine_glue(${app_target})
    add_dependencies(${app_target} metal_shaders)

    set_target_properties(${app_target} PROPERTIES
        OUTPUT_NAME ${output_name}
        MACOSX_BUNDLE_GUI_IDENTIFIER ${arg_BUNDLE_IDENTIFIER}
        MACOSX_BUNDLE_BUNDLE_NAME ${output_name}
        MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1.0"
        MACOSX_BUNDLE_BUNDLE_VERSION "1"
    )

    add_library(${probe_target} OBJECT EXCLUDE_FROM_ALL)

    target_sources(${probe_target} PRIVATE ${WORLDBUILDER_SRC})

    target_include_directories(${probe_target} BEFORE PRIVATE
        ${WORLDBUILDER_IMGUI_DIR}/compat
    )

    target_include_directories(${probe_target} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/res
    )

    target_compile_options(${probe_target} PRIVATE -ferror-limit=0)
    target_precompile_headers(${probe_target} PRIVATE ${WORLDBUILDER_PRECOMPILED_HEADERS})

    target_link_libraries(${probe_target} PRIVATE
        ${game_engine}
        ${game_engine_device}
        ${game_always}
        ${mfc_target}
    )
endfunction()
