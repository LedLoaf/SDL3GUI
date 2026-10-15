#pragma once
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <algorithm>
#include <type_traits>

// ─── Theme ───────────────────────────────────────────────────────────────────
struct Theme {
    SDL_Color bg = { 30,  30,  35,  255 };
    SDL_Color fg = { 220, 220, 225, 255 };
    SDL_Color accent = { 80,  120, 200, 255 };
    SDL_Color accentHover = { 100, 140, 220, 255 };
    SDL_Color border = { 60,  60,  70,  255 };
    SDL_Color inputBg = { 40,  40,  48,  255 };
    SDL_Color tabActive = { 50,  70,  110, 255 };
    SDL_Color tabInactive = { 35,  35,  42,  255 };
    SDL_Color dialogBg = { 45,  45,  55,  255 };
    SDL_Color menuBg = { 35,  35,  45,  255 };
    int fontSize = 16;
};

// ─── Forward Declarations ────────────────────────────────────────────────────
class GUIApp;
class Container;

// ─── Base Widget ─────────────────────────────────────────────────────────────
class Widget {
public:
    SDL_Rect rect{ 0, 0, 0, 0 };       // absolute
    SDL_Rect relRect{ 0, 0, 0, 0 };    // relative to parent
    bool visible = true;
    bool enabled = true;
    bool hovered = false;
    bool focused = false;
    std::string tooltip;  // empty = no tooltip

    void setTooltip(const std::string& t) { tooltip = t; }

    virtual ~Widget() = default;
    virtual void render(SDL_Renderer* r, TTF_Font* font, const Theme& theme) {}
    virtual bool handleEvent(const SDL_Event& e, GUIApp* app) { return false; }
    virtual void update() {}

    bool contains(int x, int y) const {
        return x >= rect.x && x < rect.x + rect.w &&
            y >= rect.y && y < rect.y + rect.h;
    }
};

// ─── Container ───────────────────────────────────────────────────────────────
class Container : public Widget {
public:
    std::vector<std::unique_ptr<Widget>> children;
    std::vector<Widget*> zOrder;

    template<typename T, typename... Args>
    T* add(Args&&... args) {
        T* raw = new T(std::forward<Args>(args)...);
        children.push_back(std::unique_ptr<Widget>(raw));
        zOrder.push_back(raw);
        return raw;
    }

    void layout() {
        for (auto& c : children) {
            c->rect.x = rect.x + c->relRect.x;
            c->rect.y = rect.y + c->relRect.y;
            c->rect.w = c->relRect.w;
            c->rect.h = c->relRect.h;
            if (auto* sub = dynamic_cast<Container*>(c.get()))
                sub->layout();
        }
    }

    void render(SDL_Renderer* r, TTF_Font* font, const Theme& theme) override {
        if (!visible) return;
        for (auto* w : zOrder) {
            if (w->visible) w->render(r, font, theme);
        }
    }

    bool handleEvent(const SDL_Event& e, GUIApp* app) override {
        for (auto it = zOrder.rbegin(); it != zOrder.rend(); ++it) {
            if ((*it)->visible && (*it)->handleEvent(e, app))
                return true;
        }
        return false;
    }
};

// ─── Label ───────────────────────────────────────────────────────────────────
class Label : public Widget {
public:
    std::string text;

    Label(const std::string& txt, int x, int y, int w, int h)
        : text(txt) {
        relRect = { x, y, w, h };
    }

    void render(SDL_Renderer* r, TTF_Font* font, const Theme& theme) override {
        if (!visible) return;
        SDL_Surface* surf = TTF_RenderText_Blended(font, text.c_str(), 0, theme.fg);
        if (surf) {
            SDL_Texture* tex = SDL_CreateTextureFromSurface(r, surf);
            SDL_FRect dst{ (float)rect.x, (float)rect.y, (float)surf->w, (float)surf->h };
            SDL_RenderTexture(r, tex, nullptr, &dst);
            SDL_DestroyTexture(tex);
            SDL_DestroySurface(surf);
        }
    }
};

// ─── Button ──────────────────────────────────────────────────────────────────
class Button : public Widget {
public:
    std::string text;
    std::function<void()> onClick;

    template<typename Callback = std::nullptr_t>
    Button(const std::string& txt, int x, int y, int w, int h, Callback&& cb = std::nullptr_t{})
        : text(txt) {
        relRect = { x, y, w, h };
        if constexpr (!std::is_same_v<std::decay_t<Callback>, std::nullptr_t>) {
            onClick = std::function<void()>(std::forward<Callback>(cb));
        }
    }

