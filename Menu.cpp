#include "Menu.h"

#undef max
#undef min

#include <iostream>

ImVec4 Menu::ARGBToImVec4(const uint32_t& color)
{
	const float
		a = (color >> 24 & 0xFF) / 255.0f,
		r = (color >> 16 & 0xFF) / 255.0f,
		g = (color >> 8 & 0xFF) / 255.0f,
		b = (color & 0xFF) / 255.0f;

	return ImVec4(r, g, b, a);
}

uint32_t Menu::ImVec4ToARGB(const ImVec4& color)
{
	const uint32_t
		a = static_cast<uint32_t>(color.w * 255.0f) & 0xFF,
		r = static_cast<uint32_t>(color.x * 255.0f) & 0xFF,
		g = static_cast<uint32_t>(color.y * 255.0f) & 0xFF,
		b = static_cast<uint32_t>(color.z * 255.0f) & 0xFF;

	return (a << 24) | (r << 16) | (g << 8) | b;
}

void Menu::Render()
{
	const auto& mSelectedLine = Chat::getInstance().mSelectedLine;

	const auto& io = ImGui::GetIO();
	
	if (IsEditLineActive()) Chat::setSampCursorMode(2);
	const int cursorMode = Chat::getSampCursorMode();
	const bool cursorEnabled = cursorMode >= 2 && cursorMode <= 3;

	if (Chat::isGTAMenuActive() || !cursorEnabled) return;

	if (mSelectedLine > -1)
	{
		if (ImGui::Begin("##SelectedLine", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground))
		{
			const auto& position = Chat::getInstance().getChatEntryManager().get(mSelectedLine);
			const auto& rect = position.rect;

			// TODO: Offsets by SAMP char height
			ImGui::SetWindowPos(ImVec2((float)rect.x1 - io.DisplaySize.x * (10.0f / 1920), (float)rect.y1 - io.DisplaySize.y * (5.0f / 1080)));
			ImGui::SetCursorPosX(0.0f);
			ImGui::Text(">");

			ImGui::End();
		}
	}

	auto& chat = Chat::getInstance();
	if (ImGui::Begin("##PopupWrapper", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar))
	{
		if (this->openPopup && !ImGui::IsPopupOpen("TextContextMenu"))
		{
			ImGui::OpenPopup("TextContextMenu");
			this->openPopup = false;
			this->popupActive = true;
		} else if (this->popupActive && (!ImGui::IsPopupOpen("TextContextMenu") || mSelectedLine < 0) )
		{
			ClosePopup();
		}

		if (ImGui::BeginPopupContextItem("TextContextMenu"))
		{
			if (chat.mSelectedLine != -1)
			{
				ImGui::PushItemWidth(350.0f);
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12.0f, 12.0f));
				ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.12f, 0.12f, 0.12f, 0.9f));
				ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
				if (ImGui::MenuItem("Copy"))
				{
					const bool with_colors = (GetKeyState(VK_SHIFT) & 0x8000) != 0; // Hold shift
					std::string text(chat.pChat->m_entry[chat.mSelectedLine].m_szText);
					if (!with_colors)
					{
						for (size_t pos = 0; pos + 7 < text.size();)
						{
							if (text[pos] == '{' && text[pos + 7] == '}')
							{
								bool is_hex = true;
								for (size_t i = 1; i < 7; ++i)
								{
									if (!std::isxdigit(static_cast<unsigned char>(text[pos + i])))
									{
										is_hex = false;
										break;
									}
								}
								if (is_hex)
								{
									text.erase(pos, 8);
									continue;
								}
							}
							++pos;
						}
					}
					ImGui::SetClipboardText(Chat::convertToUTF8(text).c_str());
				}
				if (ImGui::MenuItem(u8"Delete"))
				{
					Chat::sampDeleteChatLine(chat.mSelectedLine);
				}
				if (ImGui::MenuItem("Edit"))
				{
					const auto& pChat = Chat::getInstance().pChat;

					editLineBuffer[0] = '\0';
					strcat(editLineBuffer, chat.convertToUTF8(pChat->m_entry[chat.mSelectedLine].m_szText).c_str());
					editLineColor = ARGBToImVec4(pChat->m_entry[mSelectedLine].m_textColor);

					editLineActive = true;
				}
				if (ImGui::MenuItem("Clear"))
				{
					Chat::sampDeleteChatLineAll();
				}
				ImGui::PopItemWidth();
				ImGui::PopStyleVar();
				ImGui::PopStyleColor(2);
			}

			ImGui::EndPopup();
			ImGui::End();
		}
	}

	if (IsEditLineActive() && chat.mSelectedLine > -1)
	{
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.12f, 0.12f, 0.9f));
		if (ImGui::Begin("##EditLine", &editLineActive, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground))
		{
			ImGui::SetWindowPos(ImVec2((io.DisplaySize.x - ImGui::GetWindowWidth()) * 0.5f,
				(io.DisplaySize.y - ImGui::GetWindowHeight()) * 0.5f));

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12.0f, 12.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 12.0f));
			ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
			ImGui::PushItemWidth(io.DisplaySize.x * (750.0f / 1920));

			if (ImGui::ColorButton("ColorButton", editLineColor, ImGuiColorEditFlags_NoTooltip))
			{
				ImGui::OpenPopup("ColorPickerPopup");
			}

			if (ImGui::BeginPopup("ColorPickerPopup"))
			{
				this->colorPopupActive = true;
				if (ImGui::ColorPicker3("##picker", (float*)&editLineColor, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview))
				{
					auto& pChat = chat.pChat;
					pChat->m_entry[mSelectedLine].m_textColor = ImVec4ToARGB(editLineColor);
					chat.chatUpdate();
				}
				ImGui::EndPopup();
			} else this->colorPopupActive = false;

			ImGui::SameLine(0.0f, 5.0f);

			if (ImGui::InputText("##ChatLine", editLineBuffer, IM_ARRAYSIZE(editLineBuffer)))
			{
				if (chat.mSelectedLine > -1)
				{
					const auto& pChat = chat.pChat;

					pChat->m_entry[chat.mSelectedLine].m_szText[0] = '\0';
					const std::string text(Chat::convertFromUTF8(editLineBuffer));
					strcat(pChat->m_entry[chat.mSelectedLine].m_szText, text.c_str());
					chat.chatUpdate();
				}
			}

			// Input outline
			ImVec2 inputPos = ImGui::GetItemRectMin();
			ImVec2 inputSize = ImGui::GetItemRectSize();
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			drawList->AddRect(inputPos, ImVec2(inputPos.x + inputSize.x, inputPos.y + inputSize.y), IM_COL32(255, 255, 255, 255));

			ImGui::PopItemWidth();
			ImGui::PopStyleVar(2);
			ImGui::PopStyleColor();
			ImGui::End();
		}
		ImGui::PopStyleColor();
	}
}

