#include "glory/engine/debug_panel.hpp"

#include "glory/engine/camera.hpp"
#include "glory/engine/third_person_camera.hpp"
#include "glory/engine/model_renderer.hpp"
#include "glory/engine/audio_system.hpp"

#include <SDL3/SDL_video.h>
#include <SDL3/SDL_opengl.h>
#include <imgui.h>
#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_sdl3.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cfloat>
#include <stdexcept>
#include <string_view>

namespace glory::engine {
namespace {

bool inputString(const char* label, std::string& value, ImGuiInputTextFlags flags = ImGuiInputTextFlags_None) {
    std::array<char, 256> buffer{};
    const std::size_t copySize = std::min(value.size(), buffer.size() - 1);
    std::copy_n(value.data(), copySize, buffer.data());
    if (!ImGui::InputText(label, buffer.data(), buffer.size(), flags)) return false;
    value = buffer.data();
    return true;
}

std::string diagnosticEscape(std::string_view value) {
    std::string result;
    for (const char character : value) {
        if (character == '"' || character == '\\') result.push_back('\\');
        if (static_cast<unsigned char>(character) >= 0x20) result.push_back(character);
    }
    return result;
}

} // namespace

DebugPanel::DebugPanel(SDL_Window* window, void* glContext) {
    try {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        contextCreated_ = true;
        ImGui::GetIO().IniFilename = nullptr;
        ImGui::StyleColorsDark();
        if (!ImGui_ImplSDL3_InitForOpenGL(window, glContext)) {
            throw std::runtime_error("Dear ImGui SDL3 backend initialization failed");
        }
        sdlBackendInitialized_ = true;
        if (!ImGui_ImplOpenGL3_Init("#version 450 core")) {
            throw std::runtime_error("Dear ImGui OpenGL3 backend initialization failed");
        }
        openGlBackendInitialized_ = true;
    } catch (...) {
        if (openGlBackendInitialized_) ImGui_ImplOpenGL3_Shutdown();
        if (sdlBackendInitialized_) ImGui_ImplSDL3_Shutdown();
        if (contextCreated_) ImGui::DestroyContext();
        openGlBackendInitialized_ = false;
        sdlBackendInitialized_ = false;
        contextCreated_ = false;
        throw;
    }
}

DebugPanel::~DebugPanel() {
    if (openGlBackendInitialized_) ImGui_ImplOpenGL3_Shutdown();
    if (sdlBackendInitialized_) ImGui_ImplSDL3_Shutdown();
    if (contextCreated_) ImGui::DestroyContext();
}

void DebugPanel::processEvent(const SDL_Event& event) noexcept {
    ImGui_ImplSDL3_ProcessEvent(&event);
    if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
        if (event.key.key == SDLK_F3) {
            devToolsVisible_ = !devToolsVisible_;
        } else if (event.key.key == SDLK_F8) {
            feedbackOpenRequested_ = !feedbackOpenRequested_;
        }
    }
}

bool DebugPanel::wantsCaptureMouse() const noexcept {
    return (devToolsVisible_ || feedbackOpenRequested_) && captureMouse_;
}

bool DebugPanel::wantsCaptureKeyboard() const noexcept {
    return (devToolsVisible_ || feedbackOpenRequested_) && captureKeyboard_;
}

void DebugPanel::logAnimationEvent(const AnimationEvent& event) {
    animationEventLog_.push_back(event.name + " @ " + std::to_string(event.time) + " s");
    if (animationEventLog_.size() > 8) animationEventLog_.erase(animationEventLog_.begin());
}