    void render(SDL_Renderer* r, TTF_Font* font, const Theme& theme) override {
        if (!visible) return;
        SDL_Color col = hovered ? theme.accentHover : theme.accent;
        SDL_SetRenderDrawColor(r, col.r, col.g, col.b, col.a);
        SDL_FRect fr{ (float)rect.x, (float)rect.y, (float)rect.w, (float)rect.h };
        SDL_RenderFillRect(r, &fr);

        SDL_Color bc = theme.border;
        SDL_SetRenderDrawColor(r, bc.r, bc.g, bc.b, bc.a);
        SDL_RenderRect(r, &fr);

        SDL_Surface* surf = TTF_RenderText_Blended(font, text.c_str(), 0, theme.fg);
        if (surf) {
            SDL_Texture* tex = SDL_CreateTextureFromSurface(r, surf);
            SDL_FRect dst{
                (float)(rect.x + (rect.w - surf->w) / 2.0f),
                (float)(rect.y + (rect.h - surf->h) / 2.0f),
                (float)surf->w, (float)surf->h };
            SDL_RenderTexture(r, tex, nullptr, &dst);
            SDL_DestroyTexture(tex);
            SDL_DestroySurface(surf);
        }
    }

    bool handleEvent(const SDL_Event& e, GUIApp* app) override {
        if (!enabled || !visible) return false;
        float mx, my;
        SDL_GetMouseState(&mx, &my);
        hovered = contains((int)mx, (int)my);

        if (e.type == SDL_EVENT_MOUSE_BUTTON_UP &&
            e.button.button == SDL_BUTTON_LEFT &&
            contains((int)e.button.x, (int)e.button.y)) {
            if (onClick) onClick();
            return true;
        }
        return false;
    }
};

// ─── TextBox ─────────────────────────────────────────────────────────────────
class TextBox : public Widget {
public:
    std::string text;
    int cursorPos = 0;
    int scrollOffset = 0;
   
    TextBox(int x, int y, int w, int h) { relRect = { x, y, w, h }; }

    void render(SDL_Renderer* r, TTF_Font* font, const Theme& theme) override {
        if (!visible) return;
        SDL_Color bg = focused ? SDL_Color{ 50, 50, 65, 255 } : theme.inputBg;
        SDL_SetRenderDrawColor(r, bg.r, bg.g, bg.b, bg.a);
        SDL_FRect fr{ (float)rect.x, (float)rect.y, (float)rect.w, (float)rect.h };
        SDL_RenderFillRect(r, &fr);

        SDL_Color bc = focused ? theme.accent : theme.border;
        SDL_SetRenderDrawColor(r, bc.r, bc.g, bc.b, bc.a);
        SDL_RenderRect(r, &fr);

        std::string display = text.substr(scrollOffset);
        SDL_Surface* surf = TTF_RenderText_Blended(font, display.c_str(), 0, theme.fg);
        if (surf) {
            SDL_Texture* tex = SDL_CreateTextureFromSurface(r, surf);
            SDL_FRect dst{ (float)(rect.x + 4),
                          (float)(rect.y + (rect.h - surf->h) / 2.0f),
                          (float)surf->w, (float)surf->h };
            SDL_RenderTexture(r, tex, nullptr, &dst);
            SDL_DestroyTexture(tex);
            SDL_DestroySurface(surf);
        }

        if (focused) {
            int textW = 0, textH = 0;
            TTF_GetStringSize(font, text.c_str(), 0, &textW, &textH);
            int cx = rect.x + 4 + textW - scrollOffset;
            SDL_SetRenderDrawColor(r, theme.fg.r, theme.fg.g, theme.fg.b, theme.fg.a);
            SDL_RenderLine(r, cx, rect.y + 4, cx, rect.y + rect.h - 4);
        }
    }

    bool handleEvent(const SDL_Event& e, GUIApp* app) override {
        if (!visible) return false;
        if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
            e.button.button == SDL_BUTTON_LEFT &&
            contains((int)e.button.x, (int)e.button.y)) {
            focused = true;
            return true;
        }
        // ... rest unchanged

