#include "dev_console.hpp"


GuiDevConsole::GuiDevConsole(IEngineRuntime* runtime)
: runtime(runtime) {
    setSize(gui::perc(40), gui::perc(100));
    setStyleClasses({ "dev-console" });

    log_box = guiCreate<GuiElement>();
    log_box->setSize(gui::fill(), gui::fill());
    log_box->setStyleClasses({ "dev-console-log" });

    auto input_container = guiCreate<GuiElement>();
    input_container->setSize(gui::fill(), gui::content());
    input_container->setStyleClasses({ "input-box", "input-box-editable", "input-box-string" });
    input_container->primary_axis = GUI_PRIMARY_AXIS::X;

    auto prompt = guiCreate<GuiTextElement>(">:");
    prompt->setStyleClasses({ "dev-console-prompt" });
    input_container->pushBack(prompt);

    input_box = guiCreate<GuiTextElement>();
    input_box->setSize(gui::fill(), gui::content());
    input_box->setReadOnly(false);
    auto unichar_handler = input_box->getHandler<GuiEvt_Unichar>();
    input_box->subscribe<GuiEvt_Unichar>([this, unichar_handler](const GuiEvt_Unichar& e) {
        if (e.ch == 13) {
            executeCommand(input_box->getText());
            input_box->setContent("");
        } else {
            unichar_handler.invoke(e);
        }
    });
    auto keydown_hdl = input_box->getHandler<GuiEvt_KeyDown>();
    input_box->subscribe<GuiEvt_KeyDown>([this, keydown_hdl](const GuiEvt_KeyDown& e) {
        switch(e.vkey) {
        case VK_UP:
            if (history_cur <= 0 || history.empty()) {
                return;
            }
            --history_cur;
            input_box->setContent(history[history_cur]);
            input_box->cursorToEnd();
            return;
        case VK_DOWN:
            if (history_cur >= history.size() - 1) {
                return;
            }
            ++history_cur;
            input_box->setContent(history[history_cur]);
            input_box->cursorToEnd();
            return;
        }
        if (!keydown_hdl.invoke(e)) {
            e.consume = false;
        }
    });
    input_container->pushBack(input_box);

    pushBack(log_box);
    pushBack(input_container);

    Log::AddConsumer(this);

    ConRegistry::get()->registerFloat("r.gamma", "gamma correction", 2.2f, .0f, 100.f);
    ConRegistry::get()->registerFloat("r.exposure", "exposure", 1.0f, .0f, 100.f);
    ConRegistry::get()->registerBool("r.wire", "wireframe mode", false);

    ConRegistry::get()->registerFloat("dbg.float", "console variable test", 10.f, .0f, 100.f);
    ConRegistry::get()->registerInt("dbg.int", "console variable test", 13, 0, 100);
    ConRegistry::get()->registerBool("dbg.bool", "console variable test", true);
    ConRegistry::get()->registerFloat("dbg.float2", "console variable test", 3.14f);
    ConRegistry::get()->registerInt("dbg.int2", "console variable test", 99);
    ConRegistry::get()->registerCmd("con.clear", "clear console log", [this](const ConsoleCommand& cmd) {
        log_box->clearChildren();
    });
    ConRegistry::get()->registerCmd("con.fun", "display some ascii art", [this](const ConsoleCommand& cmd) {
        LOG(R"(++++---+++------------------++++++++++
++++-----------##------+#+++--++++++++
++---------++++##----++##+++++-+++--++
++----+++++++++----+++++++++##++++--++
++--++++-----++++++##+++---+++++++--++
++-----------++++++####+------------++
++---.-+++----+########+--+---.-----++
++----+##++----##########---+-------++
--++--+#+------########++---++--++--++
--++++--+#+++++########+++++--++++--++
--++#################++###########++++
--++--+++#+--++############+++++++++--
--++++####+-------+++######+++++++++--
--+#######++-------++++###########----
--++######++---++++++++#########++--++
----++#####++++++++++++########+++-+++
------+#####+++++++++########+++--++++
++---.--+##################++-----++++
++--------++###########++-------++++++
++++-----------++++-------------++++++)");
    });
}

GuiDevConsole::~GuiDevConsole() {
    // TODO: Log::RemoveConsumer(this);
}


void GuiDevConsole::executeCommand(const std::string& str) {
    if (!history.empty() && history.back() != str || history.empty()) {
        if (history.size() >= 32) {
            history.erase(history.begin());
        }
        history.push_back(str);
    }
    history_cur = history.size();

    ConsoleCommand cmd(str);
    ConRegistry::get()->dispatch(cmd);
}

void GuiDevConsole::focusInput() {
    guiSetFocusedWindow(input_box);
    input_box->cursorToEnd();
}
void GuiDevConsole::unfocusInput() {
    guiUnfocusWindow(input_box);
}

int GuiDevConsole::measureWidth(const std::optional<int>& width) {
    updateView();
    return GuiElement::measureWidth(width);
}
int GuiDevConsole::measureHeight(const std::optional<int>& height) {
    updateView();
    return GuiElement::measureHeight(height);
}
void GuiDevConsole::layout_2(const gui_layout_context& ctx) {
    updateView();
    GuiElement::layout_2(ctx);
}
void GuiDevConsole::consume(const LogEntry& e) {
    std::scoped_lock(entry_lock);
    if (received_entries.size() >= 256) {
        received_entries.erase(received_entries.begin());
    }
    received_entries.push_back(e);
}
void GuiDevConsole::updateView() {
    std::scoped_lock(entry_lock);
    for (int i = 0; i < received_entries.size(); ++i) {
        appendView(received_entries[i]);
    }
    received_entries.clear();
}
void GuiDevConsole::appendView(const LogEntry& e) {
    auto entry = guiCreate<GuiElement>();
    entry->setSize(gui::fill(), gui::content());
    entry->setStyleClasses({"dev-console-log-entry"});
    entry->primary_axis = GUI_PRIMARY_AXIS::X;
    switch(e.type) {
    case LOG_TYPE::LOG_INFO:
        entry->pushBack("INFO", {"dev-console-log-type", "log-message"});
        break;
    case LOG_TYPE::LOG_WARN:
        entry->pushBack("WARN", {"dev-console-log-type", "log-warning"});
        break;
    case LOG_TYPE::LOG_ERROR:
        entry->pushBack("ERR ", {"dev-console-log-type", "log-error"});
        break;
    case LOG_TYPE::LOG_DEBUG_INFO:
        entry->pushBack("DINF", {"dev-console-log-type", "log-debug"});
        break;
    case LOG_TYPE::LOG_DEBUG_WARN:
        entry->pushBack("DWRN", {"dev-console-log-type", "log-debug"});
        break;
    case LOG_TYPE::LOG_DEBUG_ERROR:
        entry->pushBack("DERR", {"dev-console-log-type", "log-debug"});
        break;
    }
    entry->pushBack(e.line)
        ->setSize(gui::fill(), gui::content());

    if (log_box->childCount() > 256) {
        log_box->popFront();
    }
    log_box->pushBack(entry);
}