void DebugPanel::renderGameHud(const PlayerFrameState& player, std::string& requestedSkillId, float deltaSeconds) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    if (!viewport) return;
    const ImVec2 vpPos = viewport->Pos;
    const ImVec2 vpSize = viewport->Size;

    auto worldToScreen = [&](const glm::vec3& worldPos, ImVec2& outPos) -> bool {
        const glm::vec4 clip = viewProjection_ * glm::vec4(worldPos, 1.0F);
        if (clip.w <= 0.001F) return false;
        const float invW = 1.0F / clip.w;
        const float ndcX = clip.x * invW;
        const float ndcY = clip.y * invW;
        const float ndcZ = clip.z * invW;
        if (ndcZ < -1.0F || ndcZ > 1.0F) return false;
        outPos.x = (ndcX * 0.5F + 0.5F) * vpSize.x + vpPos.x;
        outPos.y = (-ndcY * 0.5F + 0.5F) * vpSize.y + vpPos.y;
        return true;
    };

    hudAnimTime_ += deltaSeconds;

    // 0. Ground Overlays (Arena Boundary & Boss Danger Telegraph)
    ImDrawList* bgDraw = ImGui::GetBackgroundDrawList();
    if (bgDraw) {
        if (arenaBoundaryDebugEnabled_) {
            // Optional arena boundary guide; it is an ImGui overlay, not scene geometry.
            constexpr int arenaSegments = 32;
            const float arenaRadius = 35.0F;
            ImVec2 arenaPoints[arenaSegments];
            int validArena = 0;
            for (int i = 0; i < arenaSegments; ++i) {
                const float angle = (static_cast<float>(i) / static_cast<float>(arenaSegments)) * 6.2831853F;
                const glm::vec3 worldPt = glm::vec3(std::cos(angle) * arenaRadius, 0.02F, std::sin(angle) * arenaRadius);
                ImVec2 screenPt;
                if (worldToScreen(worldPt, screenPt)) arenaPoints[validArena++] = screenPt;
            }
            if (validArena >= 8)
                bgDraw->AddPolyline(arenaPoints, validArena, IM_COL32(70, 140, 220, 100), ImDrawFlags_Closed, 2.0F);
        }

        // Boss Danger Telegraph Area (Radius 4.8m)
        if (!player.bossDefeated && (!player.bossAttackName.empty() || player.bossBehaviorNode == "Charge" ||
            player.bossBehaviorNode == "GroundSlam" || player.bossBehaviorNode == "MeleeSlash")) {
            constexpr int dangerSegments = 24;
            const float dangerRadius = 4.8F;
            ImVec2 polyPoints[dangerSegments];
            int validDanger = 0;
            for (int i = 0; i < dangerSegments; ++i) {
                const float angle = (static_cast<float>(i) / static_cast<float>(dangerSegments)) * 6.2831853F;
                const glm::vec3 worldPt = player.bossPosition + glm::vec3(std::cos(angle) * dangerRadius, 0.03F, std::sin(angle) * dangerRadius);
                ImVec2 screenPt;
                if (worldToScreen(worldPt, screenPt)) {
                    polyPoints[validDanger++] = screenPt;
                }
            }
            if (validDanger >= 3) {
                const float dangerPulse = 0.32F + 0.16F * std::sin(hudAnimTime_ * 10.0F);
                bgDraw->AddConvexPolyFilled(polyPoints, validDanger, IM_COL32(255, 30, 30, static_cast<int>(dangerPulse * 255.0F)));
                bgDraw->AddPolyline(polyPoints, validDanger, IM_COL32(255, 75, 75, 230), ImDrawFlags_Closed, 2.5F);
                ImVec2 centerScreen;
                if (worldToScreen(player.bossPosition + glm::vec3(0.0F, 0.05F, 0.0F), centerScreen)) {
                    const char* warnText = "! DANGER ZONE - [SHIFT] DODGE !";
                    const float warnW = ImGui::CalcTextSize(warnText).x;
                    bgDraw->AddText(ImVec2(centerScreen.x - warnW * 0.5F, centerScreen.y - 8.0F),
                        IM_COL32(255, 245, 245, 240), warnText);
                }
            }
        }
    }

    // 1. Crosshair & Combat Reticle (Center Screen)
    if (bgDraw) {
        const ImVec2 center(vpPos.x + vpSize.x * 0.5F, vpPos.y + vpSize.y * 0.5F);
        bgDraw->AddCircleFilled(center, 2.0F, IM_COL32(255, 255, 255, 200));
        constexpr float tickInner = 5.0F;
        constexpr float tickOuter = 12.0F;
        bgDraw->AddLine(ImVec2(center.x - tickOuter, center.y), ImVec2(center.x - tickInner, center.y), IM_COL32(255, 255, 255, 160), 1.5F);
        bgDraw->AddLine(ImVec2(center.x + tickInner, center.y), ImVec2(center.x + tickOuter, center.y), IM_COL32(255, 255, 255, 160), 1.5F);
        bgDraw->AddLine(ImVec2(center.x, center.y - tickOuter), ImVec2(center.x, center.y - tickInner), IM_COL32(255, 255, 255, 160), 1.5F);
        bgDraw->AddLine(ImVec2(center.x, center.y + tickInner), ImVec2(center.x, center.y + tickOuter), IM_COL32(255, 255, 255, 160), 1.5F);

        if (player.hitStopActive) {
            bgDraw->AddText(ImVec2(center.x - 14.0F, center.y + 16.0F), IM_COL32(255, 215, 0, 240), "HIT!");
        }
    }

    // 2. Player Status Frame (Top-Left)
    const ImGuiWindowFlags hudFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

    ImGui::SetNextWindowPos(ImVec2(vpPos.x + 20.0F, vpPos.y + 20.0F), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(300.0F, 96.0F), ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08F, 0.10F, 0.14F, 0.82F));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.24F, 0.40F, 0.60F, 0.70F));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.5F);
    if (ImGui::Begin("##PlayerHudFrame", nullptr, hudFlags)) {
        const std::string charClass = player.className.empty() ? "Warrior" : player.className;
        ImGui::TextColored(ImVec4(0.95F, 0.95F, 0.95F, 1.0F), "Hero  |  Lv. 1  %s", charClass.c_str());

        const float hpFraction = GameHudCalculations::calculateHpFraction(player.playerHp, player.playerMaxHp);
        const ImVec4 hpColor = hpFraction > 0.5F ? ImVec4(0.22F, 0.80F, 0.35F, 1.0F)
            : (hpFraction > 0.25F ? ImVec4(0.95F, 0.65F, 0.15F, 1.0F) : ImVec4(0.90F, 0.20F, 0.20F, 1.0F));

        char hpOverlay[64];
        std::snprintf(hpOverlay, sizeof(hpOverlay), "HP %.0f / %.0f (%.0f%%)", player.playerHp, player.playerMaxHp, hpFraction * 100.0F);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, hpColor);
        ImGui::ProgressBar(hpFraction, ImVec2(-1.0F, 20.0F), hpOverlay);
        ImGui::PopStyleColor();

        // Combat status badges
        if (player.playerStunned) {
            ImGui::TextColored(ImVec4(1.0F, 0.25F, 0.25F, 1.0F), "[STUNNED]");
        } else if (player.isInvulnerable) {
            ImGui::TextColored(ImVec4(1.0F, 0.85F, 0.20F, 1.0F), "[I-FRAME]");
        } else if (player.isDodging) {
            ImGui::TextColored(ImVec4(0.35F, 0.85F, 1.0F, 1.0F), "[DODGING]");
        } else if (player.dodgeCooldownRemaining > 0.0F) {
            ImGui::TextColored(ImVec4(0.65F, 0.65F, 0.65F, 1.0F), "Dodge CD: %.1f s", player.dodgeCooldownRemaining);
        } else {
            ImGui::TextColored(ImVec4(0.40F, 0.85F, 0.50F, 1.0F), "Dodge: Ready [Shift]");
        }

        if (player.comboWindowOpen) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0F, 0.75F, 0.25F, 1.0F), "| Combo x%u", player.comboStep);
        }
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);

    // 3. Boss Health Bar (Top-Center)
    if (!player.bossName.empty() && GameHudCalculations::shouldShowBossBar(player.bossDefeated, player.bossHp, player.bossMaxHp, true)) {
        constexpr float bossBarWidth = 520.0F;
        constexpr float bossBarHeight = 74.0F;
        ImGui::SetNextWindowPos(ImVec2(vpPos.x + (vpSize.x - bossBarWidth) * 0.5F, vpPos.y + 18.0F), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(bossBarWidth, bossBarHeight), ImGuiCond_Always);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12F, 0.06F, 0.06F, 0.88F));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.70F, 0.20F, 0.20F, 0.80F));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0F);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.5F);
        if (ImGui::Begin("##BossHudFrame", nullptr, hudFlags)) {
            const char* phaseText = player.bossPhase.empty() ? "Phase 1" : player.bossPhase.c_str();
            const float textWidth = ImGui::CalcTextSize(player.bossName.c_str()).x;
            const float availX = ImGui::GetContentRegionAvail().x;
            if (availX > textWidth) {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availX - textWidth) * 0.5F);
            }
            if (player.targetLocked) {
                ImGui::TextColored(ImVec4(1.0F, 0.85F, 0.20F, 1.0F), "[TAB] TARGET LOCKED: %s (%s)", player.bossName.c_str(), phaseText);
            } else {
                ImGui::TextColored(ImVec4(1.0F, 0.82F, 0.40F, 1.0F), "%s  (%s)", player.bossName.c_str(), phaseText);
            }

            const float bossFraction = GameHudCalculations::calculateHpFraction(player.bossHp, player.bossMaxHp);
            char bossHpOverlay[64];
            std::snprintf(bossHpOverlay, sizeof(bossHpOverlay), "%.0f / %.0f (%.1f%%)", player.bossHp, player.bossMaxHp, bossFraction * 100.0F);
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.85F, 0.18F, 0.18F, 1.0F));
            ImGui::ProgressBar(bossFraction, ImVec2(-1.0F, 22.0F), bossHpOverlay);
            ImGui::PopStyleColor();
        }
        ImGui::End();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
    }

    // 4. Skill Action Bar (Bottom-Center)
    const std::size_t skillCount = std::min<std::size_t>(player.skills.size(), 4);
    constexpr float slotWidth = 72.0F;
    constexpr float slotHeight = 64.0F;
    constexpr float slotSpacing = 8.0F;
    const float totalBarWidth = (skillCount > 0 ? (static_cast<float>(skillCount) * slotWidth + static_cast<float>(skillCount - 1) * slotSpacing) : slotWidth) + 90.0F;

    ImGui::SetNextWindowPos(ImVec2(vpPos.x + (vpSize.x - totalBarWidth) * 0.5F, vpPos.y + vpSize.y - 82.0F), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(totalBarWidth, slotHeight + 12.0F), ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.06F, 0.08F, 0.11F, 0.80F));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.25F, 0.35F, 0.50F, 0.65F));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0F);
    const ImGuiWindowFlags barFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
    if (ImGui::Begin("##SkillActionBar", nullptr, barFlags)) {
        static const char* keyLabels[] = {"[LMB]", "[1]", "[2]", "[3]", "[4]"};
        static const ImVec4 themeBorders[] = {
            ImVec4(1.00F, 0.82F, 0.25F, 0.95F), // LMB: Basic Strike (Gold)
            ImVec4(1.00F, 0.35F, 0.30F, 0.95F), // [1]: Heavy Slash (Crimson)
            ImVec4(0.25F, 0.82F, 1.00F, 0.95F), // [2]: Whirlwind (Cyan)
            ImVec4(0.30F, 0.95F, 0.50F, 0.95F), // [3]: Shield Guard (Emerald)
            ImVec4(0.85F, 0.45F, 1.00F, 0.95F)  // [4]: Ultimate (Violet)
        };
        static const ImVec4 themeBgs[] = {
            ImVec4(0.24F, 0.20F, 0.10F, 0.90F),
            ImVec4(0.26F, 0.11F, 0.13F, 0.90F),
            ImVec4(0.10F, 0.22F, 0.34F, 0.90F),
            ImVec4(0.11F, 0.26F, 0.18F, 0.90F),
            ImVec4(0.23F, 0.12F, 0.30F, 0.90F)
        };
        for (std::size_t i = 0; i < skillCount; ++i) {
            const auto& skill = player.skills[i];
            const bool onCooldown = skill.cooldownRemaining > 0.0F;

            ImGui::BeginGroup();
            const ImVec4 slotBg = onCooldown ? ImVec4(0.14F, 0.14F, 0.16F, 0.85F)
                : (i < 5 ? themeBgs[i] : ImVec4(0.15F, 0.28F, 0.45F, 0.90F));
            const ImVec4 slotBorder = onCooldown ? ImVec4(0.38F, 0.38F, 0.42F, 0.55F)
                : (i < 5 ? themeBorders[i] : ImVec4(0.35F, 0.65F, 1.0F, 0.9F));
            ImGui::PushStyleColor(ImGuiCol_ChildBg, slotBg);
            ImGui::PushStyleColor(ImGuiCol_Border, slotBorder);
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0F);
            ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.5F);
            if (ImGui::BeginChild(("slot_" + skill.id).c_str(), ImVec2(slotWidth, slotHeight), true, barFlags)) {
                if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    requestedSkillId = skill.id;
                }
                const char* keyLabel = i < 5 ? keyLabels[i] : "[?]";
                ImGui::TextColored(ImVec4(1.0F, 0.88F, 0.35F, 1.0F), "%s", keyLabel);
                std::string shortName = skill.name.size() > 7 ? skill.name.substr(0, 6) + ".." : skill.name;
                ImGui::TextUnformatted(shortName.c_str());
                if (onCooldown) {
                    ImGui::TextColored(ImVec4(1.0F, 0.4F, 0.4F, 1.0F), "%.1fs", skill.cooldownRemaining);
                } else {
                    ImGui::TextColored(ImVec4(0.35F, 1.0F, 0.45F, 1.0F), "Ready");
                }
            }
            ImGui::EndChild();
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(2);
            ImGui::EndGroup();
            ImGui::SameLine(0.0F, slotSpacing);
        }

        // Dodge Slot
        ImGui::BeginGroup();
        const bool dodgeCd = player.dodgeCooldownRemaining > 0.0F;
        const ImVec4 dodgeBg = dodgeCd ? ImVec4(0.14F, 0.14F, 0.16F, 0.85F) : ImVec4(0.10F, 0.28F, 0.22F, 0.90F);
        const ImVec4 dodgeBorder = dodgeCd ? ImVec4(0.38F, 0.38F, 0.42F, 0.55F) : ImVec4(0.20F, 0.95F, 0.70F, 0.95F);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, dodgeBg);
        ImGui::PushStyleColor(ImGuiCol_Border, dodgeBorder);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0F);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.5F);
        if (ImGui::BeginChild("slot_dodge", ImVec2(76.0F, slotHeight), true, barFlags)) {
            ImGui::TextColored(ImVec4(0.4F, 1.0F, 0.7F, 1.0F), "[Shift]");
            ImGui::TextUnformatted("Dodge");
            if (dodgeCd) {
                ImGui::TextColored(ImVec4(1.0F, 0.4F, 0.4F, 1.0F), "%.1fs", player.dodgeCooldownRemaining);
            } else {
                ImGui::TextColored(ImVec4(0.35F, 1.0F, 0.45F, 1.0F), "Ready");
            }
        }
        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
        ImGui::EndGroup();
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);

    // Ingest new hit events for floating combat text
    for (const auto& hit : player.hitEvents) {
        FloatingCombatText ft;
        ft.damage = std::abs(hit.damage);
        ft.isPlayerDamage = hit.damage < 0.0F;
        ft.isCrit = ft.damage >= 30.0F;
        ft.worldPosition = hit.position + glm::vec3(0.0F, 0.4F, 0.0F);
        ft.age = 0.0F;
        ft.maxAge = 1.1F;
        floatingTexts_.push_back(ft);
    }

    // 5. Floating Combat Numbers & Target Reticle (Foreground DrawList)
    ImDrawList* fgDraw = ImGui::GetForegroundDrawList();
    if (fgDraw) {
        // Floating Numbers
        for (auto& ft : floatingTexts_) {
            ft.age += deltaSeconds;
            ft.worldPosition.y += deltaSeconds * 1.5F;
            ImVec2 screenPt;
            if (worldToScreen(ft.worldPosition, screenPt)) {
                const float alpha = std::clamp((ft.maxAge - ft.age) / 0.4F, 0.0F, 1.0F);
                const int alphaByte = static_cast<int>(alpha * 255.0F);
                if (alphaByte > 5) {
                    std::string text;
                    ImU32 col = 0;
                    if (ft.isPlayerDamage) {
                        text = "-" + std::to_string(static_cast<int>(ft.damage));
                        col = IM_COL32(255, 65, 65, alphaByte);
                    } else if (ft.isCrit) {
                        text = "CRIT! -" + std::to_string(static_cast<int>(ft.damage));
                        col = IM_COL32(255, 215, 0, alphaByte);
                    } else {
                        text = "-" + std::to_string(static_cast<int>(ft.damage));
                        col = IM_COL32(255, 235, 150, alphaByte);
                    }
                    const float textW = ImGui::CalcTextSize(text.c_str()).x;
                    const ImVec2 textPos(screenPt.x - textW * 0.5F, screenPt.y - 10.0F);
                    fgDraw->AddText(ImVec2(textPos.x + 1.0F, textPos.y + 1.0F), IM_COL32(0, 0, 0, alphaByte), text.c_str());
                    fgDraw->AddText(textPos, col, text.c_str());
                }
            }
        }
        std::erase_if(floatingTexts_, [](const FloatingCombatText& ft) { return ft.age >= ft.maxAge; });

        // Target Lock-On Diamond Reticle
        if (player.targetLocked && !player.bossDefeated) {
            ImVec2 lockScreenPt;
            if (worldToScreen(player.targetLockPosition + glm::vec3(0.0F, 2.2F, 0.0F), lockScreenPt)) {
                const float pulse = std::sin(hudAnimTime_ * 8.0F) * 3.0F;
                const float r = 16.0F + pulse;
                fgDraw->AddQuad(
                    ImVec2(lockScreenPt.x, lockScreenPt.y - r),
                    ImVec2(lockScreenPt.x + r, lockScreenPt.y),
                    ImVec2(lockScreenPt.x, lockScreenPt.y + r),
                    ImVec2(lockScreenPt.x - r, lockScreenPt.y),
                    IM_COL32(255, 205, 30, 230), 2.0F);
                fgDraw->AddCircleFilled(lockScreenPt, 3.5F, IM_COL32(255, 60, 60, 240));
                const char* lockBadge = "[LOCKED]";
                const float badgeW = ImGui::CalcTextSize(lockBadge).x;
                fgDraw->AddText(ImVec2(lockScreenPt.x - badgeW * 0.5F, lockScreenPt.y - r - 15.0F),
                    IM_COL32(255, 225, 60, 240), lockBadge);
            }
        }

        // Minimal Quick Hint (Centered above Skill Bar in a sleek translucent pill)
        const char* hint = "[WASD] Di chuyen  |  [Space] Nhay  |  [Shift] Ne don  |  [1-4 / LMB] Tung chieu  |  [Tab] Khoa muc tieu  |  [F1] Camera  |  [F3] Dev Tools";
        const float hintW = ImGui::CalcTextSize(hint).x;
        const ImVec2 hintPos(vpPos.x + (vpSize.x - hintW) * 0.5F, vpPos.y + vpSize.y - 110.0F);
        fgDraw->AddRectFilled(
            ImVec2(hintPos.x - 14.0F, hintPos.y - 4.0F),
            ImVec2(hintPos.x + hintW + 14.0F, hintPos.y + 18.0F),
            IM_COL32(12, 16, 24, 210), 8.0F);
        fgDraw->AddRect(
            ImVec2(hintPos.x - 14.0F, hintPos.y - 4.0F),
            ImVec2(hintPos.x + hintW + 14.0F, hintPos.y + 18.0F),
            IM_COL32(75, 100, 140, 160), 8.0F, 0, 1.0F);
        fgDraw->AddText(ImVec2(hintPos.x + 1.0F, hintPos.y + 1.0F), IM_COL32(0, 0, 0, 220), hint);
        fgDraw->AddText(hintPos, IM_COL32(215, 230, 248, 240), hint);
    }
}

