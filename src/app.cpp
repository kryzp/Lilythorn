#include "app.h"

#include "graphics/backend.h"

#include "platform.h"
#include "camera.h"
#include "input/input.h"
#include "math/timer.h"
#include "math/calc.h"
#include "math/colour.h"

llt::App* llt::g_app = nullptr;

using namespace llt;

App::App(const Config& config)
	: m_config(config)
	, m_running(false)
	, m_camera(config.width, config.height, 70.0f, 0.1f, 100.0f)
	, m_renderer()
{
	g_platform = new Platform(config);
	g_vulkanBackend = new VulkanBackend(config);

	g_inputState = new Input();

	Backbuffer* backbuffer = g_vulkanBackend->createBackbuffer();
	backbuffer->setClearColour(0, Colour::black());

	g_platform->setWindowName(m_config.name);
	g_platform->setWindowSize({ m_config.width, m_config.height });
	g_platform->setWindowMode(m_config.windowMode);
	g_platform->setWindowOpacity(m_config.opacity);
	g_platform->toggleCursorVisible(!m_config.hasFlag(Config::FLAG_CURSOR_INVISIBLE));
	g_platform->toggleWindowResizable(m_config.hasFlag(Config::FLAG_RESIZABLE));

	if (m_config.hasFlag(Config::FLAG_CENTRE_WINDOW)) {
		const auto screenSize = g_platform->getScreenSize();
		g_platform->setWindowPosition({
			(int)(screenSize.x - m_config.width) / 2,
			(int)(screenSize.y - m_config.height) / 2
		});
	}

	m_running = true;

	m_camera.position = glm::vec3(0.0f, 2.75f, 3.5f);
	m_camera.setPitch(-glm::radians(30.0f));
	m_camera.update(0.0f);

	m_renderer.init(backbuffer);

	if (m_config.onInit) {
		m_config.onInit();
	}
}

App::~App()
{
	if (m_config.onDestroy) {
		m_config.onDestroy();
	}

	delete g_inputState;

	delete g_vulkanBackend;
	delete g_platform;
}

void App::run()
{
	double accumulator = 0.0;
	const double targetDeltaTime = 1.0 / static_cast<double>(m_config.targetFPS);
	
	Timer deltaTimer;
	deltaTimer.start();

	Timer elapsedTimer;
	elapsedTimer.start();

	g_platform->setCursorPosition(m_config.width / 2, m_config.height / 2);

	while (m_running)
	{
		g_platform->pollEvents();
		g_inputState->update();

		if (g_inputState->isPressed(KB_KEY_ESCAPE)) {
			exit();
		}

		double deltaTime = deltaTimer.reset();

		accumulator += CalcF::min(deltaTime, targetDeltaTime);

		while (accumulator >= targetDeltaTime)
		{
			m_camera.update(targetDeltaTime);
			g_platform->setCursorPosition(m_config.width/2, m_config.height/2);

			accumulator -= targetDeltaTime;
		}

		m_renderer.render(m_camera, deltaTime, elapsedTimer.getElapsedSeconds());

		LLT_LOG("fps: %f", 1.0 / deltaTime);
	}
}

void App::exit()
{
	m_running = false;

	if (m_config.onExit) {
		m_config.onExit();
	}
}