        if (e.type == SDL_EVENT_KEY_DOWN && focused) {
            if (e.key.key == SDLK_BACKSPACE && !text.empty()) {
                text.pop_back();
                if (cursorPos > 0) cursorPos--;
            }
            else if (e.key.key == SDLK_LEFT) {
                if (cursorPos > 0) cursorPos--;
            }
            else if (e.key.key == SDLK_RIGHT) {
                if (cursorPos < (int)text.size()) cursorPos++;
            }
            return true;
        }
        if (e.type == SDL_EVENT_TEXT_INPUT && focused) {
            text += e.text.text;
            return true;
        }
        return false;
    }
};

// ─── ScrollBox ───────────────────────────────────────────────────────────────
class ScrollBox : public Widget {
public:
    std::vector<std::unique_ptr<Widget>> items;
    float scrollY = 0.0f;
    int viewportH = 0;
    int contentH = 0;
    int selectedIndex = -1;          // -1 = nothing selected
    bool dragging = false;
    int dragOffset = 0;
    std::function<void(const std::string&)> onSelect;  // fires on selection change

    ScrollBox(int x, int y, int w, int h) : viewportH(h) { relRect = { x, y, w, h }; }

    template<typename T, typename... Args>
    T* addItem(Args&&... args) {
        T* raw = new T(std::forward<Args>(args)...);
        items.push_back(std::unique_ptr<Widget>(raw));
        return raw;
    }

    void addNewItem(const std::string& text) {
        int y = contentH;
        items.push_back(std::unique_ptr<Widget>(new Label(text, 4, y, relRect.w - 14, 28)));
        update();
    }

    std::string getSelectedText() const {
        if (selectedIndex < 0 || selectedIndex >= (int)items.size()) return "";
        auto* lbl = dynamic_cast<Label*>(items[selectedIndex].get());
        return lbl ? lbl->text : "";
    }

    void update() override {
        contentH = 0;
        for (auto& item : items) {
            item->relRect.x = 2;
            item->relRect.y = contentH;
            contentH += item->relRect.h;
        }
    }

    void render(SDL_Renderer* r, TTF_Font* font, const Theme& theme) override {
        if (!visible) return;
        SDL_Rect clip = rect;
        SDL_SetRenderClipRect(r, &clip);

        SDL_Color bg = theme.inputBg;
        SDL_SetRenderDrawColor(r, bg.r, bg.g, bg.b, bg.a);
        SDL_FRect fr{ (float)rect.x, (float)rect.y, (float)rect.w, (float)rect.h };
        SDL_RenderFillRect(r, &fr);

        for (int i = 0; i < (int)items.size(); ++i) {
            auto& item = items[i];
            int drawY = item->relRect.y - (int)scrollY;
            if (drawY + item->relRect.h > 0 && drawY < viewportH) {
                // Highlight selected item
                if (i == selectedIndex) {
                    SDL_SetRenderDrawColor(r, theme.accent.r, theme.accent.g, theme.accent.b, 255);
                    SDL_FRect hl{ (float)rect.x, (float)(rect.y + drawY),
                                 (float)(rect.w - 12), (float)item->relRect.h };
                    SDL_RenderFillRect(r, &hl);
                }
                int savedX = item->rect.x, savedY = item->rect.y;
                item->rect.x = rect.x + item->relRect.x;
                item->rect.y = rect.y + drawY;
                item->render(r, font, theme);
                item->rect.x = savedX;
                item->rect.y = savedY;
            }
        }

        SDL_SetRenderClipRect(r, nullptr);

        // Scrollbar
        if (contentH > viewportH) {
            int sbW = 10;
            int sbX = rect.x + rect.w - sbW;
            int trackH = viewportH;
            int thumbH = std::max(20, (int)((float)viewportH / contentH * viewportH));
            int thumbY = rect.y + (int)((scrollY / (contentH - viewportH)) * (trackH - thumbH));

            SDL_SetRenderDrawColor(r, 50, 50, 60, 255);
            SDL_FRect track{ (float)sbX, (float)rect.y, (float)sbW, (float)trackH };
            SDL_RenderFillRect(r, &track);

            SDL_SetRenderDrawColor(r, 100, 100, 120, 255);
            SDL_FRect thumb{ (float)sbX, (float)thumbY, (float)sbW, (float)thumbH };
            SDL_RenderFillRect(r, &thumb);
        }
    }

