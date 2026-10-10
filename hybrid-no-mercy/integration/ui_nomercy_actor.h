#pragma once
#include <RmlUi/Core.h>
namespace recompui {
class ElementNoMercyActor : public Rml::Element {
public:
    explicit ElementNoMercyActor(const Rml::String& tag);
protected:
    void OnRender() override;
};
void register_nomercy_actor_images();
}