void DebugPanel::beginFrame(float frameSeconds, std::uint64_t fixedTicks,
    const glm::vec3& cameraPosition, float cameraYaw, float cameraPitch, bool& thirdPersonMode,
    bool& enhancedCamera, RenderPipeline& renderPipeline,
    const PlayerFrameState& player, bool& resetBossRequested,
    std::string& requestedSkillId,
    std::string& statusCommand, std::vector<SkillEditorSkill>& editorSkills,
    const SaveSkillsCallback& saveSkills, const ReloadSkillsCallback& reloadSkills,
    std::vector<WorldEditorSpawnPoint>& worldSpawns,
    std::vector<WorldEditorTriggerZone>& worldTriggers,
    const SaveWorldCallback& saveWorld, const ReloadWorldCallback& reloadWorld,
    const MatchmakingCallback& matchmaking, AccountPanelState* accountPanel,
    const AccountActionCallback& accountAction, InventoryPanelState* inventoryPanel,
    const InventoryActionCallback& inventoryAction, ZonePanelState* zonePanel,
    const ZoneChangeCallback& zoneChange,
    SocialPanelState* socialPanel, const SocialActionCallback& socialAction,
    DungeonPanelState* dungeonPanel, const DungeonActionCallback& dungeonAction,
    MarketPanelState* marketPanel, const MarketActionCallback& marketAction,
    FeedbackPanelState* feedbackPanel, const FeedbackSubmitCallback& feedbackSubmit,
    DirectionalLight& light, ModelRenderer& renderer, AudioSystem& audio) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    if (frameSeconds > 0.0F) {
        const float instantaneousFps = 1.0F / frameSeconds;
        averageFps_ = averageFps_ == 0.0F ? instantaneousFps : averageFps_ * 0.9F + instantaneousFps * 0.1F;
    }

    // Always render clean Player Game HUD overlay
    renderGameHud(player, requestedSkillId, frameSeconds > 0.0F ? frameSeconds : 0.016F);

    // Closed Beta Feedback Window (F8)
    if (feedbackPanel) {
        feedbackPanel->open = feedbackOpenRequested_;
        if (feedbackPanel->open) {
            bool open = true;
            if (ImGui::Begin("Closed Beta Feedback (F8)", &open)) {
                static const char* categories[] = {"Bug", "Balance", "Suggestion", "Praise"};
                ImGui::Combo("Category", &feedbackPanel->category, categories, 4);
                ImGui::SliderInt("Rating", &feedbackPanel->rating, 1, 5, "%d / 5 stars");
                inputString("Subject", feedbackPanel->subject);
                std::array<char, 1024> description{};
                const auto copySize = std::min(feedbackPanel->description.size(), description.size() - 1);
                std::copy_n(feedbackPanel->description.data(), copySize, description.data());
                if (ImGui::InputTextMultiline("Description", description.data(), description.size(),
                    ImVec2(-1.0F, 90.0F))) feedbackPanel->description = description.data();

                const char* platform = SDL_GetPlatform();
                using GetGlString = const unsigned char* (*)(unsigned int);
                const auto getGlString = reinterpret_cast<GetGlString>(SDL_GL_GetProcAddress("glGetString"));
                const auto* rendererText = getGlString ? getGlString(GL_RENDERER) : nullptr;
                std::string gpu = rendererText
                    ? reinterpret_cast<const char*>(rendererText) : "Unknown OpenGL renderer";
                if (gpu.size() > 96) gpu.resize(96);
                const glm::vec3 position = player.position;
                std::string zone = zonePanel ? zonePanel->currentZone : "Unknown";
                if (zone.size() > 32) zone.resize(32);
                int level = 1;
                std::string characterName;
                if (accountPanel && accountPanel->selectedCharacter >= 0 &&
                    static_cast<std::size_t>(accountPanel->selectedCharacter) < accountPanel->characters.size()) {
                    const auto& character = accountPanel->characters[
                        static_cast<std::size_t>(accountPanel->selectedCharacter)];
                    level = character.level;
                    characterName = character.name;
                }
                ImGui::SeparatorText("Diagnostics");
                ImGui::Text("OS: %s | Renderer: %s", platform ? platform : "Unknown", gpu.c_str());
                ImGui::Text("Average FPS: %.1f | Ping RTT: %u ms", averageFps_,
                    zonePanel ? zonePanel->pingRttMs : 0U);
                ImGui::Text("Zone: %s | Position: %.2f, %.2f, %.2f", zone.c_str(), position.x, position.y, position.z);
                ImGui::Text("Character: %s | Class: %s | Level: %d", characterName.c_str(),
                    player.className.c_str(), level);
                if (ImGui::Button("Submit Feedback")) {
                    if (!accountPanel || !accountPanel->authenticated || !zonePanel || zonePanel->characterId == 0) {
                        feedbackPanel->message = "Log in and enter a character before submitting.";
                    } else if (!feedbackSubmit || feedbackPanel->subject.empty() || feedbackPanel->description.empty()) {
                        feedbackPanel->message = "Enter a subject and description.";
                    } else {
                        char diagnostics[451]{};
                        std::snprintf(diagnostics, sizeof(diagnostics),
                            "{\"platform\":\"%s\",\"renderer\":\"%s\",\"average_fps\":%.1f,\"ping_rtt_ms\":%u,\"zone\":\"%s\",\"position\":[%.3f,%.3f,%.3f],\"class\":\"%s\",\"level\":%d}",
                            diagnosticEscape(platform ? platform : "Unknown").c_str(),
                            diagnosticEscape(gpu).c_str(), averageFps_, zonePanel ? zonePanel->pingRttMs : 0U,
                            diagnosticEscape(zone).c_str(), position.x, position.y, position.z,
                            diagnosticEscape(player.className).c_str(), level);
                        feedbackPanel->message = feedbackSubmit(*feedbackPanel, diagnostics)
                            ? "Feedback sent; waiting for server confirmation..." : "Unable to send feedback.";
                    }
                }
                if (!feedbackPanel->message.empty()) ImGui::TextWrapped("%s", feedbackPanel->message.c_str());
            }
            ImGui::End();
            feedbackOpenRequested_ = open;
            feedbackPanel->open = open;
        }
    }

    // Clean Centered Login Modal if connected to server but not authenticated yet
    if (accountPanel && !accountPanel->authenticated) {
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        const ImVec2 vpPos = vp ? vp->Pos : ImVec2(0.0F, 0.0F);
        const ImVec2 vpSize = vp ? vp->Size : ImVec2(1280.0F, 720.0F);
        ImGui::SetNextWindowPos(ImVec2(vpPos.x + (vpSize.x - 380.0F) * 0.5F, vpPos.y + (vpSize.y - 240.0F) * 0.5F), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(380.0F, 240.0F), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Glory Account Login", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            inputString("Username", accountPanel->username);
            inputString("Password", accountPanel->password, ImGuiInputTextFlags_Password);
            if (ImGui::Button("Login") && accountAction &&
                !accountAction(AccountAction::Login, *accountPanel)) {
                accountPanel->message = "Not connected to the server.";
                accountPanel->messageIsError = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Register") && accountAction &&
                !accountAction(AccountAction::Register, *accountPanel)) {
                accountPanel->message = "Not connected to the server.";
                accountPanel->messageIsError = true;
            }
            if (!accountPanel->message.empty()) {
                const ImVec4 color = accountPanel->messageIsError
                    ? ImVec4(1.0F, 0.35F, 0.3F, 1.0F) : ImVec4(0.35F, 1.0F, 0.45F, 1.0F);
                ImGui::TextColored(color, "%s", accountPanel->message.c_str());
            }
            ImGui::Separator();
            if (ImGui::Button("Play Offline (Single Player Sandbox)", ImVec2(-1.0F, 26.0F))) {
                accountPanel->authenticated = true;
                accountPanel->message.clear();
            }
            ImGui::TextDisabled("Press F3 anytime for Dev Tools.");
        }
        ImGui::End();
    }

    // Unified Developer Tools Hub (F3)
    if (devToolsVisible_) {
        ImGui::SetNextWindowPos(ImVec2(24.0F, 24.0F), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(760.0F, 660.0F), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Glory Dev Tools (F3)", &devToolsVisible_)) {
            if (ImGui::BeginTabBar("DevToolsTabs")) {
                // ==================== TAB 1: Diagnostics & Engine ====================
                if (ImGui::BeginTabItem("Diagnostics & Engine")) {
                    const float safeFrameSeconds = std::isfinite(frameSeconds) && frameSeconds > 0.0F ? frameSeconds : 0.0F;
                    ImGui::Text("FPS: %.1f | Frame time: %.3f ms | Fixed ticks: %llu",
                        safeFrameSeconds > 0.0F ? 1.0F / safeFrameSeconds : 0.0F,
                        safeFrameSeconds * 1000.0F,
                        static_cast<unsigned long long>(fixedTicks));

                    ImGui::SeparatorText("Camera");
                    ImGui::Text("Mode: %s", thirdPersonMode ? "Third-Person" : "Free-Fly");
                    ImGui::Checkbox("Third-Person Camera (F1)", &thirdPersonMode);
                    ImGui::Checkbox("Enhanced Shoulder Camera (compare old)", &enhancedCamera);
                    ImGui::Text("Position: %.2f, %.2f, %.2f | Yaw: %.1f | Pitch: %.1f",
                        cameraPosition.x, cameraPosition.y, cameraPosition.z, cameraYaw, cameraPitch);
                    ImGui::SeparatorText("HDR / Linear Color / GPU Timing");
                    bool hdrEnabled = renderPipeline.hdrEnabled();
                    if (ImGui::Checkbox("HDR + Linear + Tonemap", &hdrEnabled)) renderPipeline.setHdrEnabled(hdrEnabled);
                    float exposure = renderPipeline.exposure();
                    if (ImGui::SliderFloat("Exposure", &exposure, 0.1F, 4.0F, "%.2f")) renderPipeline.setExposure(exposure);
                    ImGui::Text("Pipeline: %s | Exposure: %.2f", hdrEnabled ? "RGBA16F + ACES" : "Legacy direct", renderPipeline.exposure());
                    ImGui::Checkbox("Arena boundary debug line", &arenaBoundaryDebugEnabled_);
                    ImGui::TextDisabled("sRGB output is encoded once in the final shader pass.");
                    ImGui::Text("GPU timing: %s", renderPipeline.timerStatus().c_str());
                    ImGui::TextWrapped("Renderer: %s", renderPipeline.rendererInfo().c_str());
                    if (!renderPipeline.history().empty()) {
                        constexpr const char* passNames[] = {"Shadow", "Scene", "Sky", "Particle/overlay", "Tonemap", "GPU frame"};
                        const GpuTimingSample& latest = renderPipeline.history().back();
                        for (std::size_t index = 0; index < latest.milliseconds.size(); ++index)
                            ImGui::Text("%s: %.3f ms", passNames[index], latest.milliseconds[index]);
                        std::vector<float> frameHistory;
                        frameHistory.reserve(renderPipeline.history().size());
                        float maxGpuFrameMs = 1.0F;
                        for (const GpuTimingSample& sample : renderPipeline.history())
                        {
                            const float frameMs = static_cast<float>(sample.milliseconds[static_cast<std::size_t>(GpuPass::Total)]);
                            frameHistory.push_back(frameMs);
                            maxGpuFrameMs = std::max(maxGpuFrameMs, frameMs);
                        }
                        ImGui::PlotLines("GPU frame history (ms)", frameHistory.data(),
                            static_cast<int>(frameHistory.size()), 0, nullptr, 0.0F, maxGpuFrameMs * 1.1F, ImVec2(0.0F, 60.0F));
                    } else if (renderPipeline.timerQueriesAvailable()) {
                        ImGui::TextDisabled("Waiting for asynchronous GPU query results...");
                    }

                    ImGui::SeparatorText("Player Telemetry");
                    ImGui::Text("Position: %.2f, %.2f, %.2f | Velocity: %.2f, %.2f, %.2f",
                        player.position.x, player.position.y, player.position.z,
                        player.velocity.x, player.velocity.y, player.velocity.z);
                    ImGui::Text("Grounded: %s | Collision contacts: %u",
                        player.isGrounded ? "Yes" : "No", player.collisionCount);

                    ImGui::SeparatorText("Shadows");
                    bool shadowsEnabled = renderer.shadowsEnabled();
                    if (ImGui::Checkbox("Enable Shadow", &shadowsEnabled)) renderer.setShadowsEnabled(shadowsEnabled);
                    static int lightingPreset = 0;
                    static const char* presetNames[] = {"Sunset", "Sunrise", "Noon", "Old"};
                    if (ImGui::Combo("Lighting preset", &lightingPreset, presetNames, 4)) {
                        switch (lightingPreset) {
                        case 0:
                            light.direction = {-0.612F, -0.5F, -0.612F}; light.color = {1.0F, 0.68F, 0.42F};
                            light.ambientStrength = 0.72F; light.diffuseStrength = 1.35F; break;
                        case 1:
                            light.direction = {-0.36F, -0.5F, -0.78F}; light.color = {0.72F, 0.62F, 1.0F};
                            light.ambientStrength = 0.82F; light.diffuseStrength = 1.1F; break;
                        case 2:
                            light.direction = {-0.18F, -0.94F, -0.30F}; light.color = {1.0F, 0.96F, 0.88F};
                            light.ambientStrength = 0.9F; light.diffuseStrength = 1.0F; break;
                        default:
                            light.direction = {0.25F, -0.85F, 0.45F}; light.color = {1.0F, 0.96F, 0.90F};
                            light.ambientStrength = 0.55F; light.diffuseStrength = 0.95F; break;
                        }
                    }
                    bool hemisphere = renderer.hemisphereEnabled();
                    if (ImGui::Checkbox("Hemisphere ambient", &hemisphere)) renderer.setHemisphereEnabled(hemisphere);
                    bool fog = renderer.fogEnabled();
                    if (ImGui::Checkbox("Distance + height fog", &fog)) renderer.setFogEnabled(fog);
                    float fogDensity = renderer.fogDensity();
                    if (ImGui::SliderFloat("Fog density", &fogDensity, 0.0F, 0.03F, "%.4f")) renderer.setFogDensity(fogDensity);
                    bool newSky = renderer.skyboxEnabled();
                    if (ImGui::Checkbox("Layered sunset sky", &newSky)) renderer.setSkyboxEnabled(newSky);
                    bool characterLighting = renderer.characterLightingEnabled();
                    if (ImGui::Checkbox("Character fill / rim", &characterLighting))
                        renderer.setCharacterLightingEnabled(characterLighting);
                    float shadowBias = renderer.shadowBias();
                    if (ImGui::SliderFloat("Shadow Bias", &shadowBias, 0.0001F, 0.02F, "%.4f", ImGuiSliderFlags_Logarithmic)) {
                        renderer.setShadowBias(shadowBias);
                    }

                    ImGui::SeparatorText("PBR Material & Skybox");
                    float metallic = renderer.metallic();
                    float roughness = renderer.roughness();
                    float ao = renderer.ambientOcclusion();
                    bool pbrChanged = ImGui::SliderFloat("Metallic", &metallic, 0.0F, 1.0F);
                    pbrChanged |= ImGui::SliderFloat("Roughness", &roughness, 0.04F, 1.0F);
                    pbrChanged |= ImGui::SliderFloat("AO", &ao, 0.0F, 1.0F);
                    if (pbrChanged) renderer.setPbrMaterial(metallic, roughness, ao);
                    bool skyboxEnabled = renderer.skyboxEnabled();
                    if (ImGui::Checkbox("Enable Skybox", &skyboxEnabled)) renderer.setSkyboxEnabled(skyboxEnabled);

                    ImGui::SeparatorText("Particles & Audio");
                    if (ImGui::Button("Test particle burst")) (void)renderer.spawnParticleBurst({0.0F, -0.2F, 0.0F});
                    ImGui::SameLine();
                    ImGui::Text("Active particles: %zu", renderer.activeParticleCount());
                    float masterVolume = audio.masterVolume();
                    if (ImGui::SliderFloat("Master Volume", &masterVolume, 0.0F, 1.0F)) audio.setMasterVolume(masterVolume);
                    bool muted = audio.muted();
                    if (ImGui::Checkbox("Mute", &muted)) audio.setMuted(muted);
                    ImGui::SameLine();
                    ImGui::Text("Audio: %s", audio.available() ? "Ready" : "Unavailable");

                    ImGui::SeparatorText("Directional Light");
                    static float lightYaw = -135.0F;
                    static float lightPitch = -30.0F;
                    const bool yawChanged = ImGui::SliderFloat("Light yaw", &lightYaw, -180.0F, 180.0F);
                    const bool pitchChanged = ImGui::SliderFloat("Light pitch", &lightPitch, -89.0F, 89.0F);
                    if (yawChanged || pitchChanged) {
                        setDirectionFromAngles(light, lightYaw, lightPitch);
                    }
                    ImGui::ColorEdit3("Light color", &light.color.x);
                    ImGui::SliderFloat("Ambient", &light.ambientStrength, 0.35F, 1.5F);
                    ImGui::SliderFloat("Diffuse", &light.diffuseStrength, 0.0F, 2.0F);
                    ImGui::SliderFloat("Specular", &light.specularStrength, 0.0F, 2.0F);
                    ImGui::SliderFloat("Shininess", &light.shininess, 2.0F, 256.0F, "%.0f", ImGuiSliderFlags_Logarithmic);
                    ImGui::EndTabItem();
                }

                // ==================== TAB 2: Combat & Skills ====================
                if (ImGui::BeginTabItem("Combat & Skills")) {
                    ImGui::SeparatorText("Boss Encounter State");
                    ImGui::Text("Boss: %s | Phase: %s", player.bossName.c_str(), player.bossPhase.c_str());
                    ImGui::Text("Boss HP: %.0f / %.0f", player.bossHp, player.bossMaxHp);
                    const float bossHpFraction = player.bossMaxHp > 0.0F ? player.bossHp / player.bossMaxHp : 0.0F;
                    ImGui::ProgressBar(bossHpFraction, ImVec2(-1.0F, 0.0F), "Boss Health");
                    ImGui::Text("[BT Node: %s] | Hits received: %u", player.bossBehaviorNode.c_str(), player.dummyHitCount);
                    if (player.bossDefeated)
                        ImGui::TextColored(ImVec4(0.35F, 1.0F, 0.45F, 1.0F), "VICTORY! BOSS DEFEATED");
                    if (ImGui::Button("Reset Boss Fight")) resetBossRequested = true;

                    ImGui::SeparatorText("Status Effects & Buffs");
                    if (ImGui::Button("Player Speed +30%")) statusCommand = "player_speed";
                    ImGui::SameLine();
                    if (ImGui::Button("Player Attack +30%")) statusCommand = "player_attack";
                    if (ImGui::Button("Dummy Burn")) statusCommand = "dummy_burn";
                    ImGui::SameLine();
                    if (ImGui::Button("Dummy Stun")) statusCommand = "dummy_stun";
                    ImGui::SameLine();
                    if (ImGui::Button("Clear All Effects")) statusCommand = "clear_effects";

                    ImGui::SeparatorText("Combat Feel Telemetry");
                    ImGui::Text("Combo step: %u | Combo window: %s | Cancel: %s | Hit-stop: %s",
                        player.comboStep, player.comboWindowOpen ? "Open" : "Closed",
                        player.cancelWindowAvailable ? "Available" : "Unavailable",
                        player.hitStopActive ? "Active" : "Inactive");
                    ImGui::Text("Dodging: %s | Dodge CD: %.2f s | I-Frame: %s",
                        player.isDodging ? "Yes" : "No", player.dodgeCooldownRemaining,
                        player.isInvulnerable ? "Active" : "Inactive");

                    ImGui::SeparatorText("Animation");
                    ImGui::Text("State: %s", renderer.animationState().c_str());
                    if (ImGui::Button("Idle")) (void)renderer.triggerAnimationState("Idle");
                    ImGui::SameLine();
                    if (ImGui::Button("Run")) (void)renderer.triggerAnimationState("Run");
                    ImGui::SameLine();
                    if (ImGui::Button("Attack")) (void)renderer.triggerAnimationState("Attack");
                    float fadeDuration = renderer.fadeDuration();
                    if (ImGui::SliderFloat("Fade duration", &fadeDuration, 0.0F, 1.0F, "%.2f s")) {
                        renderer.setFadeDuration(fadeDuration);
                    }
                    ImGui::Text("Blend factor: %.2f | Clip: %s", renderer.blendFactor(), renderer.animationName().c_str());
                    if (renderer.hasAnimation()) {
                        if (renderer.animationCount() > 1) {
                            if (ImGui::BeginCombo("Animation clip", renderer.animationName().c_str())) {
                                for (std::size_t index = 0; index < renderer.animationCount(); ++index) {
                                    const bool selected = index == renderer.animationIndex();
                                    if (ImGui::Selectable(renderer.animationName(index).c_str(), selected)) renderer.selectAnimation(index);
                                    if (selected) ImGui::SetItemDefaultFocus();
                                }
                                ImGui::EndCombo();
                            }
                        }
                        if (ImGui::Button(renderer.isAnimationPlaying() ? "Pause" : "Play")) {
                            renderer.setAnimationPlaying(!renderer.isAnimationPlaying());
                        }
                        float animationTime = renderer.animationTime();
                        if (ImGui::SliderFloat("Timeline", &animationTime, 0.0F, renderer.animationDuration(), "%.2f s")) {
                            renderer.setAnimationTime(animationTime);
                        }
                    }

                    ImGui::SeparatorText("Skill & Event Editor");
                    if (editorSkills.empty()) {
                        ImGui::TextUnformatted("No skill data loaded.");
                    } else {
                        if (selectedEditorSkill_.empty() || std::none_of(editorSkills.begin(), editorSkills.end(),
                            [&](const SkillEditorSkill& skill) { return skill.id == selectedEditorSkill_; }))
                            selectedEditorSkill_ = editorSkills.front().id;
                        if (ImGui::BeginCombo("Skill", selectedEditorSkill_.c_str())) {
                            for (const SkillEditorSkill& skill : editorSkills) {
                                if (ImGui::Selectable((skill.id + " - " + skill.name).c_str(), skill.id == selectedEditorSkill_))
                                    selectedEditorSkill_ = skill.id;
                            }
                            ImGui::EndCombo();
                        }
                        auto foundSkill = std::find_if(editorSkills.begin(), editorSkills.end(), [&](const SkillEditorSkill& skill) {
                            return skill.id == selectedEditorSkill_;
                        });
                        if (foundSkill != editorSkills.end()) {
                            SkillEditorSkill& skill = *foundSkill;
                            inputString("Name", skill.name);
                            ImGui::InputFloat("Damage", &skill.damage, 1.0F, 5.0F, "%.2f");
                            ImGui::InputFloat("Cooldown", &skill.cooldown, 0.05F, 0.25F, "%.3f s");
                            ImGui::InputFloat("Cast Time", &skill.castTime, 0.05F, 0.25F, "%.3f s");
                            inputString("Animation State", skill.animationState);
                            inputString("Combo Next", skill.comboNext);
                            ImGui::InputFloat("Combo Window", &skill.comboWindow, 0.05F, 0.25F, "%.3f s");
                            ImGui::InputFloat("Input Buffer", &skill.inputBuffer, 0.05F, 0.25F, "%.3f s");
                            ImGui::InputFloat("Cancel Window Start", &skill.cancelWindowStart, 0.05F, 0.25F, "%.3f s");
                            ImGui::InputFloat("Hit-Stop Duration", &skill.hitStopDuration, 0.01F, 0.05F, "%.3f s");
                            ImGui::InputFloat("Knockback Speed", &skill.knockbackSpeed, 0.25F, 1.0F, "%.2f");
                            ImGui::InputFloat("Knockback Duration", &skill.knockbackDuration, 0.025F, 0.1F, "%.3f s");
                            ImGui::InputFloat("Hitbox Radius", &skill.hitboxRadius, 0.05F, 0.25F, "%.2f");
                            ImGui::InputFloat3("Hitbox Offset", &skill.hitboxOffset.x, "%.2f");
                            ImGui::InputFloat("Active Start", &skill.activeStart, 0.01F, 0.05F, "%.3f s");
                            ImGui::InputFloat("Active End", &skill.activeEnd, 0.01F, 0.05F, "%.3f s");

                            float timelineDuration = std::max(skill.activeEnd + skill.comboWindow, skill.castTime + skill.activeEnd);
                            for (const SkillEditorEvent& event : skill.animationEvents)
                                timelineDuration = std::max(timelineDuration, event.time);
                            timelineDuration = std::max(timelineDuration, 0.05F);
                            ImGui::SliderFloat("Timeline Scrubber", &timelineCursor_, 0.0F, timelineDuration, "%.3f s");
                        }
                    }
                    if (ImGui::Button("Save to JSON")) {
                        editorMessage_ = saveSkills ? (saveSkills(editorSkills, editorMessage_) ? "Saved skills.json" : editorMessage_)
                                                    : "Save callback is unavailable";
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Hot-Reload Skills (F5)") || ImGui::IsKeyPressed(ImGuiKey_F5)) {
                        if (reloadSkills) editorMessage_ = reloadSkills(editorSkills, editorMessage_)
                            ? "Skills reloaded (F5)" : editorMessage_;
                        else editorMessage_ = "Reload callback is unavailable";
                    }
                    if (!editorMessage_.empty()) ImGui::TextWrapped("%s", editorMessage_.c_str());
                    ImGui::EndTabItem();
                }

                // ==================== TAB 3: World & Spawners ====================
                if (ImGui::BeginTabItem("World & Spawners")) {
                    ImGui::SeparatorText("Spawn Points");
                    if (worldSpawns.empty()) {
                        ImGui::TextUnformatted("No spawn points loaded.");
                    } else {
                        if (selectedSpawnId_.empty() || std::none_of(worldSpawns.begin(), worldSpawns.end(),
                            [&](const WorldEditorSpawnPoint& spawn) { return spawn.id == selectedSpawnId_; }))
                            selectedSpawnId_ = worldSpawns.front().id;
                        if (ImGui::BeginCombo("Spawn Point", selectedSpawnId_.c_str())) {
                            for (const auto& spawn : worldSpawns)
                                if (ImGui::Selectable((spawn.name + " (" + spawn.id + ")").c_str(), spawn.id == selectedSpawnId_))
                                    selectedSpawnId_ = spawn.id;
                            ImGui::EndCombo();
                        }
                        auto spawnIt = std::find_if(worldSpawns.begin(), worldSpawns.end(), [&](const auto& spawn) {
                            return spawn.id == selectedSpawnId_;
                        });
                        if (spawnIt != worldSpawns.end()) {
                            WorldEditorSpawnPoint& spawn = *spawnIt;
                            inputString("Spawn Name", spawn.name);
                            const char* entityTypes[] = {"Player", "PracticeDummy", "Boss"};
                            int entityType = spawn.entityType == "Boss" ? 2 : (spawn.entityType == "PracticeDummy" ? 1 : 0);
                            if (ImGui::Combo("Entity Type", &entityType, entityTypes, IM_ARRAYSIZE(entityTypes)))
                                spawn.entityType = entityTypes[entityType];
                            ImGui::InputFloat3("Position", &spawn.position.x, "%.2f");
                            ImGui::InputFloat("Yaw", &spawn.yaw, 1.0F, 10.0F, "%.1f deg");
                            if (ImGui::Button("Snap to Player")) spawn.position = player.position;
                        }
                    }

                    ImGui::SeparatorText("Trigger Zones");
                    if (ImGui::Button("Add Trigger Zone")) {
                        const std::string id = "trigger_" + std::to_string(worldTriggers.size() + 1);
                        worldTriggers.push_back({id, "New Trigger", "Box", player.position, glm::vec3(2.0F), 2.0F,
                            "EncounterBoundary"});
                        selectedTriggerId_ = id;
                    }
                    if (!worldTriggers.empty()) {
                        if (selectedTriggerId_.empty() || std::none_of(worldTriggers.begin(), worldTriggers.end(),
                            [&](const WorldEditorTriggerZone& zone) { return zone.id == selectedTriggerId_; }))
                            selectedTriggerId_ = worldTriggers.front().id;
                        if (ImGui::BeginCombo("Trigger Zone", selectedTriggerId_.c_str())) {
                            for (const auto& zone : worldTriggers)
                                if (ImGui::Selectable((zone.name + " (" + zone.id + ")").c_str(), zone.id == selectedTriggerId_))
                                    selectedTriggerId_ = zone.id;
                            ImGui::EndCombo();
                        }
                        auto zoneIt = std::find_if(worldTriggers.begin(), worldTriggers.end(), [&](const auto& zone) {
                            return zone.id == selectedTriggerId_;
                        });
                        if (zoneIt != worldTriggers.end()) {
                            WorldEditorTriggerZone& zone = *zoneIt;
                            inputString("Trigger Name", zone.name);
                            ImGui::InputFloat3("Center", &zone.center.x, "%.2f");
                            const char* shapes[] = {"Box", "Sphere"};
                            int shape = zone.shape == "Sphere" ? 1 : 0;
                            if (ImGui::Combo("Shape", &shape, shapes, IM_ARRAYSIZE(shapes))) zone.shape = shapes[shape];
                            if (zone.shape == "Box") ImGui::InputFloat3("Half Extents", &zone.extents.x, "%.2f");
                            else ImGui::InputFloat("Radius", &zone.radius, 0.1F, 0.5F, "%.2f");
                            const char* triggerTypes[] = {"EncounterBoundary", "BossAggro", "HealArea", "HazardArea"};
                            int triggerType = 0;
                            for (int index = 0; index < IM_ARRAYSIZE(triggerTypes); ++index)
                                if (zone.triggerType == triggerTypes[index]) triggerType = index;
                            if (ImGui::Combo("Trigger Type", &triggerType, triggerTypes, IM_ARRAYSIZE(triggerTypes)))
                                zone.triggerType = triggerTypes[triggerType];
                            if (ImGui::SmallButton("Remove Trigger Zone")) {
                                worldTriggers.erase(zoneIt);
                                selectedTriggerId_.clear();
                            }
                        }
                    }

                    ImGui::SeparatorText("Active Trigger States");
                    if (player.activeTriggerStates.empty()) ImGui::TextUnformatted("No active trigger zones.");
                    else for (const std::string& state : player.activeTriggerStates) ImGui::BulletText("%s", state.c_str());

                    if (ImGui::Button("Save World Data")) {
                        if (saveWorld) worldEditorMessage_ = saveWorld(worldSpawns, worldTriggers, worldEditorMessage_)
                            ? "World data saved" : worldEditorMessage_;
                        else worldEditorMessage_ = "Save callback is unavailable";
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Reload Spawns")) {
                        if (reloadWorld) worldEditorMessage_ = reloadWorld(worldSpawns, worldTriggers, worldEditorMessage_)
                            ? "World spawns reloaded" : worldEditorMessage_;
                        else worldEditorMessage_ = "Reload callback is unavailable";
                    }
                    if (!worldEditorMessage_.empty()) ImGui::TextWrapped("%s", worldEditorMessage_.c_str());
                    ImGui::EndTabItem();
                }

                // ==================== TAB 4: Multiplayer & Arena ====================
                if (ImGui::BeginTabItem("Multiplayer & Arena")) {
                    if (player.networkConnected) {
                        ImGui::SeparatorText("Authoritative Network State");
                        ImGui::Text("Server tick: %llu | players: %u",
                            static_cast<unsigned long long>(player.serverTick), player.serverPlayerCount);
                        ImGui::Text("Client %u pos: %.2f, %.2f, %.2f | HP: %.1f / %.1f",
                            player.authoritativeClientId, player.authoritativePosition.x, player.authoritativePosition.y, player.authoritativePosition.z,
                            player.authoritativeHp, player.authoritativeMaxHp);
                        ImGui::Text("Prediction: client seq %u | server ack %u | pending %u",
                            player.clientInputSequence, player.serverAckSequence, player.unacknowledgedInputCount);
                        ImGui::Text("Reconciliations: %llu | offset: %.3f m",
                            static_cast<unsigned long long>(player.reconciliationCount), player.mispredictionOffset);
                    } else {
                        ImGui::TextUnformatted("Network: Offline / Disconnected");
                    }

                    ImGui::SeparatorText("Arena Matchmaking");
                    static int selectedMode = 1;
                    const char* modes[] = {"Duel 1v1", "Team 3v3"};
                    ImGui::Combo("Mode", &selectedMode, modes, IM_ARRAYSIZE(modes));
                    if (ImGui::Button("Join Matchmaking") && matchmaking)
                        (void)matchmaking(static_cast<unsigned int>(selectedMode + 1), true);
                    ImGui::SameLine();
                    if (ImGui::Button("Leave Queue") && matchmaking)
                        (void)matchmaking(static_cast<unsigned int>(selectedMode + 1), false);
                    static const char* matchStates[] = {"Waiting", "Countdown", "In Round", "Finished"};
                    const unsigned int stateIndex = std::min(player.matchState, 3U);
                    ImGui::Text("State: %s | Mode: %s", matchStates[stateIndex],
                        player.matchMode == 2 ? "Team 3v3" : "Duel 1v1");
                    if (player.matchState == 1) ImGui::Text("Round starts in %.1f s", player.countdownRemaining);

                    if (zonePanel) {
                        ImGui::SeparatorText("Zone & AOI");
                        ImGui::Text("Current zone: %s | Ping RTT: %u ms", zonePanel->currentZone.c_str(), zonePanel->pingRttMs);
                        ImGui::Text("Server entities: %u | Visible in AOI: %u", zonePanel->totalEntityCount, zonePanel->visibleEntityCount);
                        if (zonePanel->characterId > 0) {
                            if (ImGui::Button("Teleport to Zone 1") && zoneChange) (void)zoneChange(0, *zonePanel);
                            ImGui::SameLine();
                            if (ImGui::Button("Teleport to Zone 2") && zoneChange) (void)zoneChange(1, *zonePanel);
                        }
                    }

                    if (dungeonPanel) {
                        ImGui::SeparatorText("Dungeon Instance");
                        ImGui::Text("Dungeon: %s", dungeonPanel->dungeonName.c_str());
                        if (dungeonPanel->inDungeon) {
                            ImGui::Text("Instance ID: %llu | Timer: %.0f s", static_cast<unsigned long long>(dungeonPanel->instanceId), dungeonPanel->timerRemaining);
                            if (ImGui::Button("Leave Dungeon") && dungeonAction) (void)dungeonAction(DungeonAction::Leave, *dungeonPanel);
                        } else if (dungeonPanel->characterId > 0) {
                            if (ImGui::Button("Enter Dungeon (Solo)") && dungeonAction) (void)dungeonAction(DungeonAction::EnterSolo, *dungeonPanel);
                            ImGui::SameLine();
                            if (ImGui::Button("Enter Dungeon (Party)") && dungeonAction) (void)dungeonAction(DungeonAction::EnterParty, *dungeonPanel);
                        }
                    }
                    ImGui::EndTabItem();
                }

                // ==================== TAB 5: Account & Economy ====================
                if (ImGui::BeginTabItem("Account & Economy")) {
                    if (accountPanel) {
                        ImGui::SeparatorText("Account & Character Selection");
                        if (!accountPanel->authenticated) {
                            inputString("Username", accountPanel->username);
                            inputString("Password", accountPanel->password, ImGuiInputTextFlags_Password);
                            if (ImGui::Button("Login") && accountAction) (void)accountAction(AccountAction::Login, *accountPanel);
                            ImGui::SameLine();
                            if (ImGui::Button("Register") && accountAction) (void)accountAction(AccountAction::Register, *accountPanel);
                        } else {
                            ImGui::Text("Signed in as %s", accountPanel->username.c_str());
                            for (std::size_t index = 0; index < accountPanel->characters.size(); ++index) {
                                const auto& character = accountPanel->characters[index];
                                const std::string label = character.name + " | Lv. " + std::to_string(character.level) + " | " + character.className;
                                if (ImGui::Selectable(label.c_str(), accountPanel->selectedCharacter == static_cast<int>(index)))
                                    accountPanel->selectedCharacter = static_cast<int>(index);
                            }
                            const bool hasSelection = accountPanel->selectedCharacter >= 0 &&
                                static_cast<std::size_t>(accountPanel->selectedCharacter) < accountPanel->characters.size();
                            if (hasSelection && ImGui::Button("Enter World") && accountAction)
                                (void)accountAction(AccountAction::EnterWorld, *accountPanel);
                        }
                    }

                    if (inventoryPanel && inventoryPanel->characterId > 0) {
                        ImGui::SeparatorText("Inventory (24 slots)");
                        if (ImGui::BeginTable("DevInventorySlots", 6, ImGuiTableFlags_Borders)) {
                            for (int index = 0; index < 24; ++index) {
                                ImGui::TableNextColumn();
                                const bool occupied = static_cast<std::size_t>(index) < inventoryPanel->slots.size() &&
                                    !inventoryPanel->slots[static_cast<std::size_t>(index)].itemId.empty();
                                std::string label = "[" + std::to_string(index + 1) + "] Empty";
                                if (occupied) {
                                    const auto& item = inventoryPanel->slots[static_cast<std::size_t>(index)];
                                    label = "[" + std::to_string(index + 1) + "] " + item.name + " x" + std::to_string(item.quantity);
                                }
                                if (ImGui::Selectable(label.c_str(), inventoryPanel->selectedInventorySlot == index))
                                    inventoryPanel->selectedInventorySlot = index;
                            }
                            ImGui::EndTable();
                        }
                        if (ImGui::Button("Simulate Boss Loot Drop") && inventoryAction)
                            (void)inventoryAction(InventoryAction::SimulateBossLoot, *inventoryPanel);
                    }

                    if (marketPanel) {
                        ImGui::SeparatorText("Marketplace");
                        ImGui::Text("Gold: %lld", static_cast<long long>(marketPanel->gold));
                        inputString("Search Items", marketPanel->search);
                        if (ImGui::Button("Refresh Listings") && marketAction)
                            (void)marketAction(MarketAction::Browse, *marketPanel);
                    }

                    if (socialPanel) {
                        ImGui::SeparatorText("Social & Chat");
                        inputString("Chat Input", socialPanel->chatInput);
                        if (ImGui::Button("Send World Chat") && socialAction) {
                            socialPanel->selectedChannel = 0;
                            (void)socialAction(SocialAction::SendChat, *socialPanel);
                            socialPanel->chatInput.clear();
                        }
                    }
                    ImGui::EndTabItem();
                }

                // ==================== TAB 6: Anti-Cheat Monitor ====================
                if (ImGui::BeginTabItem("Anti-Cheat Monitor")) {
                    ImGui::Text("Current speed: %.2f m/s | Maximum allowed: %.2f m/s",
                        player.antiCheatCurrentSpeed, player.antiCheatMaximumSpeed);
                    ImGui::Text("Violations: speed %u | teleport %u | collision %u | cooldown %u",
                        player.speedViolations, player.teleportViolations, player.collisionViolations, player.cooldownViolations);
                    if (!player.antiCheatMessage.empty()) ImGui::TextWrapped("Server: %s", player.antiCheatMessage.c_str());

                    ImGui::SeparatorText("Cheat injection probes");
                    if (ImGui::Button("Inject Speed Hack (+500% speed)")) statusCommand = "anti_cheat_speed_probe";
                    if (ImGui::Button("Inject Cooldown Hack (Spam skill)")) statusCommand = "anti_cheat_cooldown_probe";
                    if (ImGui::Button("Inject Teleport Hack (Jump 50m)")) statusCommand = "anti_cheat_teleport_probe";
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        }
        ImGui::End();
    }

    const ImGuiIO& io = ImGui::GetIO();
    captureMouse_ = io.WantCaptureMouse;
    captureKeyboard_ = io.WantCaptureKeyboard;
}

void DebugPanel::render() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

} // namespace glory::engine