    bool handleEvent(const SDL_Event& e, GUIApp* app) override {
        if (!visible) return false;

        // Wheel
        if (e.type == SDL_EVENT_MOUSE_WHEEL &&
            contains((int)e.wheel.mouse_x, (int)e.wheel.mouse_y)) {
            scrollY += e.wheel.y * 40.0f;
            float maxScroll = std::max(0.0f, (float)contentH - viewportH);
            scrollY = std::clamp(scrollY, 0.0f, maxScroll);
            return true;
        }

        // Scrollbar drag
        if (contentH > viewportH) {
            int sbW = 10;
            int sbX = rect.x + rect.w - sbW;
            int trackH = viewportH;
            int thumbH = std::max(20, (int)((float)viewportH / contentH * viewportH));
            int thumbY = rect.y + (int)((scrollY / (contentH - viewportH)) * (trackH - thumbH));
            SDL_Rect thumbRect{ sbX, thumbY, sbW, thumbH };
            SDL_Rect trackRect{ sbX, rect.y, sbW, trackH };

            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT) {
                SDL_Point pt{ (int)e.button.x, (int)e.button.y };
                if (SDL_PointInRect(&pt, &trackRect)) {
                    dragging = true;
                    dragOffset = (int)e.button.y - thumbY;
                    return true;
                }
            }
            if (e.type == SDL_EVENT_MOUSE_BUTTON_UP && dragging) {
                dragging = false;
                return true;
            }
            if (dragging && e.type == SDL_EVENT_MOUSE_MOTION) {
                int newThumbY = (int)e.motion.y - dragOffset;
                float ratio = (float)(newThumbY - rect.y) / (float)(trackH - thumbH);
                scrollY = ratio * (float)(contentH - viewportH);
                scrollY = std::clamp(scrollY, 0.0f, std::max(0.0f, (float)contentH - viewportH));
                return true;
            }
        }

        // Item click (selection)
        if (e.type == SDL_EVENT_MOUSE_BUTTON_UP && e.button.button == SDL_BUTTON_LEFT) {
            int mx = (int)e.button.x, my = (int)e.button.y;
            if (contains(mx, my) && mx < rect.x + rect.w - 10) { // not on scrollbar
                for (int i = 0; i < (int)items.size(); ++i) {
                    int drawY = items[i]->relRect.y - (int)scrollY;
                    SDL_Rect itemRect{ rect.x, rect.y + drawY, rect.w - 12, items[i]->relRect.h };
                    SDL_Point pt{ mx, my };
                    if (SDL_PointInRect(&pt, &itemRect)) {
                        int prev = selectedIndex;
                        selectedIndex = i;
                        if (onSelect && prev != selectedIndex)
                            onSelect(getSelectedText());
                        return true;
                    }
                }
            }
        }

        for (auto& item : items) {
            if (item->handleEvent(e, app)) return true;
        }
        return false;
    }
};

// ─── Toolbar ─────────────────────────────────────────────────────────────────
class Toolbar : public Container {
public:
    int itemSpacing = 4;
    int itemH = 32;

    Toolbar(int x, int y, int w, int h) { relRect = { x, y, w, h }; }

    template<typename T, typename... Args>
    T* addTool(Args&&... args) {
        int nextX = rect.x + itemSpacing;
        for (auto& c : children) {
            nextX = c->rect.x + c->rect.w + itemSpacing;
        }
        T* raw = new T(std::forward<Args>(args)...);
        raw->relRect = { nextX - rect.x, 2, raw->relRect.w, itemH };
        children.push_back(std::unique_ptr<Widget>(raw));
        zOrder.push_back(raw);
        return raw;
    }

    void render(SDL_Renderer* r, TTF_Font* font, const Theme& theme) override {
        if (!visible) return;
        SDL_Color bg = { 40, 40, 50, 255 };
        SDL_SetRenderDrawColor(r, bg.r, bg.g, bg.b, bg.a);
        SDL_FRect fr{ (float)rect.x, (float)rect.y, (float)rect.w, (float)rect.h };
        SDL_RenderFillRect(r, &fr);
        SDL_SetRenderDrawColor(r, theme.border.r, theme.border.g, theme.border.b, theme.border.a);
        SDL_RenderLine(r, rect.x, rect.y + rect.h - 1, rect.x + rect.w, rect.y + rect.h - 1);
        Container::render(r, font, theme);
    }
};

// ─── TabPanel ────────────────────────────────────────────────────────────────
class TabPanel : public Widget {
public:
    struct Tab {
        std::string name;
        std::unique_ptr<Container> content;
    };
    std::vector<Tab> tabs;
    int activeTab = 0;
    int tabH = 28;

    TabPanel(int x, int y, int w, int h) { relRect = { x, y, w, h }; }

