#ifndef CLASS_RENDER_MANAGER_HEADER
#define CLASS_RENDER_MANAGER_HEADER

#include "Renderer.hpp"
#include "Button.hpp"

class ObjectManager {
public:
	ObjectManager() = delete;
	static MainWindowRenderer& GetMainRenderer() {
		static MainWindowRenderer renderer;
		return renderer;
	}

};

#endif // !CLASS_RENDER_MANAGER_HEADER
