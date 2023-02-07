#include <Engine/Core/Application.h>
#include <Engine/Core/Engine.h>
#include <Engine/Log/Log.h>
#include <Engine/Input/Input.h>
#include <Engine/Utils/Debug/Profiler.h>

namespace engine {
    class ApplicationCustomData : public InputObserver {
    public:
        ApplicationCustomData() {
            PROFILE_TIME_SCOPED(ApplicationLoading)
            LOG_TAG_LEVEL(engine::LogLevel::L_MESSAGE, ApplicationCustomData, "create");
            Engine::getInstance().getModule<Input>()->addObserver(this);
        }

        ~ApplicationCustomData() {
            Engine::getInstance().getModule<Input>()->removeObserver(this);
            LOG_TAG_LEVEL(engine::LogLevel::L_MESSAGE, ApplicationCustomData, "destroy");
        }

        bool onInputPointerEvent(const PointerEvent& event) override {
            return false;
        }

        bool onInputWheelEvent(const float dx, const float dy) override {
            return false;
        }

        bool onInpuKeyEvent(const KeyEvent& event) override {
            switch (event.key) {
                case KeyboardKey::K_ESCAPE:
                    if (event.state != InputEventState::IES_RELEASE) break;
                    Engine::getInstance().getModule<Device>()->leaveMainLoop();
                    break;
                case KeyboardKey::K_ENTER:
                    if (Engine::getInstance().getModule<Input>()->isAltPressed()) {
                        if (event.state != InputEventState::IES_RELEASE) break;
                        Engine::getInstance().getModule<Device>()->setFullscreen(!Engine::getInstance().getModule<Device>()->isFullscreen());
                    }
                    break;
                default:
                    break;
            }

            return false;
        }

        bool onInpuCharEvent(const uint16_t code) override { return false; };

        void update(const float delta) {

        }

        void draw(const float delta) {

        }

        void resize(const uint16_t w, const uint16_t h) {

        }

    private:

    };

    Application::Application() noexcept {
        _customData = new ApplicationCustomData();
    }

    void Application::freeCustomData() {
        if (_customData) {
            delete _customData;
            _customData = nullptr;
            LOG_TAG(Application, "finished");
        }
    }

    void Application::nextFrame(const float delta) {
        _customData->draw(delta);
    }

    void Application::update(const float delta) {
        _customData->update(delta);
    }

    void Application::resize(const uint16_t w, const uint16_t h) {
        if (w != 0 && h != 0) {
            _customData->resize(w, h);
        }
    }
}