    Container* addTab(const std::string& name) {
        Tab tab;
        tab.name = name;
        tab.content = std::make_unique<Container>();
        tab.content->relRect = { 2, tabH, rect.w - 4, rect.h - tabH - 2 };
        tabs.push_back(std::move(tab));
        return tabs.back().content.get();
    }



    void render(SDL_Renderer* r, TTF_Font* font, const Theme& theme) override {
        if (!visible) return;
        int tx = rect.x;
        for (int i = 0; i < (int)tabs.size(); ++i) {
            int tw = 60 + (int)tabs[i].name.size() * 7;
            SDL_Color col = (i == activeTab) ? theme.tabActive : theme.tabInactive;
            SDL_SetRenderDrawColor(r, col.r, col.g, col.b, col.a);
            SDL_FRect tr{ (float)tx, (float)rect.y, (float)tw, (float)tabH };
            SDL_RenderFillRect(r, &tr);

            SDL_Surface* surf = TTF_RenderText_Blended(font, tabs[i].name.c_str(), 0, theme.fg);
            if (surf) {
                SDL_Texture* tex = SDL_CreateTextureFromSurface(r, surf);
                SDL_FRect dst{ (float)(tx + 8), (float)(rect.y + 5), (float)surf->w, (float)surf->h };
                SDL_RenderTexture(r, tex, nullptr, &dst);
                SDL_DestroyTexture(tex);
                SDL_DestroySurface(surf);
            }
            tx += tw;
        }

        if (activeTab < (int)tabs.size()) {
            auto* content = tabs[activeTab].content.get();
            content->rect.x = rect.x + content->relRect.x;
            content->rect.y = rect.y + content->relRect.y;
            content->rect.w = content->relRect.w;
            content->rect.h = content->relRect.h;
            content->layout();
            content->render(r, font, theme);
        }
    }

    bool handleEvent(const SDL_Event& e, GUIApp* app) override {
        if (!visible) return false;
        if (e.type == SDL_EVENT_MOUSE_BUTTON_UP && e.button.button == SDL_BUTTON_LEFT) {
            int tx = rect.x;
            for (int i = 0; i < (int)tabs.size(); ++i) {
                int tw = 60 + (int)tabs[i].name.size() * 7;
                SDL_Rect tabRect{ tx, rect.y, tw, tabH };
                SDL_Point pt{ (int)e.button.x, (int)e.button.y };
                if (SDL_PointInRect(&pt, &tabRect)) {
                    activeTab = i;
                    return true;
                }
                tx += tw;
            }
        }
        if (activeTab < (int)tabs.size()) {
            auto* content = tabs[activeTab].content.get();
            content->rect.x = rect.x + content->relRect.x;
            content->rect.y = rect.y + content->relRect.y;
            content->rect.w = content->relRect.w;
            content->rect.h = content->relRect.h;
            content->layout();
            return content->handleEvent(e, app);
        }
        return false;
    }
};

// ─── Menu ────────────────────────────────────────────────────────────────────
class Menu : public Widget {
public:
    struct MenuItem {
        std::string text;
        std::function<void()> action;
    };
    std::string title;
    std::vector<MenuItem> items;
    bool open = false;
    int itemH = 24;

    Menu(const std::string& title, int x, int y, int w, int h)
        : title(title) {
        relRect = { x, y, w, h };
    }

    void addItem(const std::string& text, std::function<void()> action = nullptr) {
        items.push_back({ text, std::move(action) });
    }

    int menuWidth() const {
        int w = rect.w;
        for (auto& item : items)
            if ((int)item.text.size() * 8 > w) w = (int)item.text.size() * 8;
        return w + 16;
    }

    int menuHeight() const {
        return itemH * (int)items.size() + 4;
    }

