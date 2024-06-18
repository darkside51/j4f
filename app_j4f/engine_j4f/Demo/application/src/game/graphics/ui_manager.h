#pragma once

#include <algorithm>
#include <memory>
#include <vector>

namespace game {

    class UIModule {
    public:
        ~UIModule() = default;
        virtual void draw(const float delta) = 0;

    private:
    };

    class UIManager {
    public:
        void draw(const float delta) {
            for (auto& module : _uiModules) {
                module->draw(delta);
            }
        };

        void registerUIModule(std::unique_ptr<UIModule>&& module) {
            _uiModules.emplace_back(std::move(module));
        }

        void unregisterUIModule(UIModule * module) {
            _uiModules.erase(std::remove_if(_uiModules.begin(), _uiModules.end(), [module](auto const & value) { return value.get() == module; }),
                _uiModules.end());
        }

    private:
        std::vector<std::unique_ptr<UIModule>> _uiModules;
    };
}