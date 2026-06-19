#pragma once

#include <mutex>
#include <vector>
#include "gui/gui.hpp"
#include "log/log.hpp"
#include "engine_runtime/engine_runtime.hpp"
#include "con_registry/con_registry.hpp"


class GuiDevConsole : public GuiElement, public LogConsumer {
    GuiElement* log_box = nullptr;
    GuiTextElement* input_box = nullptr;
    IEngineRuntime* runtime = nullptr;

    std::mutex entry_lock;
    std::vector<LogEntry> received_entries;
    std::vector<std::string> history;
    int history_cur = 0;
public:
    GuiDevConsole(IEngineRuntime* runtime);
    ~GuiDevConsole();

    void executeCommand(const std::string& str);

    void focusInput();
    void unfocusInput();

    int measureWidth(const std::optional<int>& width) override;
    int measureHeight(const std::optional<int>& height) override;
    void layout_2(const gui_layout_context& ctx);
    
    void consume(const LogEntry& e) override;
    
    void updateView();
    void appendView(const LogEntry& e);
};

