include(${CMAKE_SOURCE_DIR}/cmake/imgui.cmake)

set(WORLDBUILDER_PRECOMPILED_HEADERS
    [["Utility/CppMacros.h"]]
    [["WWLib/always.h"]]
    [["Common/STLTypedefs.h"]]
    [["StdAfx.h"]]
    [["WWLib/WWCommon.h"]]
)

set(WORLDBUILDER_USER32_SRC
    ImGui/compat/CommCtrl.h
    ImGui/compat/CommDlg.h
    ImGui/compat/RichEdit.h
    ImGui/compat/Win32Api.h
    ImGui/compat/WinBaseExtras.h
    ImGui/compat/WinGdi.h
    ImGui/compat/WinUser.h
    ImGui/compat/WinUserMessages.h
    ImGui/compat/WinUserStyles.h
    ImGui/user32/Win32Class.cpp
    ImGui/user32/Win32Class.h
    ImGui/user32/Win32ColorDialog.cpp
    ImGui/user32/Win32CommonDialogs.cpp
    ImGui/user32/Win32CommonDialogs.h
    ImGui/user32/Win32Control.cpp
    ImGui/user32/Win32Control.h
    ImGui/user32/Win32DefWindowProc.cpp
    ImGui/user32/Win32Dialog.cpp
    ImGui/user32/Win32Dialog.h
    ImGui/user32/Win32DialogParser.cpp
    ImGui/user32/Win32FileDialogs.cpp
    ImGui/user32/Win32Focus.cpp
    ImGui/user32/Win32Gdi.h
    ImGui/user32/Win32GdiDraw.cpp
    ImGui/user32/Win32GdiObjects.cpp
    ImGui/user32/Win32GdiRegions.cpp
    ImGui/user32/Win32Geometry.cpp
    ImGui/user32/Win32Images.cpp
    ImGui/user32/Win32Input.cpp
    ImGui/user32/Win32Input.h
    ImGui/user32/Win32InputQueue.cpp
    ImGui/user32/Win32InputQueue.h
    ImGui/user32/Win32Menu.cpp
    ImGui/user32/Win32Menu.h
    ImGui/user32/Win32Messages.cpp
    ImGui/user32/Win32Messages.h
    ImGui/user32/Win32Metrics.cpp
    ImGui/user32/Win32Metrics.h
    ImGui/user32/Win32Paths.cpp
    ImGui/user32/Win32Paths.h
    ImGui/user32/Win32Platform.h
    ImGui/user32/Win32Profile.cpp
    ImGui/user32/Win32Profile.h
    ImGui/user32/Win32PropertySheet.cpp
    ImGui/user32/Win32PropertySheet.h
    ImGui/user32/Win32PropertySheetPages.cpp
    ImGui/user32/Win32ResourceParser.cpp
    ImGui/user32/Win32ResourceParser.h
    ImGui/user32/Win32Resources.cpp
    ImGui/user32/Win32Resources.h
    ImGui/user32/Win32ResourceScript.cpp
    ImGui/user32/Win32ResourceScript.h
    ImGui/user32/Win32ScrollBars.cpp
    ImGui/user32/Win32System.cpp
    ImGui/user32/Win32SystemColors.cpp
    ImGui/user32/Win32SystemColors.h
    ImGui/user32/Win32Window.cpp
    ImGui/user32/Win32Window.h
    ImGui/user32/Win32WindowApi.cpp
    ImGui/user32/controls/Win32Button.cpp
    ImGui/user32/controls/Win32Button.h
    ImGui/user32/controls/Win32ButtonDraw.cpp
    ImGui/user32/controls/Win32ColorPicker.cpp
    ImGui/user32/controls/Win32ColorPicker.h
    ImGui/user32/controls/Win32ComboBox.cpp
    ImGui/user32/controls/Win32ComboBox.h
    ImGui/user32/controls/Win32ComboBoxDraw.cpp
    ImGui/user32/controls/Win32ControlDrawing.cpp
    ImGui/user32/controls/Win32ControlDrawing.h
    ImGui/user32/controls/Win32Controls.cpp
    ImGui/user32/controls/Win32Controls.h
    ImGui/user32/controls/Win32Edit.cpp
    ImGui/user32/controls/Win32Edit.h
    ImGui/user32/controls/Win32EditMessages.cpp
    ImGui/user32/controls/Win32ImageList.cpp
    ImGui/user32/controls/Win32ImageList.h
    ImGui/user32/controls/Win32ItemList.cpp
    ImGui/user32/controls/Win32ItemList.h
    ImGui/user32/controls/Win32ListBox.cpp
    ImGui/user32/controls/Win32ListBox.h
    ImGui/user32/controls/Win32ListView.h
    ImGui/user32/controls/Win32ListViewItems.cpp
    ImGui/user32/controls/Win32ListViewMessages.cpp
    ImGui/user32/controls/Win32RichEdit.cpp
    ImGui/user32/controls/Win32RichEdit.h
    ImGui/user32/controls/Win32RichEditInput.cpp
    ImGui/user32/controls/Win32Static.cpp
    ImGui/user32/controls/Win32Static.h
    ImGui/user32/controls/Win32StatusBar.cpp
    ImGui/user32/controls/Win32StatusBar.h
    ImGui/user32/controls/Win32TabControl.cpp
    ImGui/user32/controls/Win32TabControl.h
    ImGui/user32/controls/Win32Toolbar.cpp
    ImGui/user32/controls/Win32Toolbar.h
    ImGui/user32/controls/Win32ToolbarDraw.cpp
    ImGui/user32/controls/Win32Trackbar.cpp
    ImGui/user32/controls/Win32Trackbar.h
    ImGui/user32/controls/Win32TreeView.h
    ImGui/user32/controls/Win32TreeViewDraw.cpp
    ImGui/user32/controls/Win32TreeViewInput.cpp
    ImGui/user32/controls/Win32TreeViewItems.cpp
    ImGui/user32/controls/Win32TreeViewMessages.cpp
    ImGui/user32/presentation/Win32KeyboardRouter.cpp
    ImGui/user32/presentation/Win32KeyboardRouter.h
    ImGui/user32/presentation/Win32MenuPresenter.cpp
    ImGui/user32/presentation/Win32MenuPresenter.h
    ImGui/user32/presentation/Win32MouseRouter.cpp
    ImGui/user32/presentation/Win32MouseRouter.h
    ImGui/user32/presentation/Win32Renderer.cpp
    ImGui/user32/presentation/Win32Renderer.h
    ImGui/user32/presentation/Win32WindowPresenter.cpp
    ImGui/user32/presentation/Win32WindowPresenter.h
    ImGui/d3dx/D3DXFont.cpp
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/ImGui/imgui_impl_dx8wrapper.cpp
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/ImGui/imgui_impl_dx8wrapper.h
)

