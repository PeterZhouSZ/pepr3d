#pragma once

#include "IconsMaterialDesign.h"
#include "peprimgui.h"

#include "commands/CommandManager.h"
#include "imgui_internal.h"  // must be included after peprimgui.h! beware of clang-format, keep the empty line above!
#include "tools/Tool.h"

namespace pepr3d {

class MainApplication;

/// A top part of the user interface that shows a File menu, Undo, Redo, and all tools
class Toolbar {
   public:
    explicit Toolbar(MainApplication& app) : mApplication(app) {}

    /// Returns the height.
    float getHeight() {
        return mHeight;
    }

    /// Draws the Toolbar using ImGui.
    void draw();

   private:
    MainApplication& mApplication;

    std::size_t mSelectedButtonIndex = 3;
    float mHeight = 50.0f;

    /// Draws a vertical separator.
    void drawSeparator();

    /// Properties of a clickable and selectable button in the Toolbar
    struct ButtonProperties {
        std::string label = ICON_MD_HELP;
        bool isEnabled = true;
        bool isDropDown = false;
        bool isToggled = false;
    };

    /// Draws a clickable and selectable button in the Toolbar according to the ButtonProperties
    template <typename Callback>
    void drawButton(ButtonProperties& props, Callback onPressed);

    /// Draws a File drop down menu.
    void drawFileDropDown();

    /// Draws the Undo and Redo buttons.
    void drawUndoRedo();

    using ToolsVector = std::vector<std::unique_ptr<Tool>>;

    /// Draws buttons of all Tool instances.
    void drawToolButtons();

    /// Draws a single button for a specified Tool (an iterator to the Tool).
    void drawToolButton(ToolsVector::iterator tool);
};

template <typename Callback>
void Toolbar::drawButton(ButtonProperties& props, Callback onPressed) {
    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const std::string label = props.label + (props.isDropDown ? ICON_MD_ARROW_DROP_DOWN : "");
    const ImVec2 label_size = ImGui::CalcTextSize(label.c_str());
    const ImGuiID id = ImGui::GetCurrentWindow()->GetID((props.label + "##toolbar").c_str());

    ImVec2 pos = ImGui::GetCursorPos();
    ImVec2 size =
        ImGui::CalcItemSize(glm::ivec2(mHeight * (props.isDropDown ? 1.5f : 1.0f), mHeight),
                            label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

    const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
    ImGui::ItemSize(bb, style.FramePadding.y);
    if(!ImGui::ItemAdd(bb, id)) {
        return;
    }

    bool isHovered, isHeld;
    if(ImGui::ButtonBehavior(bb, id, &isHovered, &isHeld) && props.isEnabled) {
        onPressed();
    }

    // Render
    ImU32 col = ImGui::GetColorU32((isHovered && isHeld) ? ImGuiCol_ButtonActive
                                                         : isHovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
    if(props.isToggled) {
        col = ImGui::ColorConvertFloat4ToU32(ci::ColorA::hex(0x017BDA));
        ImGui::PushStyleColor(ImGuiCol_Text, ci::ColorA::hex(0xFFFFFF));
    }
    if(!props.isEnabled) {
        col = ImGui::GetColorU32(ImGuiCol_Button);
        ImGui::PushStyleColor(ImGuiCol_Text, ci::ColorA::hex(0xD9D9D9));
    }
    ImGui::RenderNavHighlight(bb, id);
    ImGui::RenderFrame(bb.Min, bb.Max, col, true, style.FrameRounding);
    ImGui::RenderTextClipped(ImVec2(bb.Min.x + style.FramePadding.x, bb.Min.y + style.FramePadding.y),
                             ImVec2(bb.Max.x - style.FramePadding.x, bb.Max.y - style.FramePadding.y), label.c_str(),
                             nullptr, &label_size, style.ButtonTextAlign, &bb);
    if(props.isToggled) {
        ImGui::PopStyleColor();
    }
    if(!props.isEnabled) {
        ImGui::PopStyleColor();
    }

    return;
}

}  // namespace pepr3d