    void render(SDL_Renderer* r, TTF_Font* font, const Theme& theme) override {
        if (!visible) return;

        // Button
        SDL_Color col = open ? theme.accentHover : theme.accent;
        SDL_SetRenderDrawColor(r, col.r, col.g, col.b, col.a);
        SDL_FRect fr{ (float)rect.x, (float)rect.y, (float)rect.w, (float)rect.h };
        SDL_RenderFillRect(r, &fr);

        SDL_Surface* surf = TTF_RenderText_Blended(font, title.c_str(), 0, theme.fg);
        if (surf) {
            SDL_Texture* tex = SDL_CreateTextureFromSurface(r, surf);
            SDL_FRect dst{ (float)(rect.x + 8), (float)(rect.y + 4), (float)surf->w, (float)surf->h };
            SDL_RenderTexture(r, tex, nullptr, &dst);
            SDL_DestroyTexture(tex);
            SDL_DestroySurface(surf);
        }

        // Dropdown (rendered as overlay on main window)
        if (open) {
            int mw = menuWidth();
            int mh = menuHeight();
            int mx = rect.x;
            int my = rect.y + rect.h;

            // Background
            SDL_SetRenderDrawColor(r, theme.menuBg.r, theme.menuBg.g, theme.menuBg.b, 255);
            SDL_FRect drop{ (float)mx, (float)my, (float)mw, (float)mh };
            SDL_RenderFillRect(r, &drop);
            SDL_SetRenderDrawColor(r, theme.border.r, theme.border.g, theme.border.b, 255);
            SDL_RenderRect(r, &drop);

            // Items
            for (int i = 0; i < (int)items.size(); ++i) {
                int iy = my + 2 + i * itemH;
                // Hover highlight
                float pmx, pmy;
                SDL_GetMouseState(&pmx, &pmy);
                if ((int)pmx >= mx && (int)pmx < mx + mw &&
                    (int)pmy >= iy && (int)pmy < iy + itemH) {
                    SDL_SetRenderDrawColor(r, theme.accent.r, theme.accent.g, theme.accent.b, 255);
                    SDL_FRect hl{ (float)mx, (float)iy, (float)mw, (float)itemH };
                    SDL_RenderFillRect(r, &hl);
                }
                SDL_Surface* isurf = TTF_RenderText_Blended(font, items[i].text.c_str(), 0, theme.fg);
                if (isurf) {
                    SDL_Texture* itex = SDL_CreateTextureFromSurface(r, isurf);
                    SDL_FRect idst{ 10.0f + (float)mx, (float)(iy + 4), (float)isurf->w, (float)isurf->h };
                    SDL_RenderTexture(r, itex, nullptr, &idst);
                    SDL_DestroyTexture(itex);
                    SDL_DestroySurface(isurf);
                }
            }
        }
    }

    bool handleEvent(const SDL_Event& e, GUIApp* app) override {
        if (!visible) return false;
        if (e.type == SDL_EVENT_MOUSE_BUTTON_UP && e.button.button == SDL_BUTTON_LEFT) {
            int mx = (int)e.button.x, my = (int)e.button.y;

            // Click on button toggles
            if (contains(mx, my)) {
                open = !open;
                return true;
            }

            // Click on dropdown item
            if (open) {
                int mw = menuWidth();
                int dropY = rect.y + rect.h;
                if (mx >= rect.x && mx < rect.x + mw &&
                    my >= dropY && my < dropY + menuHeight()) {
                    int idx = (my - dropY - 2) / itemH;
                    if (idx >= 0 && idx < (int)items.size()) {
                        if (items[idx].action) items[idx].action();
                        open = false;
                        return true;
                    }
                }
                // Click outside closes
                open = false;
                return true;
            }
        }
        return false;
    }
};

// ─── Dialog ──────────────────────────────────────────────────────────────────
class Dialog : public Container {
public:
    std::string title;
    bool modal = true;
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    bool open = false;

    Dialog(const std::string& title, int w, int h, bool modal = true)
        : title(title), modal(modal)
    {
        relRect = { 0, 0, w, h };
        rect = { 0, 0, w, h };
    }

    void openDialog(SDL_Window* parent) {
        if (!window) {
            int px, py, pw, ph;
            SDL_GetWindowPosition(parent, &px, &py);
            SDL_GetWindowSize(parent, &pw, &ph);
            int dx = px + (pw - rect.w) / 2;
            int dy = py + (ph - rect.h) / 2;

            window = SDL_CreateWindow(title.c_str(), rect.w, rect.h,
                SDL_WINDOW_UTILITY | SDL_WINDOW_RESIZABLE);
            SDL_SetWindowPosition(window, dx, dy);
            if (modal) {
                SDL_SetWindowModal(window, parent);
            }
            renderer = SDL_CreateRenderer(window, nullptr);
        }
        open = true;
        SDL_ShowWindow(window);
    }

    void closeDialog() {
        open = false;
        if (window) SDL_HideWindow(window);
    }

