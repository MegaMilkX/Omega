#include "gui_test2_reflect.auto.hpp"

#include "platform/platform.hpp"
#include "xui/xui.hpp"
#include "xui/renderer/gl_renderer.hpp"
#include "gpu/gpu.hpp"
#include "resource_manager/resource_manager.hpp"


int main() {
	cppiReflectInit();

	platformInit(true, true);
	gpuInit();

	std::unique_ptr<xui::Host> uihost(new xui::Host);
	std::unique_ptr<xui::IRenderer> uirenderer(new xui::GLRenderer);
	platformSetXuiHost(uihost.get());

	while (platformIsRunning()) {
		platformPollMessages();

		int width, height;
		platformGetWindowSize(width, height);

		//guiPollMessages();
		uihost->layout(width, height);
		uihost->draw(uirenderer.get());
		uihost->render(uirenderer.get(), width, height, true);

		platformSwapBuffers();
	}

	uihost.reset();
	gpuCleanup();
	platformCleanup();
	return 0;
}