add_library(z_worldbuilder_user32 STATIC EXCLUDE_FROM_ALL ${WORLDBUILDER_USER32_SRC})

target_include_directories(z_worldbuilder_user32 BEFORE PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/ImGui/compat
)

target_include_directories(z_worldbuilder_user32 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/ImGui/user32
    ${CMAKE_CURRENT_SOURCE_DIR}/ImGui/user32/controls
    ${CMAKE_CURRENT_SOURCE_DIR}/ImGui/user32/presentation
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/ImGui
)

target_link_libraries(z_worldbuilder_user32 PUBLIC
    z_gameenginedevice
    zi_always
    imgui
    macos_platform
)

set(WORLDBUILDER_MFC_SRC
    ImGui/compat/afx.h
    ImGui/compat/afxcmn.h
    ImGui/compat/afxdlgs.h
    ImGui/compat/afxdtctl.h
    ImGui/compat/afxext.h
    ImGui/compat/afxres.h
    ImGui/compat/afxwin.h
    ImGui/compat/d3dx8.h
    ImGui/compat/PrSht.h
    ImGui/compat/mfc/AfxApp.h
    ImGui/compat/mfc/AfxBars.h
    ImGui/compat/mfc/AfxCmdTarget.h
    ImGui/compat/mfc/AfxCommonControls.h
    ImGui/compat/mfc/AfxControls.h
    ImGui/compat/mfc/AfxCore.h
    ImGui/compat/mfc/AfxDialog.h
    ImGui/compat/mfc/AfxDialogs.h
    ImGui/compat/mfc/AfxDocument.h
    ImGui/compat/mfc/AfxFile.h
    ImGui/compat/mfc/AfxFrame.h
    ImGui/compat/mfc/AfxGdi.h
    ImGui/compat/mfc/AfxGeometry.h
    ImGui/compat/mfc/AfxMenu.h
    ImGui/compat/mfc/AfxMessageMacros.h
    ImGui/compat/mfc/AfxStandardResources.h
    ImGui/compat/mfc/AfxString.h
    ImGui/compat/mfc/AfxView.h
    ImGui/compat/mfc/AfxWnd.h
    ImGui/mfc/AfxApp.cpp
    ImGui/mfc/AfxCmdTarget.cpp
    ImGui/mfc/AfxCommonControls.cpp
    ImGui/mfc/AfxCommonDialogs.cpp
    ImGui/mfc/AfxPropertySheet.cpp
    ImGui/mfc/AfxControlBar.cpp
    ImGui/mfc/AfxControls.cpp
    ImGui/mfc/AfxCore.cpp
    ImGui/mfc/AfxDialog.cpp
    ImGui/mfc/AfxDialogBar.cpp
    ImGui/mfc/AfxDocManager.cpp
    ImGui/mfc/AfxDocTemplate.cpp
    ImGui/mfc/AfxDocument.cpp
    ImGui/mfc/AfxFile.cpp
    ImGui/mfc/AfxFrame.cpp
    ImGui/mfc/AfxFrameBars.cpp
    ImGui/mfc/AfxFrameCommands.cpp
    ImGui/mfc/AfxGdi.cpp
    ImGui/mfc/AfxGeometry.cpp
    ImGui/mfc/AfxInternal.h
    ImGui/mfc/AfxLayout.cpp
    ImGui/mfc/AfxMenu.cpp
    ImGui/mfc/AfxRecentFileList.cpp
    ImGui/mfc/AfxStandardResources.cpp
    ImGui/mfc/AfxState.cpp
    ImGui/mfc/AfxStatusBar.cpp
    ImGui/mfc/AfxString.cpp
    ImGui/mfc/AfxToolBar.cpp
    ImGui/mfc/AfxView.cpp
    ImGui/mfc/AfxViewPrint.cpp
    ImGui/mfc/AfxWnd.cpp
    ImGui/mfc/AfxWndApi.cpp
    ImGui/mfc/AfxWndDialogSupport.cpp
    ImGui/mfc/AfxWndMessages.cpp
)