void Menu::RebuildFonts()
{
	std::string fontPath(256, '\0');

	if (SHGetSpecialFolderPathA(GetActiveWindow(), fontPath.data(), CSIDL_FONTS, false))
	{
		ImGui_ImplDX9_InvalidateDeviceObjects();

		fontPath.resize(fontPath.find('\0'));
		std::string fontName{ Chat::getSampFontName() };
		if (fontName == "Arial") fontName += "Bd";
		fontPath += "\\" + fontName + ".ttf";

		auto& io = ImGui::GetIO();
		ImVector<ImWchar> ranges;
		ImFontGlyphRangesBuilder builder;
		io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
		builder.AddRanges(io.Fonts->GetGlyphRangesCyrillic());
		builder.AddText(u8"‚„…†‡ˆ‰‹‘’“”•–—™›¹");
		builder.BuildRanges(&ranges);

		ImFontConfig cfg;
		cfg.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_LightHinting | ImGuiFreeTypeBuilderFlags_ForceAutoHint;
		io.Fonts->Clear();
		io.Fonts->AddFontFromFileTTF(fontPath.c_str(), Chat::getSampFontSize() - 2, &cfg, ranges.Data);
		io.Fonts->Build();

		ImGui::GetStyle().ItemSpacing.y = 2;

		ImGui_ImplDX9_CreateDeviceObjects();
	}
}

void Menu::ShowPopup()
{
	this->openPopup = true;
}

void Menu::ClosePopup()
{
	ImGui::CloseCurrentPopup();
	this->popupActive = false;
}

bool Menu::IsPopupActive() const
{
	return popupActive;
}

bool Menu::IsEditLineActive() const
{
	return editLineActive;
}

bool Menu::IsColorPopupActive() const
{
	return this->colorPopupActive;
}

void Menu::CloseEditLine()
{
	this->editLineActive = false;
	Chat::setSampCursorMode(0);
}