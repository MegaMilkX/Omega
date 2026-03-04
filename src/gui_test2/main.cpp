#include "gui_test2_reflect.auto.hpp"

#include "platform/platform.hpp"
#include "gui/gui.hpp"
#include "gpu/gpu.hpp"
#include "resource_manager/resource_manager.hpp"


int main() {
	cppiReflectInit();

	platformInit(true, true);
	gpuInit();
	std::shared_ptr<Font> fnt = fontGet("fonts/ProggyClean.ttf", 16, 72);
	guiInit(fnt);

	auto e = new GuiDemoWindow();
	e->setSize(gui::px(400), gui::px(600));
	guiGetRoot()->pushBack(e);
	
	while (platformIsRunning()) {
		platformPollMessages();

		guiPollMessages();
		guiLayout();
		guiDraw();
		guiRender();
		platformSwapBuffers();
	}

	guiCleanup();
	gpuCleanup();
	platformCleanup();
	return 0;
}