add_library(z_worldbuilder_mfc STATIC EXCLUDE_FROM_ALL ${WORLDBUILDER_MFC_SRC})

target_include_directories(z_worldbuilder_mfc PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/ImGui/mfc
)

target_compile_options(z_worldbuilder_mfc PUBLIC -fno-delete-null-pointer-checks)

target_link_libraries(z_worldbuilder_mfc PUBLIC
    z_worldbuilder_user32
)

set(WORLDBUILDER_MACOS_OBJCXX_SRC
    ${CMAKE_SOURCE_DIR}/Platform/MacOS/Source/WorldBuilder/WBMacFramePump.mm
    ${CMAKE_SOURCE_DIR}/Platform/MacOS/Source/WorldBuilder/WBMacMain.mm
    ${CMAKE_SOURCE_DIR}/Platform/MacOS/Source/WorldBuilder/WBMacMessageBox.mm
    ${CMAKE_SOURCE_DIR}/Platform/MacOS/Source/WorldBuilder/WBMacPlatform.mm
)

set(WORLDBUILDER_MACOS_SRC
    ${WORLDBUILDER_MACOS_OBJCXX_SRC}
    ${CMAKE_SOURCE_DIR}/Platform/MacOS/Source/WorldBuilder/WBMacCocoa.h
    ${CMAKE_SOURCE_DIR}/Platform/MacOS/Source/WorldBuilder/WBMacFramePump.h
    ${CMAKE_SOURCE_DIR}/Platform/MacOS/Source/WorldBuilder/WBMacMessageBox.h
    ${CMAKE_SOURCE_DIR}/Platform/MacOS/Source/WorldBuilder/WBMacPlatform.h
    ${CMAKE_SOURCE_DIR}/Platform/MacOS/Source/WorldBuilder/WBMacWorldBuilderPlatform.cpp
)