    void render() {
        if (!open || !renderer) return;
        SDL_Log("Dialog rect: %d,%d %dx%d", rect.x, rect.y, rect.w, rect.h);
        layout();  // ← THIS LINE computes children's rect from relRect

        SDL_SetRenderDrawColor(renderer, 45, 45, 55, 255);
        SDL_RenderClear(renderer);

        TTF_Font* font = TTF_OpenFont("C:/Windows/Fonts/arial.ttf", 16.0f);
        if (font) {
            SDL_SetRenderDrawColor(renderer, 30, 30, 40, 255);
            SDL_FRect titleBar{ 0, 0, (float)rect.w, 30.0f };
            SDL_RenderFillRect(renderer, &titleBar);
            SDL_Surface* surf = TTF_RenderText_Blended(font, title.c_str(), 0,
                SDL_Color{ 220, 220, 225, 255 });
            if (surf) {
                SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
                SDL_FRect dst{ 10.0f, 5.0f, (float)surf->w, (float)surf->h };
                SDL_RenderTexture(renderer, tex, nullptr, &dst);
                SDL_DestroyTexture(tex);
                SDL_DestroySurface(surf);
            }

            Theme theme;
            for (auto* w : zOrder) {
                if (w->visible) w->render(renderer, font, theme);
            }
            TTF_CloseFont(font);
        }
        SDL_RenderPresent(renderer);
    }

    bool handleEvent(const SDL_Event& e) {
        if (!open) return false;
        if (e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
            window && e.window.windowID == SDL_GetWindowID(window)) {
            closeDialog();
            return true;
        }
        for (auto it = zOrder.rbegin(); it != zOrder.rend(); ++it) {
            if ((*it)->handleEvent(e, nullptr)) return true;
        }
        return false;
    }

    ~Dialog() {
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
    }
};

// ─── GUIApp ──────────────────────────────────────────────────────────────────
class GUIApp {
public:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    TTF_Font* font = nullptr;
    Theme theme;
    bool running = false;
    Widget* focusedWidget = nullptr;
    Widget* hoveredWidget = nullptr;
    int hoverFrame = 0;
    static constexpr int TOOLTIP_DELAY = 45; // ~0.75s at 60fps

    std::unique_ptr<Container> root;
    std::vector<std::unique_ptr<Dialog>> dialogs;

    bool init(const char* title, int w, int h) {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            SDL_Log("SDL_Init failed: %s", SDL_GetError());
            return false;
        }
        if (!TTF_Init()) {
            SDL_Log("TTF_Init failed: %s", SDL_GetError());
            return false;
        }
        window = SDL_CreateWindow(title, w, h, SDL_WINDOW_RESIZABLE);
        if (!window) return false;
        renderer = SDL_CreateRenderer(window, nullptr);
        if (!renderer) return false;

        SDL_StartTextInput(window);

        font = TTF_OpenFont("C:/Windows/Fonts/arial.ttf", (float)theme.fontSize);
        if (!font) {
            SDL_Log("Failed to load font: %s", SDL_GetError());
            return false;
        }

