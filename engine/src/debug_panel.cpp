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
    if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_F8 && !event.key.repeat)
        feedbackOpenRequested_ = !feedbackOpenRequested_;
}

bool DebugPanel::wantsCaptureMouse() const noexcept { return captureMouse_; }
bool DebugPanel::wantsCaptureKeyboard() const noexcept { return captureKeyboard_; }

void DebugPanel::logAnimationEvent(const AnimationEvent& event) {
    animationEventLog_.push_back(event.name + " @ " + std::to_string(event.time) + " s");
    if (animationEventLog_.size() > 8) animationEventLog_.erase(animationEventLog_.begin());
}

void DebugPanel::beginFrame(float frameSeconds, std::uint64_t fixedTicks,
    const glm::vec3& cameraPosition, float cameraYaw, float cameraPitch, bool& thirdPersonMode,
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

    if (accountPanel) {
        ImGui::Begin("Account & Characters");
        if (!accountPanel->authenticated) {
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
        } else {
            ImGui::Text("Signed in as %s", accountPanel->username.c_str());
            if (ImGui::Button("Refresh Characters") && accountAction &&
                !accountAction(AccountAction::RefreshCharacters, *accountPanel)) {
                accountPanel->message = "Unable to request characters.";
                accountPanel->messageIsError = true;
            }
            ImGui::SeparatorText("Characters");
            if (accountPanel->characters.empty()) ImGui::TextUnformatted("No characters yet.");
            for (std::size_t index = 0; index < accountPanel->characters.size(); ++index) {
                const auto& character = accountPanel->characters[index];
                const std::string label = character.name + " | Lv. " + std::to_string(character.level) +
                    " | " + character.className;
                if (ImGui::Selectable(label.c_str(), accountPanel->selectedCharacter == static_cast<int>(index)))
                    accountPanel->selectedCharacter = static_cast<int>(index);
            }
            if (ImGui::Button("Create New Character")) ImGui::OpenPopup("Create Character");
            if (ImGui::BeginPopup("Create Character")) {
                inputString("Character Name", accountPanel->characterName);
                if (ImGui::BeginCombo("Class", accountPanel->className.c_str())) {
                    for (const auto& classChoice : accountPanel->classChoices) {
                        const bool selected = accountPanel->className == classChoice.name;
                        if (ImGui::Selectable(classChoice.name.c_str(), selected))
                            accountPanel->className = classChoice.name;
                        if (selected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                const auto classInfo = std::find_if(accountPanel->classChoices.begin(),
                    accountPanel->classChoices.end(), [&](const ClassChoiceView& entry) {
                        return entry.name == accountPanel->className;
                    });
                if (classInfo != accountPanel->classChoices.end()) {
                    ImGui::TextWrapped("%s", classInfo->description.c_str());
                    ImGui::Text("HP %.0f | Attack %.0f | Defense %.0f | Speed %.1f m/s",
                        classInfo->hp, classInfo->attack, classInfo->defense, classInfo->speed);
                    ImGui::Text("Dodge cooldown: %.1f s", classInfo->dodgeCooldown);
                    std::string skillNames = "Skills: ";
                    for (std::size_t index = 0; index < classInfo->skills.size(); ++index) {
                        if (index) skillNames += ", ";
                        skillNames += classInfo->skills[index];
                    }
                    ImGui::TextWrapped("%s", skillNames.c_str());
                }
                if (ImGui::Button("Create") && accountAction &&
                    !accountAction(AccountAction::CreateCharacter, *accountPanel)) {
                    accountPanel->message = "Unable to create character.";
                    accountPanel->messageIsError = true;
                }
                ImGui::EndPopup();
            }
            const bool hasSelection = accountPanel->selectedCharacter >= 0 &&
                static_cast<std::size_t>(accountPanel->selectedCharacter) < accountPanel->characters.size();
            if (!hasSelection) ImGui::BeginDisabled();
            if (ImGui::Button("Enter World with Character") && hasSelection && accountAction &&
                !accountAction(AccountAction::EnterWorld, *accountPanel)) {
                accountPanel->message = "Unable to select this character.";
                accountPanel->messageIsError = true;
            }
            if (!hasSelection) ImGui::EndDisabled();
        }
        if (!accountPanel->message.empty()) {
            const ImVec4 color = accountPanel->messageIsError
                ? ImVec4(1.0F, 0.35F, 0.3F, 1.0F) : ImVec4(0.35F, 1.0F, 0.45F, 1.0F);
            ImGui::TextColored(color, "%s", accountPanel->message.c_str());
        }
        ImGui::End();
    }

    if (inventoryPanel && inventoryPanel->characterId > 0) {
        ImGui::Begin("Inventory & Equipment");
        ImGui::Text("Character ID: %lld", static_cast<long long>(inventoryPanel->characterId));
        ImGui::SeparatorText("Inventory (24 slots)");
        if (ImGui::BeginTable("InventorySlots", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame)) {
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
        static const char* equipmentSlots[] = {"Weapon", "Armor", "Accessory"};
        ImGui::SeparatorText("Equipment");
        for (int slot = 0; slot < 3; ++slot) {
            const auto found = std::find_if(inventoryPanel->equipment.begin(), inventoryPanel->equipment.end(),
                [slot](const EquipmentPanelItem& item) { return item.slotType == slot; });
            ImGui::Text("%s: %s", equipmentSlots[slot],
                found == inventoryPanel->equipment.end() ? "Empty" : found->name.c_str());
        }
        ImGui::Combo("Equipment slot", &inventoryPanel->selectedEquipmentSlot,
            equipmentSlots, IM_ARRAYSIZE(equipmentSlots));
        const bool canEquip = inventoryPanel->selectedInventorySlot >= 0 &&
            static_cast<std::size_t>(inventoryPanel->selectedInventorySlot) < inventoryPanel->slots.size() &&
            !inventoryPanel->slots[static_cast<std::size_t>(inventoryPanel->selectedInventorySlot)].itemId.empty();
        if (!canEquip) ImGui::BeginDisabled();
        if (ImGui::Button("Equip") && inventoryAction &&
            !inventoryAction(InventoryAction::Equip, *inventoryPanel)) {
            inventoryPanel->message = "Unable to equip selected item.";
            inventoryPanel->messageIsError = true;
        }
        if (!canEquip) ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Unequip") && inventoryAction &&
            !inventoryAction(InventoryAction::Unequip, *inventoryPanel)) {
            inventoryPanel->message = "Unable to unequip selected slot.";
            inventoryPanel->messageIsError = true;
        }
        ImGui::SeparatorText("Character Stats");
        ImGui::Text("Attack: %d + %d = %d", inventoryPanel->baseAttack, inventoryPanel->bonusAttack,
            inventoryPanel->baseAttack + inventoryPanel->bonusAttack);
        ImGui::Text("Defense: %d + %d = %d", inventoryPanel->baseDefense, inventoryPanel->bonusDefense,
            inventoryPanel->baseDefense + inventoryPanel->bonusDefense);
        ImGui::Text("Max HP: %d + %d = %d", inventoryPanel->baseMaxHp, inventoryPanel->bonusMaxHp,
            inventoryPanel->baseMaxHp + inventoryPanel->bonusMaxHp);
        ImGui::SeparatorText("World Loot");
        if (ImGui::Button("Simulate Boss Loot Drop") && inventoryAction &&
            !inventoryAction(InventoryAction::SimulateBossLoot, *inventoryPanel)) {
            inventoryPanel->message = "Unable to request a loot drop.";
            inventoryPanel->messageIsError = true;
        }
        for (const auto& drop : inventoryPanel->drops) {
            const std::string label = drop.name + " x" + std::to_string(drop.quantity) +
                " @ " + std::to_string(drop.position.x) + ", " + std::to_string(drop.position.y) +
                ", " + std::to_string(drop.position.z);
            if (ImGui::Selectable(label.c_str(), inventoryPanel->selectedLootId == drop.lootId))
                inventoryPanel->selectedLootId = drop.lootId;
        }
        if (inventoryPanel->selectedLootId == 0) ImGui::BeginDisabled();
        if (ImGui::Button("Pick Up") && inventoryAction &&
            !inventoryAction(InventoryAction::PickupLoot, *inventoryPanel)) {
            inventoryPanel->message = "Unable to pick up selected loot.";
            inventoryPanel->messageIsError = true;
        }
        if (inventoryPanel->selectedLootId == 0) ImGui::EndDisabled();
        if (!inventoryPanel->message.empty()) {
            const ImVec4 color = inventoryPanel->messageIsError
                ? ImVec4(1.0F, 0.35F, 0.3F, 1.0F) : ImVec4(0.35F, 1.0F, 0.45F, 1.0F);
            ImGui::TextColored(color, "%s", inventoryPanel->message.c_str());
        }
        ImGui::End();
    }

    if (zonePanel) {
        ImGui::Begin("Zone & AOI");
        ImGui::Text("Current zone: %s", zonePanel->currentZone.c_str());
        ImGui::Text("Server entities: %u", zonePanel->totalEntityCount);
        ImGui::Text("Visible in AOI: %u", zonePanel->visibleEntityCount);
        if (zonePanel->characterId <= 0) ImGui::BeginDisabled();
        if (ImGui::Button("Teleport to Zone 1") && zoneChange)
            (void)zoneChange(0, *zonePanel);
        ImGui::SameLine();
        if (ImGui::Button("Teleport to Zone 2") && zoneChange)
            (void)zoneChange(1, *zonePanel);
        if (zonePanel->characterId <= 0) ImGui::EndDisabled();
        ImGui::End();
    }

    if (dungeonPanel) {
        ImGui::Begin("Dungeon Instance");
        ImGui::Text("Dungeon: %s", dungeonPanel->dungeonName.c_str());
        ImGui::TextUnformatted("Available: The Shadow Crypt");
        if (dungeonPanel->isCompleted)
            ImGui::TextColored({0.4F, 1.0F, 0.5F, 1.0F}, "DUNGEON CLEAR!");
        if (dungeonPanel->isFailed)
            ImGui::TextColored({1.0F, 0.35F, 0.3F, 1.0F}, "DUNGEON FAILED");
        if (dungeonPanel->inDungeon) {
            ImGui::Text("Instance: %llu", static_cast<unsigned long long>(dungeonPanel->instanceId));
            ImGui::Text("Time remaining: %.0f s", dungeonPanel->timerRemaining);
            ImGui::Text("Boss HP: %.0f / %.0f", dungeonPanel->bossHealth, dungeonPanel->bossMaxHealth);
            if (ImGui::Button("Leave Dungeon") && dungeonAction)
                (void)dungeonAction(DungeonAction::Leave, *dungeonPanel);
        } else {
            if (dungeonPanel->characterId <= 0) ImGui::BeginDisabled();
            if (ImGui::Button("Enter Dungeon (Solo)") && dungeonAction)
                (void)dungeonAction(DungeonAction::EnterSolo, *dungeonPanel);
            ImGui::SameLine();
            if (ImGui::Button("Enter Dungeon (Party)") && dungeonAction)
                (void)dungeonAction(DungeonAction::EnterParty, *dungeonPanel);
            if (dungeonPanel->characterId <= 0) ImGui::EndDisabled();
        }
        if (!dungeonPanel->message.empty()) ImGui::TextWrapped("%s", dungeonPanel->message.c_str());
        ImGui::End();
    }

    if (marketPanel) {
        ImGui::Begin("Marketplace / Chợ Giao Dịch");
        ImGui::Text("Gold: %lld", static_cast<long long>(marketPanel->gold));
        if (ImGui::BeginTabBar("MarketTabs")) {
            if (ImGui::BeginTabItem("Browse Market")) {
                if (marketPanel->itemType == "Mine") marketPanel->itemType.clear();
                inputString("Search", marketPanel->search);
                static const char* types[] = {"All", "Weapon", "Armor", "Accessory", "Consumable"};
                int typeIndex = marketPanel->itemType.empty() ? 0 :
                    marketPanel->itemType == "Weapon" ? 1 : marketPanel->itemType == "Armor" ? 2 :
                    marketPanel->itemType == "Accessory" ? 3 : 4;
                if (ImGui::Combo("Item type", &typeIndex, types, IM_ARRAYSIZE(types))) {
                    marketPanel->itemType = typeIndex == 0 ? "" : types[typeIndex];
                    marketPanel->page = 0;
                }
                if (ImGui::Button("Refresh") && marketAction)
                    (void)marketAction(MarketAction::Browse, *marketPanel);
                ImGui::SameLine();
                if (marketPanel->page == 0) ImGui::BeginDisabled();
                if (ImGui::Button("Previous") && marketPanel->page > 0) {
                    --marketPanel->page;
                    if (marketAction) (void)marketAction(MarketAction::Browse, *marketPanel);
                }
                if (marketPanel->page == 0) ImGui::EndDisabled();
                ImGui::SameLine();
                ImGui::Text("Page %u", marketPanel->page + 1);
                ImGui::SameLine();
                if (ImGui::Button("Next")) {
                    ++marketPanel->page;
                    if (marketAction) (void)marketAction(MarketAction::Browse, *marketPanel);
                }
                for (const auto& listing : marketPanel->listings) {
                    const std::string label = listing.itemName + " x" + std::to_string(listing.quantity) +
                        " | " + std::to_string(listing.unitPrice) + " gold ea. | " + listing.sellerName;
                    if (ImGui::Selectable(label.c_str(), marketPanel->selectedListing == listing.listingId))
                        marketPanel->selectedListing = listing.listingId;
                }
                if (marketPanel->selectedListing == 0) ImGui::BeginDisabled();
                if (ImGui::Button("Buy") && marketAction)
                    (void)marketAction(MarketAction::Buy, *marketPanel);
                if (marketPanel->selectedListing == 0) ImGui::EndDisabled();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Sell Item")) {
                if (ImGui::BeginTable("SellInventory", 3, ImGuiTableFlags_Borders)) {
                    ImGui::TableSetupColumn("Slot"); ImGui::TableSetupColumn("Item"); ImGui::TableSetupColumn("Qty");
                    ImGui::TableHeadersRow();
                    if (inventoryPanel) for (std::size_t i = 0; i < inventoryPanel->slots.size(); ++i) {
                        const auto& item = inventoryPanel->slots[i];
                        if (item.itemId.empty()) continue;
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        const std::string slotLabel = std::to_string(i + 1);
                        if (ImGui::Selectable(slotLabel.c_str(), marketPanel->selectedSlot == static_cast<int>(i)))
                            marketPanel->selectedSlot = static_cast<int>(i);
                        ImGui::TableNextColumn(); ImGui::TextUnformatted(item.name.c_str());
                        ImGui::TableNextColumn(); ImGui::Text("%d", item.quantity);
                    }
                    ImGui::EndTable();
                }
                ImGui::InputInt("Quantity", &marketPanel->quantity);
                ImGui::InputInt("Unit price", &marketPanel->unitPrice);
                if (marketPanel->selectedSlot < 0) ImGui::BeginDisabled();
                if (ImGui::Button("List on Market") && marketAction)
                    (void)marketAction(MarketAction::List, *marketPanel);
                if (marketPanel->selectedSlot < 0) ImGui::EndDisabled();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("My Listings")) {
                if (ImGui::Button("Refresh My Listings") && marketAction) {
                    marketPanel->itemType = "Mine";
                    (void)marketAction(MarketAction::Browse, *marketPanel);
                }
                for (const auto& listing : marketPanel->listings) {
                    const std::string label = listing.itemName + " x" + std::to_string(listing.quantity) +
                        " | " + std::to_string(listing.unitPrice) + " gold ea.";
                    if (ImGui::Selectable(label.c_str(), marketPanel->selectedListing == listing.listingId))
                        marketPanel->selectedListing = listing.listingId;
                }
                if (marketPanel->selectedListing == 0) ImGui::BeginDisabled();
                if (ImGui::Button("Cancel Listing") && marketAction)
                    (void)marketAction(MarketAction::Cancel, *marketPanel);
                if (marketPanel->selectedListing == 0) ImGui::EndDisabled();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        if (!marketPanel->message.empty()) ImGui::TextUnformatted(marketPanel->message.c_str());
        ImGui::End();
    }

    if (socialPanel) {
        ImGui::Begin("Social (Chat, Party, Guild)");
        const bool connectedCharacter = socialPanel->characterId > 0;
        if (ImGui::BeginTabBar("SocialTabs")) {
            if (ImGui::BeginTabItem("Chat")) {
                static const char* channels[] = {"World", "Zone", "Party", "Guild", "Whisper"};
                static const char* channelNames[] = {"World", "Zone", "Party", "Guild", "Whisper", "System"};
                ImGui::Combo("Channel", &socialPanel->selectedChannel, channels, IM_ARRAYSIZE(channels));
                inputString("Whisper target", socialPanel->targetName);
                if (ImGui::BeginChild("ChatHistory", ImVec2(0.0F, 220.0F), true)) {
                    static const ImVec4 channelColors[] = {
                        {0.45F, 0.82F, 1.0F, 1.0F}, {0.55F, 1.0F, 0.62F, 1.0F},
                        {1.0F, 0.78F, 0.35F, 1.0F}, {0.9F, 0.65F, 1.0F, 1.0F},
                        {1.0F, 0.55F, 0.58F, 1.0F}, {0.8F, 0.84F, 0.9F, 1.0F}};
                    for (const auto& line : socialPanel->chatHistory) {
                        const auto channel = std::min<std::size_t>(line.channel, IM_ARRAYSIZE(channelColors) - 1);
                        ImGui::TextColored(channelColors[channel], "[%s] %s: %s",
                            channelNames[channel], line.senderName.c_str(), line.text.c_str());
                    }
                    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 8.0F) ImGui::SetScrollHereY(1.0F);
                }
                ImGui::EndChild();
                inputString("Message", socialPanel->chatInput);
                if (!connectedCharacter) ImGui::BeginDisabled();
                if (ImGui::Button("Send") && socialAction &&
                    !socialAction(SocialAction::SendChat, *socialPanel)) socialPanel->message = "Unable to send chat.";
                if (!connectedCharacter) ImGui::EndDisabled();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Party")) {
                if (!socialPanel->partyId && !socialPanel->partyInvitePending)
                    ImGui::TextUnformatted("Not currently in a party.");
                if (socialPanel->partyInvitePending) ImGui::Text("Party invitation received (ID %llu).",
                    static_cast<unsigned long long>(socialPanel->partyId));
                for (const auto& member : socialPanel->partyMembers) {
                    ImGui::Text("%s%s | Lv. %d | %s | HP %.0f/%.0f", member.leader ? "Leader: " : "",
                        member.name.c_str(), member.level, member.className.c_str(), member.currentHp, member.maxHp);
                    ImGui::ProgressBar(member.maxHp > 0.0F ? member.currentHp / member.maxHp : 0.0F,
                        ImVec2(-1.0F, 8.0F));
                    if (ImGui::Selectable(("Select##party" + std::to_string(member.characterId)).c_str(),
                        socialPanel->selectedMemberId == member.characterId))
                        socialPanel->selectedMemberId = member.characterId;
                }
                inputString("Player name", socialPanel->targetName);
                if (!connectedCharacter) ImGui::BeginDisabled();
                if (ImGui::Button("Create Party") && socialAction)
                    (void)socialAction(SocialAction::PartyCreate, *socialPanel);
                ImGui::SameLine();
                if (ImGui::Button("Invite") && socialAction)
                    (void)socialAction(SocialAction::PartyInvite, *socialPanel);
                ImGui::SameLine();
                if (ImGui::Button("Accept Invite") && socialAction)
                    (void)socialAction(SocialAction::PartyAccept, *socialPanel);
                if (!connectedCharacter) ImGui::EndDisabled();
                if (socialPanel->partyId && ImGui::Button("Leave Party") && socialAction)
                    (void)socialAction(SocialAction::PartyLeave, *socialPanel);
                if (socialPanel->partyId && socialPanel->selectedMemberId > 0 &&
                    socialPanel->selectedMemberId != socialPanel->characterId &&
                    ImGui::Button("Kick Selected") && socialAction)
                    (void)socialAction(SocialAction::PartyKick, *socialPanel);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Guild")) {
                ImGui::Text("Guild: %s", socialPanel->guildName.empty() ? "None" : socialPanel->guildName.c_str());
                if (socialPanel->guildInvitePending) ImGui::Text("Guild invitation received (ID %llu).",
                    static_cast<unsigned long long>(socialPanel->guildId));
                for (const auto& member : socialPanel->guildMembers) {
                    static const char* ranks[] = {"Leader", "Officer", "Member"};
                    const unsigned int rank = std::min<unsigned int>(member.rank, 2);
                    if (ImGui::Selectable((member.name + " | " + ranks[rank] + " | Lv. " +
                        std::to_string(member.level) + " | " + member.className + "##guild" +
                        std::to_string(member.characterId)).c_str(), socialPanel->selectedMemberId == member.characterId))
                        socialPanel->selectedMemberId = member.characterId;
                }
                inputString("Guild name", socialPanel->guildNameInput);
                inputString("Player name", socialPanel->targetName);
                if (!connectedCharacter) ImGui::BeginDisabled();
                if (ImGui::Button("Create Guild") && socialAction)
                    (void)socialAction(SocialAction::GuildCreate, *socialPanel);
                ImGui::SameLine();
                if (ImGui::Button("Invite Member") && socialAction)
                    (void)socialAction(SocialAction::GuildInvite, *socialPanel);
                ImGui::SameLine();
                if (ImGui::Button("Accept Invite") && socialAction)
                    (void)socialAction(SocialAction::GuildAccept, *socialPanel);
                if (!connectedCharacter) ImGui::EndDisabled();
                if (socialPanel->guildId && ImGui::Button("Leave Guild") && socialAction)
                    (void)socialAction(SocialAction::GuildLeave, *socialPanel);
                static const char* ranks[] = {"Leader", "Officer", "Member"};
                int selectedRank = 1;
                ImGui::Combo("Set selected member rank", &selectedRank, ranks, IM_ARRAYSIZE(ranks));
                if (socialPanel->guildId && socialPanel->selectedMemberId > 0 &&
                    socialPanel->selectedMemberId != socialPanel->characterId &&
                    ImGui::Button("Set Rank") && socialAction) {
                    socialPanel->memberRank = static_cast<unsigned int>(selectedRank);
                    (void)socialAction(SocialAction::GuildSetRank, *socialPanel);
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        if (!socialPanel->message.empty()) ImGui::TextWrapped("%s", socialPanel->message.c_str());
        ImGui::End();
    }

    ImGui::Begin("Glory Debug");
    const float safeFrameSeconds = std::isfinite(frameSeconds) && frameSeconds > 0.0F ? frameSeconds : 0.0F;
    ImGui::Text("FPS: %.1f", safeFrameSeconds > 0.0F ? 1.0F / safeFrameSeconds : 0.0F);
    ImGui::Text("Frame time: %.3f ms", safeFrameSeconds * 1000.0F);
    ImGui::Text("Fixed ticks: %llu", static_cast<unsigned long long>(fixedTicks));
    if (player.networkConnected) {
        ImGui::SeparatorText("Authoritative Network State");
        ImGui::Text("Server tick: %llu | players: %u",
            static_cast<unsigned long long>(player.serverTick), player.serverPlayerCount);
        ImGui::Text("Client %u position: %.2f, %.2f, %.2f", player.authoritativeClientId,
            player.authoritativePosition.x, player.authoritativePosition.y, player.authoritativePosition.z);
        ImGui::Text("Authoritative HP: %.1f / %.1f", player.authoritativeHp, player.authoritativeMaxHp);
        ImGui::Text("Animation: %s | Skill: %s", player.authoritativeAnimation.c_str(),
            player.authoritativeSkill.c_str());
        ImGui::Text("Prediction: client seq %u | server ack %u | pending %u",
            player.clientInputSequence, player.serverAckSequence, player.unacknowledgedInputCount);
        ImGui::Text("Reconciliations: %llu | offset: %.3f m",
            static_cast<unsigned long long>(player.reconciliationCount), player.mispredictionOffset);
        ImGui::TextUnformatted("Remote Players (interpolated)");
        if (player.remotePlayers.empty()) ImGui::TextUnformatted("No remote players in latest snapshot.");
        for (const auto& remote : player.remotePlayers) {
            ImGui::BulletText("ID %u | pos %.2f, %.2f, %.2f | %.1f m | HP %.0f/%.0f | %s | %s",
                remote.clientId, remote.position.x, remote.position.y, remote.position.z, remote.distance,
                remote.currentHp, remote.maxHp, remote.animationState.c_str(), remote.activeSkillId.c_str());
        }
    }
    ImGui::Begin("Anti-Cheat Monitor");
    if (ImGui::BeginTabBar("AntiCheatTabs")) {
        if (ImGui::BeginTabItem("Server Monitor")) {
            ImGui::Text("Current speed: %.2f m/s", player.antiCheatCurrentSpeed);
            ImGui::Text("Maximum allowed: %.2f m/s", player.antiCheatMaximumSpeed);
            ImGui::Text("Violations | speed %u | teleport %u | collision %u | cooldown %u",
                player.speedViolations, player.teleportViolations, player.collisionViolations,
                player.cooldownViolations);
            if (!player.antiCheatMessage.empty()) ImGui::TextWrapped("Server: %s", player.antiCheatMessage.c_str());
            ImGui::SeparatorText("Cheat injection probes (server validates; position is never applied)");
            if (ImGui::Button("Inject Speed Hack (+500% speed)")) statusCommand = "anti_cheat_speed_probe";
            if (ImGui::Button("Inject Cooldown Hack (Spam skill)")) statusCommand = "anti_cheat_cooldown_probe";
            if (ImGui::Button("Inject Teleport Hack (Jump 50m)")) statusCommand = "anti_cheat_teleport_probe";
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
    ImGui::Begin("Arena Matchmaking");
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
    if (player.localTeam == 1) ImGui::TextColored(ImVec4(1.0F, 0.32F, 0.28F, 1.0F), "Your team: RED");
    else if (player.localTeam == 2) ImGui::TextColored(ImVec4(0.3F, 0.55F, 1.0F, 1.0F), "Your team: BLUE");
    if (player.matchState == 3 && player.winningTeam != 0 && player.localTeam != 0) {
        const bool victory = player.winningTeam == player.localTeam;
        ImGui::TextColored(victory ? ImVec4(0.3F, 1.0F, 0.4F, 1.0F) : ImVec4(1.0F, 0.35F, 0.3F, 1.0F),
            "%s", victory ? "VICTORY" : "DEFEAT");
    }
    ImGui::SeparatorText("Arena members");
    if (player.localTeam != 0) {
        const ImVec4 localColor = player.localTeam == 1 ? ImVec4(1.0F, 0.35F, 0.3F, 1.0F) :
            ImVec4(0.35F, 0.55F, 1.0F, 1.0F);
        ImGui::TextColored(localColor, "You | HP %.0f / %.0f", player.authoritativeHp,
            player.authoritativeMaxHp);
        ImGui::ProgressBar(player.authoritativeMaxHp > 0.0F
            ? player.authoritativeHp / player.authoritativeMaxHp : 0.0F, ImVec2(-1.0F, 0.0F));
    }
    if (player.remotePlayers.empty() && player.localTeam == 0)
        ImGui::TextUnformatted("Join a queue to find an arena match.");
    for (const auto& remote : player.remotePlayers) {
        const ImVec4 color = remote.team == 1 ? ImVec4(1.0F, 0.35F, 0.3F, 1.0F) :
            (remote.team == 2 ? ImVec4(0.35F, 0.55F, 1.0F, 1.0F) : ImVec4(0.85F, 0.85F, 0.85F, 1.0F));
        ImGui::TextColored(color, "Client %u | HP %.0f / %.0f | %s", remote.clientId,
            remote.currentHp, remote.maxHp, remote.animationState.c_str());
        ImGui::ProgressBar(remote.maxHp > 0.0F ? remote.currentHp / remote.maxHp : 0.0F,
            ImVec2(-1.0F, 0.0F));
    }
    ImGui::End();
    ImGui::SeparatorText("Camera");
    ImGui::Text("Mode: %s", thirdPersonMode ? "Third-Person" : "Free-Fly");
    ImGui::Checkbox("Third-Person Camera (F1)", &thirdPersonMode);
    ImGui::Text("Position: %.2f, %.2f, %.2f", cameraPosition.x, cameraPosition.y, cameraPosition.z);
    ImGui::Text("Yaw: %.1f  Pitch: %.1f", cameraYaw, cameraPitch);

    ImGui::SeparatorText("Player");
    ImGui::Text("Position: %.2f, %.2f, %.2f", player.position.x, player.position.y, player.position.z);
    ImGui::Text("Velocity: %.2f, %.2f, %.2f", player.velocity.x, player.velocity.y, player.velocity.z);
    ImGui::Text("Ground contact: %s", player.isGrounded ? "Yes" : "No");
    ImGui::Text("Collision contacts: %u", player.collisionCount);

    ImGui::SeparatorText("Combat");
    ImGui::Text("Boss: %s", player.bossName.c_str());
    ImGui::Text("Boss phase: %s", player.bossPhase.c_str());
    ImGui::Text("Boss HP: %.0f / %.0f (%.1f%%)", player.bossHp, player.bossMaxHp,
        player.bossMaxHp > 0.0F ? 100.0F * player.bossHp / player.bossMaxHp : 0.0F);
    const float bossHpFraction = player.bossMaxHp > 0.0F ? player.bossHp / player.bossMaxHp : 0.0F;
    ImGui::ProgressBar(bossHpFraction, ImVec2(-1.0F, 0.0F), "Boss Health");
    ImGui::Text("[BT Node: %s]", player.bossBehaviorNode.c_str());
    ImGui::Text("Player HP: %.0f / %.0f", player.playerHp, player.playerMaxHp);
    const float playerHpFraction = player.playerMaxHp > 0.0F ? player.playerHp / player.playerMaxHp : 0.0F;
    ImGui::ProgressBar(playerHpFraction, ImVec2(-1.0F, 0.0F), "Player Health");
    if (player.bossDefeated)
        ImGui::TextColored(ImVec4(0.35F, 1.0F, 0.45F, 1.0F), "VICTORY! BOSS DEFEATED");
    if (ImGui::Button("Reset Boss Fight")) resetBossRequested = true;
    ImGui::Text("Boss hits received: %u", player.dummyHitCount);
    ImGui::Text("Player stunned: %s", player.playerStunned ? "Yes" : "No");
    ImGui::TextUnformatted("Player effects");
    for (const StatusEffectDebugState& effect : player.playerEffects)
        ImGui::BulletText("%s: %.2f s (x%u)", effect.name.c_str(), effect.remainingTime, effect.stackCount);
    ImGui::TextUnformatted("Boss effects");
    for (const StatusEffectDebugState& effect : player.dummyEffects)
        ImGui::BulletText("%s: %.2f s (x%u)", effect.name.c_str(), effect.remainingTime, effect.stackCount);
    if (ImGui::Button("Player Speed +30%")) statusCommand = "player_speed";
    ImGui::SameLine();
    if (ImGui::Button("Player Attack +30%")) statusCommand = "player_attack";
    if (ImGui::Button("Dummy Burn")) statusCommand = "dummy_burn";
    ImGui::SameLine();
    if (ImGui::Button("Dummy Stun")) statusCommand = "dummy_stun";
    if (ImGui::Button("Clear All Effects")) statusCommand = "clear_effects";
    ImGui::Text("Combo step: %u", player.comboStep);
    ImGui::Text("Combo window: %s", player.comboWindowOpen ? "Open" : "Closed");
    ImGui::Text("Cancel window: %s", player.cancelWindowAvailable ? "Available" : "Unavailable");
    ImGui::Text("Hit-stop: %s (%.3f s)", player.hitStopActive ? "Active" : "Inactive",
        player.hitStopRemaining);
    if (!player.className.empty()) ImGui::Text("Class: %s", player.className.c_str());
    ImGui::Text("Dummy knockback: %.2f, %.2f, %.2f", player.dummyKnockbackVelocity.x,
        player.dummyKnockbackVelocity.y, player.dummyKnockbackVelocity.z);
    for (const SkillDebugState& skill : player.skills) {
        ImGui::Text("%s (%s): %.2f s", skill.name.c_str(), skill.id.c_str(), skill.cooldownRemaining);
        const float cooldownFraction = skill.cooldownDuration > 0.0F
            ? std::clamp(skill.cooldownRemaining / skill.cooldownDuration, 0.0F, 1.0F) : 0.0F;
        ImGui::ProgressBar(1.0F - cooldownFraction, ImVec2(180.0F, 0.0F),
            skill.cooldownRemaining > 0.0F ? "Cooling down" : "Ready");
        ImGui::SameLine();
        if (ImGui::SmallButton(("Cast##" + skill.id).c_str())) requestedSkillId = skill.id;
    }
    ImGui::Text("Dodging: %s", player.isDodging ? "Yes" : "No");
    ImGui::Text("Dodge cooldown: %.2f s", player.dodgeCooldownRemaining);
    if (player.isInvulnerable) {
        ImGui::TextColored(ImVec4(1.0F, 0.78F, 0.12F, 1.0F), "INVULNERABLE (I-FRAME)");
    } else {
        ImGui::TextUnformatted("I-Frame: inactive");
    }

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
    ImGui::Text("Blend factor: %.2f", renderer.blendFactor());
    ImGui::TextUnformatted("Animation events");
    for (const std::string& entry : animationEventLog_) ImGui::BulletText("%s", entry.c_str());
    ImGui::Text("Clip: %s", renderer.animationName().c_str());
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

    ImGui::End();

    ImGui::Begin("Skill & Event Editor");
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
            ImGui::Text("ID: %s", skill.id.c_str());
            ImGui::SeparatorText("Basic");
            inputString("Name", skill.name);
            ImGui::InputFloat("Damage", &skill.damage, 1.0F, 5.0F, "%.2f");
            ImGui::InputFloat("Cooldown", &skill.cooldown, 0.05F, 0.25F, "%.3f s");
            ImGui::InputFloat("Cast Time", &skill.castTime, 0.05F, 0.25F, "%.3f s");
            inputString("Animation State", skill.animationState);
            ImGui::SeparatorText("Combat Feel");
            inputString("Combo Next", skill.comboNext);
            ImGui::InputFloat("Combo Window", &skill.comboWindow, 0.05F, 0.25F, "%.3f s");
            ImGui::InputFloat("Input Buffer", &skill.inputBuffer, 0.05F, 0.25F, "%.3f s");
            ImGui::InputFloat("Cancel Window Start", &skill.cancelWindowStart, 0.05F, 0.25F, "%.3f s");
            ImGui::InputFloat("Hit-Stop Duration", &skill.hitStopDuration, 0.01F, 0.05F, "%.3f s");
            ImGui::InputFloat("Knockback Speed", &skill.knockbackSpeed, 0.25F, 1.0F, "%.2f");
            ImGui::InputFloat("Knockback Duration", &skill.knockbackDuration, 0.025F, 0.1F, "%.3f s");
            ImGui::SeparatorText("Hitbox");
            ImGui::InputFloat("Radius", &skill.hitboxRadius, 0.05F, 0.25F, "%.2f");
            ImGui::InputFloat3("Offset (X, Y, Z)", &skill.hitboxOffset.x, "%.2f");
            ImGui::InputFloat("Active Start", &skill.activeStart, 0.01F, 0.05F, "%.3f s");
            ImGui::InputFloat("Active End", &skill.activeEnd, 0.01F, 0.05F, "%.3f s");

            ImGui::SeparatorText("On-Hit Effects");
            if (ImGui::Button("Add Effect")) skill.onHitEffects.push_back({"new_effect", "New Effect"});
            for (std::size_t i = 0; i < skill.onHitEffects.size(); ++i) {
                SkillEditorEffect& effect = skill.onHitEffects[i];
                ImGui::PushID(static_cast<int>(i));
                inputString("ID", effect.id);
                inputString("Name", effect.name);
                const char* effectTypes[] = {"Bleed", "Burn", "Stun", "Slow", "Regen", "AttackBoost", "SpeedBoost", "ArmorBreak"};
                int effectType = 0;
                for (int type = 0; type < IM_ARRAYSIZE(effectTypes); ++type)
                    if (effect.type == effectTypes[type]) effectType = type;
                if (ImGui::Combo("Type", &effectType, effectTypes, IM_ARRAYSIZE(effectTypes))) effect.type = effectTypes[effectType];
                ImGui::InputFloat("Duration", &effect.duration, 0.1F, 0.5F, "%.2f s");
                ImGui::InputFloat("Magnitude", &effect.magnitude, 0.25F, 1.0F, "%.2f");
                ImGui::InputFloat("Tick Interval", &effect.tickInterval, 0.05F, 0.25F, "%.2f s");
                int stacks = static_cast<int>(effect.maxStacks);
                if (ImGui::InputInt("Max Stacks", &stacks)) effect.maxStacks = static_cast<unsigned int>(std::max(1, stacks));
                if (ImGui::SmallButton("Remove Effect")) {
                    skill.onHitEffects.erase(skill.onHitEffects.begin() + static_cast<std::ptrdiff_t>(i));
                    ImGui::PopID();
                    break;
                }
                ImGui::PopID();
            }

            ImGui::SeparatorText("Animation Events");
            if (ImGui::Button("Add Event")) skill.animationEvents.push_back({skill.activeStart, "sound", "swing_sound"});
            for (std::size_t i = 0; i < skill.animationEvents.size(); ++i) {
                SkillEditorEvent& event = skill.animationEvents[i];
                ImGui::PushID(static_cast<int>(i));
                ImGui::InputFloat("Event Time", &event.time, 0.01F, 0.05F, "%.3f s");
                const char* eventTypes[] = {"sound", "particle"};
                int eventType = event.type == "particle" ? 1 : 0;
                if (ImGui::Combo("Event Type", &eventType, eventTypes, IM_ARRAYSIZE(eventTypes))) event.type = eventTypes[eventType];
                inputString("Event Name", event.name);
                if (ImGui::SmallButton("Remove Event")) {
                    skill.animationEvents.erase(skill.animationEvents.begin() + static_cast<std::ptrdiff_t>(i));
                    ImGui::PopID();
                    break;
                }
                ImGui::PopID();
            }

            float timelineDuration = std::max(skill.activeEnd + skill.comboWindow, skill.castTime + skill.activeEnd);
            for (const SkillEditorEvent& event : skill.animationEvents)
                timelineDuration = std::max(timelineDuration, event.time);
            timelineDuration = std::max(timelineDuration, 0.05F);
            ImGui::SeparatorText("Timeline");
            ImGui::SliderFloat("Scrubber", &timelineCursor_, 0.0F, timelineDuration, "%.3f s");
            const ImVec2 origin = ImGui::GetCursorScreenPos();
            const float width = std::max(100.0F, ImGui::GetContentRegionAvail().x);
            const float height = 32.0F;
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            drawList->AddRectFilled(origin, ImVec2(origin.x + width, origin.y + height), IM_COL32(48, 54, 58, 255));
            const float activeX0 = origin.x + width * skill.activeStart / timelineDuration;
            const float activeX1 = origin.x + width * skill.activeEnd / timelineDuration;
            drawList->AddRectFilled(ImVec2(activeX0, origin.y), ImVec2(activeX1, origin.y + height), IM_COL32(40, 170, 105, 220));
            const float cancelX = origin.x + width * skill.cancelWindowStart / timelineDuration;
            drawList->AddLine(ImVec2(cancelX, origin.y), ImVec2(cancelX, origin.y + height), IM_COL32(255, 190, 60, 255), 2.0F);
            for (const SkillEditorEvent& event : skill.animationEvents) {
                const float eventX = origin.x + width * event.time / timelineDuration;
                drawList->AddLine(ImVec2(eventX, origin.y), ImVec2(eventX, origin.y + height),
                    event.type == "sound" ? IM_COL32(90, 170, 255, 255) : IM_COL32(255, 110, 175, 255), 2.0F);
            }
            drawList->AddText(ImVec2(origin.x, origin.y + height + 3.0F), IM_COL32_WHITE, "0.0s");
            drawList->AddText(ImVec2(origin.x + width - 30.0F, origin.y + height + 3.0F), IM_COL32_WHITE,
                (std::to_string(timelineDuration) + "s").c_str());
            ImGui::Dummy(ImVec2(width, height + 20.0F));
            ImGui::TextColored(ImVec4(0.25F, 0.85F, 0.55F, 1.0F), "Green: Hitbox active");
            ImGui::SameLine(); ImGui::TextColored(ImVec4(1.0F, 0.74F, 0.2F, 1.0F), "Yellow: Cancel window");
            ImGui::SameLine(); ImGui::TextColored(ImVec4(0.4F, 0.7F, 1.0F, 1.0F), "Blue/Pink: Events");
        }
    }
    if (ImGui::Button("Save to JSON")) {
        editorMessage_ = saveSkills ? (saveSkills(editorSkills, editorMessage_) ? "Saved skills.json" : editorMessage_)
                                    : "Save callback is unavailable";
    }
    ImGui::SameLine();
    if (ImGui::Button("Hot-Reload Skills") || ImGui::IsKeyPressed(ImGuiKey_F5)) {
        if (reloadSkills) editorMessage_ = reloadSkills(editorSkills, editorMessage_)
            ? "Skills reloaded (F5)" : editorMessage_;
        else editorMessage_ = "Reload callback is unavailable";
    }
    if (!editorMessage_.empty()) ImGui::TextWrapped("%s", editorMessage_.c_str());
    ImGui::End();

    ImGui::Begin("World & Spawner Editor");
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
    ImGui::Text("Practice dummy spawn: %.2f, %.2f, %.2f", player.practiceDummyPosition.x,
        player.practiceDummyPosition.y, player.practiceDummyPosition.z);
    ImGui::SameLine();
    ImGui::Text("Yaw %.1f deg", player.practiceDummyYaw);

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
    ImGui::End();

    ImGui::Begin("Glory Debug");

    ImGui::SeparatorText("Shadows");
    bool shadowsEnabled = renderer.shadowsEnabled();
    if (ImGui::Checkbox("Enable Shadow", &shadowsEnabled)) renderer.setShadowsEnabled(shadowsEnabled);
    float shadowBias = renderer.shadowBias();
    if (ImGui::SliderFloat("Shadow Bias", &shadowBias, 0.0001F, 0.02F, "%.4f", ImGuiSliderFlags_Logarithmic)) {
        renderer.setShadowBias(shadowBias);
    }

    ImGui::SeparatorText("PBR material");
    float metallic = renderer.metallic();
    float roughness = renderer.roughness();
    float ao = renderer.ambientOcclusion();
    bool pbrChanged = ImGui::SliderFloat("Metallic", &metallic, 0.0F, 1.0F);
    pbrChanged |= ImGui::SliderFloat("Roughness", &roughness, 0.04F, 1.0F);
    pbrChanged |= ImGui::SliderFloat("AO", &ao, 0.0F, 1.0F);
    if (pbrChanged) renderer.setPbrMaterial(metallic, roughness, ao);
    bool skyboxEnabled = renderer.skyboxEnabled();
    if (ImGui::Checkbox("Enable Skybox", &skyboxEnabled)) renderer.setSkyboxEnabled(skyboxEnabled);

    ImGui::SeparatorText("Particles and audio");
    if (ImGui::Button("Test particle burst")) (void)renderer.spawnParticleBurst({0.0F, -0.2F, 0.0F});
    ImGui::SameLine();
    ImGui::Text("Active particles: %zu", renderer.activeParticleCount());
    float masterVolume = audio.masterVolume();
    if (ImGui::SliderFloat("Master Volume", &masterVolume, 0.0F, 1.0F)) audio.setMasterVolume(masterVolume);
    bool muted = audio.muted();
    if (ImGui::Checkbox("Mute", &muted)) audio.setMuted(muted);
    ImGui::SameLine();
    ImGui::Text("Audio: %s", audio.available() ? "Ready" : "Unavailable");

    ImGui::SeparatorText("Directional light");
    static float lightYaw = -45.0F;
    static float lightPitch = -45.0F;
    const bool yawChanged = ImGui::SliderFloat("Light yaw", &lightYaw, -180.0F, 180.0F);
    const bool pitchChanged = ImGui::SliderFloat("Light pitch", &lightPitch, -89.0F, 89.0F);
    if (yawChanged || pitchChanged) {
        setDirectionFromAngles(light, lightYaw, lightPitch);
    }
    ImGui::ColorEdit3("Light color", &light.color.x);
    ImGui::SliderFloat("Ambient", &light.ambientStrength, 0.0F, 1.0F);
    ImGui::SliderFloat("Diffuse", &light.diffuseStrength, 0.0F, 2.0F);
    ImGui::SliderFloat("Specular", &light.specularStrength, 0.0F, 2.0F);
    ImGui::SliderFloat("Shininess", &light.shininess, 2.0F, 256.0F, "%.0f", ImGuiSliderFlags_Logarithmic);
    ImGui::End();

    const ImGuiIO& io = ImGui::GetIO();
    captureMouse_ = io.WantCaptureMouse;
    captureKeyboard_ = io.WantCaptureKeyboard;
}

void DebugPanel::render() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

} // namespace glory::engine
