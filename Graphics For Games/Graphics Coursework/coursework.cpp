#pragma comment(lib, "nclgl.lib")

#include "../../NCLGL/Window.h"
#include "Renderer.h"

int main() {
	Window w("Course Work!", 1280,720,false); //This is all boring win32 window creation stuff!
	if(!w.HasInitialised()) {
		return -1;
	}

	srand((unsigned int)w.GetTimer()->GetMS() * 1000.0f);
	
	Renderer renderer(w); //This handles all the boring OGL 3.2 initialisation stuff, and sets up our tutorial!
	if(!renderer.HasInitialised()) {
		return -1;
	}

	w.LockMouseToWindow(true);
	w.ShowOSPointer(false);

	while(w.UpdateWindow() && !Window::GetKeyboard()->KeyDown(KEYBOARD_ESCAPE)){
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_1)) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_2)) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
		renderer.UpdateScene(w.GetTimer()->GetTimedMS());
		renderer.RenderScene();
	}

	return 0;
}