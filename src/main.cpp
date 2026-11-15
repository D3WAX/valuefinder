#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <functional>

using namespace geode::prelude;

struct Field {
    std::string name;
    std::function<float(GameObject*)> get;
    std::function<void(GameObject*, float)> set;
};

static std::vector<Field>& fields() {
    static std::vector<Field> f = {
        {"posX", [](auto o){ return o->getPositionX(); }, [](auto o, float v){ o->setPositionX(v); }},
        {"posY", [](auto o){ return o->getPositionY(); }, [](auto o, float v){ o->setPositionY(v); }},
        {"rotation", [](auto o){ return o->getRotation(); }, [](auto o, float v){ o->setRotation(v); }},
        {"scale", [](auto o){ return o->getScale(); }, [](auto o, float v){ o->setScale(v); }},
        {"trigger duration",
            [](auto o){ auto e = typeinfo_cast<EffectGameObject*>(o); return e ? e->m_duration : NAN; },
            [](auto o, float v){ if (auto e = typeinfo_cast<EffectGameObject*>(o)) e->m_duration = v; }},
        {"trigger moveX",
            [](auto o){ auto e = typeinfo_cast<EffectGameObject*>(o); return e ? e->m_moveOffset.x : NAN; },
            [](auto o, float v){ if (auto e = typeinfo_cast<EffectGameObject*>(o)) e->m_moveOffset.x = v; }},
    };
    return f;
}

struct Hit {
    Ref<GameObject> obj;
    size_t field;
};

class CheatPopup : public Popup<> {
protected:
    std::vector<Hit> m_hits;
    TextInput* m_input = nullptr;
    CCLabelBMFont* m_info = nullptr;

    bool setup() override {
        this->setTitle("Object Scanner");
        auto sz = m_mainLayer->getContentSize();

        m_input = TextInput::create(160.f, "value", "bigFont.fnt");
        m_input->setCommonFilter(CommonFilter::Float);
        m_input->setPosition({sz.width / 2, sz.height - 50.f});
        m_mainLayer->addChild(m_input);

        m_info = CCLabelBMFont::create("0 hits", "chatFont.fnt");
        m_info->setAnchorPoint({0.5f, 1.f});
        m_info->setScale(0.6f);
        m_info->setPosition({sz.width / 2, sz.height - 75.f});
        m_mainLayer->addChild(m_info);

        auto menu = CCMenu::create();
        menu->setPosition({sz.width / 2, 35.f});
        menu->setLayout(RowLayout::create()->setGap(8.f));
        menu->setContentSize({sz.width - 20.f, 40.f});
        auto add = [&](char const* t, SEL_MenuHandler cb) {
            menu->addChild(CCMenuItemSpriteExtra::create(
                ButtonSprite::create(t, "goldFont.fnt", "GJ_button_01.png", 0.7f), this, cb));
        };
        add("First", menu_selector(CheatPopup::onFirst));
        add("Next", menu_selector(CheatPopup::onNext));
        add("Set All", menu_selector(CheatPopup::onSetAll));
        add("Reset", menu_selector(CheatPopup::onReset));
        menu->updateLayout();
        m_mainLayer->addChild(menu);
        return true;
    }

    float value() { return utils::numFromString<float>(m_input->getString()).unwrapOr(0.f); }
    static bool eq(float a, float b) { return !std::isnan(a) && std::abs(a - b) < 0.01f; }

    void refresh() {
        std::string s = fmt::format("{} hits", m_hits.size());
        for (size_t i = 0; i < m_hits.size() && i < 5; i++) {
            s += fmt::format("\n{} (id {})", fields()[m_hits[i].field].name, m_hits[i].obj->m_objectID);
        }
        m_info->setString(s.c_str());
    }

    void onFirst(CCObject*) {
        auto pl = PlayLayer::get();
        if (!pl) return;
        float v = value();
        m_hits.clear();
        for (auto obj : CCArrayExt<GameObject*>(pl->m_objects)) {
            for (size_t i = 0; i < fields().size(); i++) {
                if (eq(fields()[i].get(obj), v)) m_hits.push_back({obj, i});
            }
        }
        refresh();
    }

    void onNext(CCObject*) {
        float v = value();
        std::erase_if(m_hits, [&](Hit& h) { return !eq(fields()[h.field].get(h.obj), v); });
        refresh();
    }

    void onSetAll(CCObject*) {
        float v = value();
        for (auto& h : m_hits) {
            fields()[h.field].set(h.obj, v);
            h.obj->updateOrientedBox(); // cek nama fungsi di bindings versi lu
        }
    }

    void onReset(CCObject*) { m_hits.clear(); refresh(); }

public:
    static CheatPopup* create() {
        auto ret = new CheatPopup();
        if (ret->initAnchored(300.f, 220.f)) { ret->autorelease(); return ret; }
        delete ret;
        return nullptr;
    }
};

class $modify(CheatPause, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        auto menu = CCMenu::create();
        auto btn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Scan", "goldFont.fnt", "GJ_button_04.png", 0.6f),
            this, menu_selector(CheatPause::onScan));
        menu->addChild(btn);
        menu->setPosition({50.f, 30.f});
        this->addChild(menu);
    }
    void onScan(CCObject*) {
        if (auto pl = PlayLayer::get()) pl->m_isTestMode = true; // cegah submit progress/leaderboard
        if (auto p = CheatPopup::create()) p->show();
    }
};