        root = std::make_unique<Container>();
        root->rect = { 0, 0, w, h };
        root->relRect = { 0, 0, w, h };
        running = true;
        return true;
    }

    Dialog* createDialog(const std::string& title, int w, int h, bool modal = true) {
        dialogs.push_back(std::make_unique<Dialog>(title, w, h, modal));
        return dialogs.back().get();
    }

    void run() {
        while (running) {
            SDL_Event e;
            while (SDL_PollEvent(&e)) {
                handleEvent(e);
            }
            render();
            SDL_Delay(16);
        }
    }

    void findFocusedWidget(Widget* w) {
        if (auto* tb = dynamic_cast<TextBox*>(w)) {
            if (tb->focused) {
                focusedWidget = tb;
                return;
            }
        }
        if (auto* c = dynamic_cast<Container*>(w)) {
            for (auto& child : c->children)
                findFocusedWidget(child.get());
        }
        if (auto* tp = dynamic_cast<TabPanel*>(w)) {
            for (auto& tab : tp->tabs)
                findFocusedWidget(tab.content.get());
        }
        if (auto* sb = dynamic_cast<ScrollBox*>(w)) {
            for (auto& item : sb->items)
                findFocusedWidget(item.get());
        }
    }

    void clearAllFocus(Widget* w) {
        if (auto* tb = dynamic_cast<TextBox*>(w))
            tb->focused = false;
        if (auto* c = dynamic_cast<Container*>(w))
            for (auto& child : c->children)
                clearAllFocus(child.get());
        if (auto* tp = dynamic_cast<TabPanel*>(w))
            for (auto& tab : tp->tabs)
                clearAllFocus(tab.content.get());
        if (auto* sb = dynamic_cast<ScrollBox*>(w))
            for (auto& item : sb->items)
                clearAllFocus(item.get());
    }


    void handleEvent(const SDL_Event& e) {
        if (e.type == SDL_EVENT_QUIT) { running = false; return; }
        if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) { running = false; return; }

        root->layout();

        if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            clearAllFocus(root.get());  // ← clear ALL textboxes
            focusedWidget = nullptr;
        }

        for (auto& d : dialogs) {
            if (d->open && d->handleEvent(e)) return;
        }
        if (root) root->handleEvent(e, this);

        // After dispatch, find which textbox is now focused
        if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            findFocused(root.get());
        }
    }

    void findFocused(Widget* w) {
        if (auto* tb = dynamic_cast<TextBox*>(w)) {
            if (tb->focused) { focusedWidget = tb; return; }
        }
        if (auto* c = dynamic_cast<Container*>(w))
            for (auto& child : c->children)
                findFocused(child.get());
        if (auto* tp = dynamic_cast<TabPanel*>(w))
            for (auto& tab : tp->tabs)
                findFocused(tab.content.get());
        if (auto* sb = dynamic_cast<ScrollBox*>(w))
            for (auto& item : sb->items)
                findFocused(item.get());
    }

    Widget* findHovered(Widget* w) {
        float mx, my;
        SDL_GetMouseState(&mx, &my);
        if (!w->visible) return nullptr;
        if (w->contains((int)mx, (int)my) && !w->tooltip.empty())
            return w;
        if (auto* c = dynamic_cast<Container*>(w)) {
            for (auto& child : c->children) {
                auto* r = findHovered(child.get());
                if (r) return r;
            }
        }
        if (auto* tp = dynamic_cast<TabPanel*>(w)) {
            for (auto& tab : tp->tabs) {
                auto* r = findHovered(tab.content.get());
                if (r) return r;
            }
        }
        if (auto* sb = dynamic_cast<ScrollBox*>(w)) {
            for (auto& item : sb->items) {
                auto* r = findHovered(item.get());
                if (r) return r;
            }
        }
        return nullptr;
    }

    void render() {
        SDL_SetRenderDrawColor(renderer, theme.bg.r, theme.bg.g, theme.bg.b, theme.bg.a);
        SDL_RenderClear(renderer);
        if (root) {
            root->layout();
            root->render(renderer, font, theme);
        }

        // ─── Tooltip ───
        hoveredWidget = findHovered(root.get());
        if (hoveredWidget) {
            hoverFrame++;
        }
        else {
            hoverFrame = 0;
        }

        if (hoveredWidget && hoverFrame >= TOOLTIP_DELAY) {
            float mx, my;
            SDL_GetMouseState(&mx, &my);
            int textW = 0, textH = 0;
            TTF_GetStringSize(font, hoveredWidget->tooltip.c_str(), 0, &textW, &textH);
            int pad = 6;
            int tw = textW + pad * 2;
            int th = textH + pad * 2;
            int tx = (int)mx + 12;
            int ty = (int)my + 12;
            // Keep on screen
            int winW, winH;
            SDL_GetWindowSize(window, &winW, &winH);
            if (tx + tw > winW) tx = (int)mx - tw - 4;
            if (ty + th > winH) ty = (int)my - th - 4;

            // Background
            SDL_SetRenderDrawColor(renderer, 45, 45, 55, 230);
            SDL_FRect bg{ (float)tx, (float)ty, (float)tw, (float)th };
            SDL_RenderFillRect(renderer, &bg);
            SDL_SetRenderDrawColor(renderer, 100, 100, 120, 255);
            SDL_RenderRect(renderer, &bg);

            // Text
            SDL_Surface* surf = TTF_RenderText_Blended(font, hoveredWidget->tooltip.c_str(), 0, theme.fg);
            if (surf) {
                SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
                SDL_FRect dst{ (float)(tx + pad), (float)(ty + pad), (float)surf->w, (float)surf->h };
                SDL_RenderTexture(renderer, tex, nullptr, &dst);
                SDL_DestroyTexture(tex);
                SDL_DestroySurface(surf);
            }
        }

        SDL_RenderPresent(renderer);

        for (auto& d : dialogs) {
            if (d->open) d->render();
        }
    }

    void quit() { running = false; }

    ~GUIApp() {
        if (font) TTF_CloseFont(font);
        TTF_Quit();
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        SDL_Quit();
    }
};