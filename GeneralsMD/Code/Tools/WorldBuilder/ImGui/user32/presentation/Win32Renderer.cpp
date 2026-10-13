#include "Win32Renderer.h"

#include "Win32Gdi.h"
#include "Win32Platform.h"
#include "Win32SystemColors.h"

#include <string>

namespace
{

const char kSystemFontFace[] = "Tahoma";
const float kSystemFontPixelHeight = 13.0f;

ImFont* loadSystemFont(ImGuiIO& io)
{
	Win32Platform* platform = Win32GetPlatform();
	const std::string path = platform != nullptr ? platform->fontFilePath(kSystemFontFace) : std::string();
	ImFont* font = path.empty() ? nullptr : io.Fonts->AddFontFromFileTTF(path.c_str(), kSystemFontPixelHeight);
	return font != nullptr ? font : io.Fonts->AddFontDefault();
}

void configureSystemFont(ImGuiIO& io)
{
	ImFont* font = io.Fonts->Fonts.empty() ? loadSystemFont(io) : io.Fonts->Fonts[0];
	ImGuiStyle& style = ImGui::GetStyle();
	if (style.FontSizeBase <= 0.0f)
	{
		style.FontSizeBase = font->LegacySize;
	}
}

}

Win32Renderer& Win32Renderer::instance()
{
	static Win32Renderer renderer;
	return renderer;
}

void Win32Renderer::configure(ImGuiIO& io) const
{
	io.ConfigFlags &= ~(ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad | ImGuiConfigFlags_ViewportsEnable);
	io.ConfigWindowsMoveFromTitleBarOnly = true;
	io.IniFilename = nullptr;
	Win32SystemColors::applyToStyle(ImGui::GetStyle());
	configureSystemFont(io);
}

void Win32Renderer::buildFrame()
{
	Win32Gdi::releaseRetiredTextures();
	m_windows.present();
	m_menus.present();
	m_mouse.route(m_windows);
	m_keyboard.route();
}