file(GLOB WORLDBUILDER_BUNDLE_RESOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/res/*)

add_executable(z_worldbuilder MACOSX_BUNDLE)

target_sources(z_worldbuilder PRIVATE
    ${WORLDBUILDER_SRC}
    ${WORLDBUILDER_MACOS_SRC}
    ${WORLDBUILDER_BUNDLE_RESOURCES}
    ${CMAKE_BINARY_DIR}/Platform/MacOS/default.metallib
)

set_source_files_properties(${CMAKE_BINARY_DIR}/Platform/MacOS/default.metallib PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
set_source_files_properties(${WORLDBUILDER_BUNDLE_RESOURCES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/WorldBuilder HEADER_FILE_ONLY ON)
set_source_files_properties(${WORLDBUILDER_MACOS_OBJCXX_SRC} PROPERTIES
    SKIP_PRECOMPILE_HEADERS ON
    COMPILE_OPTIONS "-fobjc-arc"
)

target_include_directories(z_worldbuilder BEFORE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/ImGui/compat
)

target_include_directories(z_worldbuilder PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    include
    res
    ${CMAKE_SOURCE_DIR}/Platform/MacOS/Source/WorldBuilder
)

target_precompile_headers(z_worldbuilder PRIVATE ${WORLDBUILDER_PRECOMPILED_HEADERS})

target_link_libraries(z_worldbuilder PRIVATE
    z_worldbuilder_mfc
    z_gameengine
    z_gameenginedevice
    zi_always
    imgui
    macos_platform
    "-framework UniformTypeIdentifiers"
)

macos_platform_add_engine_glue(z_worldbuilder)
add_dependencies(z_worldbuilder metal_shaders)

set_target_properties(z_worldbuilder PROPERTIES
    OUTPUT_NAME "WorldBuilderZH"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.playgenerals.worldbuilder"
    MACOSX_BUNDLE_BUNDLE_NAME "WorldBuilderZH"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1.0"
    MACOSX_BUNDLE_BUNDLE_VERSION "1"
)

add_library(z_worldbuilder_probe OBJECT EXCLUDE_FROM_ALL)

target_sources(z_worldbuilder_probe PRIVATE ${WORLDBUILDER_SRC})

target_include_directories(z_worldbuilder_probe BEFORE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/ImGui/compat
)

target_include_directories(z_worldbuilder_probe PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    include
    res
)

target_compile_options(z_worldbuilder_probe PRIVATE -ferror-limit=0)
target_precompile_headers(z_worldbuilder_probe PRIVATE ${WORLDBUILDER_PRECOMPILED_HEADERS})

target_link_libraries(z_worldbuilder_probe PRIVATE
    z_worldbuilder_mfc
    z_gameengine
    z_gameenginedevice
    zi_always
)
