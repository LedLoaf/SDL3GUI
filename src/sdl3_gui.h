#pragma once
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <algorithm>
#include <type_traits>

// ─── Forward Declarations ────────────────────────────────────────────────────
class GUIApp;
class Container;

// ─────────────────────────────────────────────────────────────────────────────
// ─── Theme ───────────────────────────────────────────────────────────────────
// ─────────────────────────────────────────────────────────────────────────────
struct Theme {
    SDL_Color   bg                = { 30,  30,  35,  255 };     // Main window background (darkest color in the palette)
    SDL_Color   fg                = { 220, 220, 225, 255 };     // Default text color (light gray, readable on dark bg)
    SDL_Color   accent            = { 80,  120, 200, 255 };     // Primary interactive color (buttons, selected items, focused borders)
    SDL_Color   accentHover       = { 100, 140, 220, 255 };     // Brighter accent for hover state (buttons, menu items)
    SDL_Color   border            = { 60,  60,  70,  255 };     // Subtle 1px outlines around widgets (buttons, textboxes, dropdowns)
    SDL_Color   inputBg           = { 40,  40,  48,  255 };     // Background fill for input fields (textboxes, scrollbox content area)
    SDL_Color   tabActive         = { 50,  70,  110, 255 };     // Active/selected tab header background
    SDL_Color   tabInactive       = { 35,  35,  42,  255 };     // Inactive tab header background
    SDL_Color   dialogBg          = { 45,  45,  55,  255 };     // Dialog window background (slightly lighter than main bg for depth)
    SDL_Color   menuBg            = { 35,  35,  45,  255 };     // Menu dropdown background
    int         fontSize          = 16;                                     // The font size
};

// ─────────────────────────────────────────────────────────────────────────────
// ─── Basic Widget─────────────────────────────────────────────────────────────
// ─────────────────────────────────────────────────────────────────────────────
class Widget 
{
public:
    virtual ~Widget() = default;

    std::string tooltip;                                    // If non-empty, GUIApp::findHovered will show a tooltip after delay
    SDL_Rect rect           { 0, 0, 0, 0 };     // Absolute screen position/size, computed by parent's layout()
    SDL_Rect relRect        { 0, 0, 0, 0 };     // Position/size relative to parent — what you set in the constructor
    bool visible            = true;                         // Skip render + event handling if false
    bool enabled            = true;                         // Skip event handling if false
    bool hovered            = false;                        // Set by Button each frame for hover coloring
    bool focused            = false;                        // Set by TextBox when clicked, cleared by GUIApp on next click
    bool stretchH           = false;                        // Tells layout() to fill parent width
    bool stretchV           = false;                        // Tells layout() to fill parent height
    int stretchBottomOffset = 0;                            // Pixels to leave free at the bottom when stretching
    int stretchRightOffset  = 0;                            // Pixels to leave free at the right when stretching
    bool anchorBottom       = false;                        // Tells layout() to measure position from bottom edge
    bool anchorRight        = false;                        // Tells layout() to measure position from right edge

    /*  Sets pixel offsets to leave free when stretching (prevents overlap with anchored widgets) */
    void setStretchOffsets(int bottom, int right)
    {
        stretchBottomOffset = bottom;
        stretchRightOffset = right;
    }

    /*  Anchors widget to bottom and/or right edge of parent (position measured from that edge) */
    void setAnchor(bool bottom, bool right) { anchorBottom = bottom; anchorRight = right; }

    /*  Enables/disables horizontal/vertical stretching in layout() */
    void setStretch(bool h, bool v)         { stretchH = h; stretchV = v; }

    /*  Sets the tooltip text shown when mouse hovers over this widget */
    void setTooltip(const std::string& t)   { tooltip = t; }

    /*  Derived classes override to handle clicks, keypresses, mouse motion, etc.
        Called by the parent Container during event dispatch, in REVERSE z-order
        (topmost widget gets first chance). Returning true stops propagation. */
    virtual bool handleEvent(const SDL_Event& e, GUIApp* app) { return false; }

    
    /*  Used by ScrollBox to recalculate content size, or by future widgets
        for animation ticks, validation, state machines, etc.
        Called by Container::layout() before rendering. */
    virtual void update() { }

    /*  Derived classes override to draw their specific visual (text, rect, scrollbar, etc.).
        Called by the parent Container during the render pass, in z-order.
        Parameters: renderer to draw with, font for text, theme for colors/sizes. */
    virtual void render(SDL_Renderer* r, TTF_Font* font, const Theme& theme) {}
 
    /*  Hit-test: is a point inside rect? Used by every event handler */
    bool contains(int x, int y) const {
        return x >= rect.x && x < rect.x + rect.w && y >= rect.y && y < rect.y + rect.h;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// ─── Container ───────────────────────────────────────────────────────────────
// ─────────────────────────────────────────────────────────────────────────────
class Container : public Widget 
{
public:
    std::vector<std::unique_ptr<Widget>>    children;     // Owns all child widgets (unique_ptr)
    std::vector<Widget*>                    zOrder;       // Raw pointers in render order — topmost is last


    /*  Creates a widget, adds to both vectors, returns the pointer 
        Returns raw pointer for configuration (setTooltip, setStretch, etc.) */
    template<typename T, typename... Args>
    T* add(Args&&... args) 
    {
        T* raw = new T(std::forward<Args>(args)...);
        children.push_back(std::unique_ptr<Widget>(raw));
        zOrder.push_back(raw);
        return raw;
    }

    /*  Computes absolute rect for every child from its relRect + own rect.
        Applies stretch and anchor rules. Recurses into nested Containers.
        Called every frame before render and before event dispatch. */
    void layout() 
    {
        for (auto& c : children) 
        {
            c->rect.x = rect.x + c->relRect.x;
            c->rect.y = rect.y + c->relRect.y;
            c->rect.w = c->relRect.w;
            c->rect.h = c->relRect.h;
            if (c->stretchH) c->rect.w      = rect.w - c->relRect.x - c->stretchRightOffset;
            if (c->stretchV) c->rect.h      = rect.h - c->relRect.y - c->stretchBottomOffset;
            if (c->anchorBottom) c->rect.y  = rect.y + rect.h - c->relRect.y - c->relRect.h;
            if (c->anchorRight)  c->rect.x  = rect.x + rect.w - c->relRect.x - c->relRect.w;
            if (auto* sub = dynamic_cast<Container*>(c.get()))
                sub->layout();
        }
    }


    /*  Dispatches event to children in REVERSE z-order (topmost first).
        First child that returns true consumes the event; stops propagation. */
    bool handleEvent(const SDL_Event& e, GUIApp* app) override 
    {
        for (auto it = zOrder.rbegin(); it != zOrder.rend(); ++it) 
        {
            if ((*it)->visible && (*it)->handleEvent(e, app))
                return true;
        }
        return false;
    }

    /*  Draws all visible children in z-order (bottom to top) */
    void render(SDL_Renderer* r, TTF_Font* font, const Theme& theme) override 
    {
        if (!visible) return;
        for (auto* w : zOrder) 
        {
            if (w->visible) w->render(r, font, theme);
        }
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// ─── Label ───────────────────────────────────────────────────────────────────
// ─────────────────────────────────────────────────────────────────────────────
class Label : public Widget
{
public:
    std::string text;           // The string to display

    /*  Label constructor taking the text, position, and size */
    Label(const std::string& txt, int x, int y, int w, int h)
        : text(txt) 
    {
        relRect = { x, y, w, h };
    }

    /*  Draws the text string at the widget's absolute position using TTF rendering.
        No interaction — purely visual. */
    void render(SDL_Renderer* r, TTF_Font* font, const Theme& theme) override
    {
        if (!visible) return;
        SDL_Surface* surf = TTF_RenderText_Blended(font, text.c_str(), 0, theme.fg);
        if (surf)
        {
            SDL_Texture* tex = SDL_CreateTextureFromSurface(r, surf);
            SDL_FRect dst {
                static_cast<float>(rect.x), static_cast<float>(rect.y), static_cast<float>(surf->w), static_cast<float>(surf->h)
            };
            SDL_RenderTexture(r, tex, nullptr, &dst);
            SDL_DestroyTexture(tex);
            SDL_DestroySurface(surf);
        }
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// ─── Button ──────────────────────────────────────────────────────────────────
// ─────────────────────────────────────────────────────────────────────────────
class Button : public Widget 
{
public:
    std::string text;                           // Label shown on the button
    std::function<void()> onClick;              // Lambda called when clicked

    /*  Creates a button with optional click callback.
        Template on Callback allows passing a lambda directly without explicit std::function conversion,
        which MSVC struggles with inside make_unique's perfect forwarding.
        Default is nullptr_t (no callback). The lambda is stored into onClick via explicit conversion. */
    template<typename Callback = std::nullptr_t>
    Button(const std::string& txt, int x, int y, int w, int h, Callback&& cb = std::nullptr_t{})
        : text(txt) 
    {
        relRect = { x, y, w, h };

        if constexpr (!std::is_same_v<std::decay_t<Callback>, std::nullptr_t>) 
        {
            onClick = std::function<void()>(std::forward<Callback>(cb));
        }
    }

    /* Draws a filled rectangle (accent color, brighter on hover) with centered text. */
    void render(SDL_Renderer* r, TTF_Font* font, const Theme& theme) override
    {
        if (!visible) return;

        SDL_Color col = hovered ? theme.accentHover : theme.accent;
        SDL_SetRenderDrawColor(r, col.r, col.g, col.b, col.a);
        SDL_FRect fr{ static_cast<float>(rect.x), static_cast<float>(rect.y), static_cast<float>(rect.w), static_cast<float>(rect.h) };
        SDL_RenderFillRect(r, &fr);

        SDL_Color bc = theme.border;
        SDL_SetRenderDrawColor(r, bc.r, bc.g, bc.b, bc.a);
        SDL_RenderRect(r, &fr);

        SDL_Surface* surf = TTF_RenderText_Blended(font, text.c_str(), 0, theme.fg);
        if (surf) 
        {
            SDL_Texture* tex = SDL_CreateTextureFromSurface(r, surf);
            SDL_FRect dst{
                (float)(rect.x + (rect.w - surf->w) / 2.0f),
                (float)(rect.y + (rect.h - surf->h) / 2.0f),
                static_cast<float>(surf->w), static_cast<float>(surf->h) };
            SDL_RenderTexture(r, tex, nullptr, &dst);
            SDL_DestroyTexture(tex);
            SDL_DestroySurface(surf);
        }
    }

    /* Tracks hover via SDL_GetMouseState. On mouse-up inside rect, fires onClick and returns true (consumes event) */
    bool handleEvent(const SDL_Event& e, GUIApp* app) override 
    {
        if (!enabled || !visible) return false;
        float mx, my;
        SDL_GetMouseState(&mx, &my);
        hovered = contains(static_cast<int>(mx), static_cast<int>(my));

        if (e.type == SDL_EVENT_MOUSE_BUTTON_UP 
            && e.button.button == SDL_BUTTON_LEFT 
            && contains(static_cast<int>(e.button.x), static_cast<int>(e.button.y))) 
        {
            if (onClick) onClick();

            return true;
        }
        return false;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// ─── TextBox ─────────────────────────────────────────────────────────────────
// ─────────────────────────────────────────────────────────────────────────────
class TextBox : public Widget 
{
public: 
    std::string text;           // Current text content
    int cursorPos = 0;          // Cursor position (0 = start)
    int scrollOffset = 0;       // Character offset for horizontal text scroll
   
    /* Constructor that takes the relative rectangle, which is the widget's position and size relative to its parent container */
    TextBox(int x, int y, int w, int h) 
    {
        relRect = { x, y, w, h }; 
    }

    /* Draws background (brighter when focused), border, text, and a blinking cursor line */
    void render(SDL_Renderer* r, TTF_Font* font, const Theme& theme) override 
    {
        if (!visible) return;
        SDL_Color bg = focused ? SDL_Color { 50, 50, 65, 255 } : theme.inputBg;
        SDL_SetRenderDrawColor(r, bg.r, bg.g, bg.b, bg.a);
        SDL_FRect fr {
            static_cast<float>(rect.x), static_cast<float>(rect.y), static_cast<float>(rect.w), static_cast<float>(rect.h)
        };
        SDL_RenderFillRect(r, &fr);

        SDL_Color bc = focused ? theme.accent : theme.border;
        SDL_SetRenderDrawColor(r, bc.r, bc.g, bc.b, bc.a);
        SDL_RenderRect(r, &fr);

        std::string display = text.substr(scrollOffset);
        SDL_Surface* surf   = TTF_RenderText_Blended(font, display.c_str(), 0, theme.fg);
        if (surf)
        {
            SDL_Texture* tex = SDL_CreateTextureFromSurface(r, surf);
            SDL_FRect dst {
                static_cast<float>(rect.x + 4),
                (float)(rect.y + (rect.h - surf->h) / 2.0f),
                static_cast<float>(surf->w), static_cast<float>(surf->h)
            };
            SDL_RenderTexture(r, tex, nullptr, &dst);
            SDL_DestroyTexture(tex);
            SDL_DestroySurface(surf);
        }

        if (focused)
        {
            int textW = 0, textH = 0;
            TTF_GetStringSize(font, text.c_str(), 0, &textW, &textH);
            int cx = rect.x + 4 + textW - scrollOffset;
            SDL_SetRenderDrawColor(r, theme.fg.r, theme.fg.g, theme.fg.b, theme.fg.a);
            SDL_RenderLine(r, cx, rect.y + 4, cx, rect.y + rect.h - 4);
        }
    }

    /* On click: sets focused = true. While focused: handles Backspace, Left, Right, and TEXT_INPUT to edit text */
    bool handleEvent(const SDL_Event& e, GUIApp* app) override
    {
        if (!visible) return false;

        // Mouse button down
        if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
            e.button.button == SDL_BUTTON_LEFT &&
            contains(static_cast<int>(e.button.x), static_cast<int>(e.button.y)))
        {
            focused = true;
            return true;
        }

        // Key pressed down
        if (e.type == SDL_EVENT_KEY_DOWN && focused)
        {
            if (e.key.key == SDLK_BACKSPACE && !text.empty())
            {
                text.pop_back();
                if (cursorPos > 0) cursorPos--;
            }
            else if (e.key.key == SDLK_LEFT)    { if (cursorPos > 0) cursorPos--; }
            else if (e.key.key == SDLK_RIGHT)   { if (cursorPos < static_cast<int>(text.size())) cursorPos++; }
            return true;
        }

        // Text is being read from keyboard
        if (e.type == SDL_EVENT_TEXT_INPUT && focused)
        {
            text += e.text.text;
            return true;
        }
        return false;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// ─── ScrollBox ───────────────────────────────────────────────────────────────
// ─────────────────────────────────────────────────────────────────────────────
class ScrollBox : public Widget 
{
public:
    std::vector<std::unique_ptr<Widget>> items;         // The scrollable content (usually Labels)
    float scrollX       = 0.0f;                         // Current scroll offset in X pixels
    float scrollY       = 0.0f;                         // Current scroll offset in Y pixels
    int viewportW       = 0;                            // Visible area width size (set in constructor)
    int viewportH       = 0;                            // Visible area height size (set in constructor)
    int contentW        = 0;                            // Total content width size (computed in update())
    int contentH        = 0;                            // Total content height size (computed in update())
    int selectedIndex   = -1;                           // Which item is highlighted (-1 = none)
    bool draggingV      = false;                        // Whether the scrollbar thumb is being dragged vertically
    bool draggingH      = false;                        // Whether the scrollbar thumb is being dragged horizontally
    int dragOffsetV     = 0;                            // Mouse offset from thumb top/left at drag start vertical
    int dragOffsetH     = 0;                            // Mouse offset from thumb top/left at drag start horizontal
    int sbW             = 10;                           // Scrollbar is 10px thick
    std::function<void(const std::string&)> onSelect;   // Callback fired when selection changes


    /* Constructor that sets up the viewport rectangle */
    ScrollBox(int x, int y, int w, int h) : viewportW(w), viewportH(h) 
    {
        relRect = { x, y, w, h };
    }

    /* Adds a widget to the scrollable list */
    template<typename T, typename... Args>
    T* addItem(Args&&... args) 
    {
        T* raw = new T(std::forward<Args>(args)...);
        items.push_back(std::unique_ptr<Widget>(raw));
        return raw;
    }

    /* Convenience: appends a Label and calls update() */
    void addNewItem(const std::string& text) 
    {
        int y = contentH;
        items.push_back(std::unique_ptr<Widget>(new Label(text, 4, y, relRect.w - sbW - 14, 28)));
        update();
    }

    /* Returns text of the currently selected item */
    std::string getSelectedText() const 
    {
        if (selectedIndex < 0 || selectedIndex >= static_cast<int>(items.size())) return "";
        auto* lbl = dynamic_cast<Label*>(items[selectedIndex].get());
        return lbl ? lbl->text : "";
    }

    /* Recalculates contentH and contentW from item sizes */
    void update() override 
    {
        contentH = 0;
        contentW = 0;
        for (auto& item : items) 
        {
            item->relRect.x = 2;
            item->relRect.y = contentH;
            contentH += item->relRect.h;
            if (item->relRect.w > contentW) contentW = item->relRect.w;
        }
    }

    /* Handles: wheel scroll, scrollbar drag (V and H), item click for selection, then passes remaining events to items */
    bool handleEvent(const SDL_Event& e, GUIApp* app) override 
    {
        if (!visible) return false;
        int clipW = rect.w - sbW;
        int clipH = rect.h - sbW;

        // Wheel scroll (vertical or horizontal)
        if (e.type == SDL_EVENT_MOUSE_WHEEL && contains(static_cast<int>(e.wheel.mouse_x), static_cast<int>(e.wheel.mouse_y))) 
        {
            if (e.wheel.y != 0) 
            {
                scrollY -= e.wheel.y * 40.0f;  // minus instead of plus
                float maxV = std::max(0.0f, static_cast<float>(contentH) - clipH);
                scrollY = std::clamp(scrollY, 0.0f, maxV);
            }
            if (e.wheel.x != 0) 
            {
                scrollX -= e.wheel.x * 40.0f;  // minus instead of plus
                float maxH = std::max(0.0f, static_cast<float>(contentW) - clipW);
                scrollX = std::clamp(scrollX, 0.0f, maxH);
            }
            return true;
        }

        // Vertical scrollbar drag
        if (contentH > clipH) 
        {
            int sbX         = rect.x + clipW;
            int thumbH      = std::max(20, static_cast<int>((float)clipH / contentH * clipH));
            int thumbY      = rect.y + static_cast<int>((scrollY / (contentH - clipH)) * (clipH - thumbH));
            SDL_Rect vTrack{ sbX, rect.y, sbW, clipH };
            SDL_Rect vThumb{ sbX, thumbY, sbW, thumbH };

            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT) 
            {
                SDL_Point pt{ static_cast<int>(e.button.x), static_cast<int>(e.button.y) };
                if (SDL_PointInRect(&pt, &vTrack)) 
                {
                    draggingV = true;
                    dragOffsetV = static_cast<int>(e.button.y) - thumbY;
                    return true;
                }
            }
            if (e.type == SDL_EVENT_MOUSE_BUTTON_UP && draggingV) { draggingV = false; return true; }
            if (draggingV && e.type == SDL_EVENT_MOUSE_MOTION) 
            {
                int newThumbY = static_cast<int>(e.motion.y) - dragOffsetV;
                float ratio = static_cast<float>(newThumbY - rect.y) / static_cast<float>(clipH - thumbH);
                scrollY = ratio * static_cast<float>(contentH - clipH);
                scrollY = std::clamp(scrollY, 0.0f, std::max(0.0f, static_cast<float>(contentH) - clipH));
                return true;
            }
        }

        // Horizontal scrollbar drag
        if (contentW > clipW) 
        {
            int sbY         = rect.y + clipH;
            int thumbW      = std::max(20, static_cast<int>((float)clipW / contentW * clipW));
            int thumbX      = rect.x + static_cast<int>((scrollX / (contentW - clipW)) * (clipW - thumbW));
            SDL_Rect hTrack{ rect.x, sbY, clipW, sbW };
            SDL_Rect hThumb{ thumbX, sbY, thumbW, sbW };

            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT) 
            {
                SDL_Point pt{ static_cast<int>(e.button.x), static_cast<int>(e.button.y) };
                if (SDL_PointInRect(&pt, &hTrack)) 
                {
                    draggingH = true;
                    dragOffsetH = static_cast<int>(e.button.x) - thumbX;
                    return true;
                }
            }
            if (e.type == SDL_EVENT_MOUSE_BUTTON_UP && draggingH) { draggingH = false; return true; }
            if (draggingH && e.type == SDL_EVENT_MOUSE_MOTION)
            {
                int newThumbX   = static_cast<int>(e.motion.x) - dragOffsetH;
                float ratio     = static_cast<float>(newThumbX - rect.x) / static_cast<float>(clipW - thumbW);
                scrollX         = ratio * static_cast<float>(contentW - clipW);
                scrollX         = std::clamp(scrollX, 0.0f, std::max(0.0f, static_cast<float>(contentW) - clipW));
                return true;
            }
        }

        // Item click (selection)
        if (e.type == SDL_EVENT_MOUSE_BUTTON_UP && e.button.button == SDL_BUTTON_LEFT) 
        {
            int mx = static_cast<int>(e.button.x), my = static_cast<int>(e.button.y);
            if (mx >= rect.x && mx < rect.x + clipW && my >= rect.y && my < rect.y + clipH) 
            {
                for (int i = 0; i < static_cast<int>(items.size()); ++i) 
                {
                    int drawX = items[i]->relRect.x - static_cast<int>(scrollX);
                    int drawY = items[i]->relRect.y - static_cast<int>(scrollY);
                    SDL_Rect itemRect{ rect.x + drawX, rect.y + drawY, items[i]->relRect.w, items[i]->relRect.h };
                    SDL_Point pt{ mx, my };
                    if (SDL_PointInRect(&pt, &itemRect)) 
                    {
                        int prev = selectedIndex;
                        selectedIndex = i;
                        if (onSelect && prev != selectedIndex)
                            onSelect(getSelectedText());
                        return true;
                    }
                }
            }
        }

        for (auto& item : items)
        {
            if (item->handleEvent(e, app)) return true;
        }
        return false;
    }

    /* Clips to viewport, draws items offset by scroll, draws both scrollbars */
    void render(SDL_Renderer* r, TTF_Font* font, const Theme& theme) override
    {
        if (!visible) return;

        int clipW = rect.w - sbW;
        int clipH = rect.h - sbW;
        SDL_Rect clip { rect.x, rect.y, clipW, clipH };
        SDL_SetRenderClipRect(r, &clip);

        SDL_Color bg = theme.inputBg;
        SDL_SetRenderDrawColor(r, bg.r, bg.g, bg.b, bg.a);
        SDL_FRect fr {
            static_cast<float>(rect.x), static_cast<float>(rect.y), static_cast<float>(clipW), static_cast<float>(clipH)
        };
        SDL_RenderFillRect(r, &fr);

        for (int i = 0; i < static_cast<int>(items.size()); ++i) 
        {
            auto& item = items[i];
            int drawX  = item->relRect.x - static_cast<int>(scrollX);
            int drawY  = item->relRect.y - static_cast<int>(scrollY);

            if (drawY + item->relRect.h > 0 && drawY < clipH &&
                drawX + item->relRect.w > 0 && drawX < clipW) 
            {
                if (i == selectedIndex) 
                {
                    SDL_SetRenderDrawColor(r, theme.accent.r, theme.accent.g, theme.accent.b, 255);
                    SDL_FRect hl{
                        static_cast<float>(rect.x + drawX), static_cast<float>(rect.y + drawY),
                        static_cast<float>(item->relRect.w), static_cast<float>(item->relRect.h)
                    };
                    SDL_RenderFillRect(r, &hl);
                }
                int savedX   = item->rect.x, savedY = item->rect.y;
                item->rect.x = rect.x + drawX;
                item->rect.y = rect.y + drawY;
                item->render(r, font, theme);
                item->rect.x = savedX;
                item->rect.y = savedY;
            }
        }

        SDL_SetRenderClipRect(r, nullptr);

        // Vertical scrollbar
        if (contentH > clipH) 
        {
            int sbX    = rect.x + clipW;
            int thumbH = std::max(20, static_cast<int>((float)clipH / contentH * clipH));
            int thumbY = rect.y + static_cast<int>((scrollY / (contentH - clipH)) * (clipH - thumbH));

            SDL_SetRenderDrawColor(r, 50, 50, 60, 255);
            SDL_FRect track {
                static_cast<float>(sbX), static_cast<float>(rect.y), static_cast<float>(sbW), static_cast<float>(clipH)
            };
            SDL_RenderFillRect(r, &track);
            SDL_SetRenderDrawColor(r, 100, 100, 120, 255);
            SDL_FRect thumb {
                static_cast<float>(sbX), static_cast<float>(thumbY), static_cast<float>(sbW), static_cast<float>(thumbH)
            };
            SDL_RenderFillRect(r, &thumb);
        }

        // Horizontal scrollbar
        if (contentW > clipW) 
        {
            int sbY    = rect.y + clipH;
            int thumbW = std::max(20, static_cast<int>((float)clipW / contentW * clipW));
            int thumbX = rect.x + static_cast<int>((scrollX / (contentW - clipW)) * (clipW - thumbW));
            SDL_SetRenderDrawColor(r, 50, 50, 60, 255);
            SDL_FRect track {
                static_cast<float>(rect.x), static_cast<float>(sbY), static_cast<float>(clipW), static_cast<float>(sbW)
            };
            SDL_RenderFillRect(r, &track);
            SDL_SetRenderDrawColor(r, 100, 100, 120, 255);
            SDL_FRect thumb {
                static_cast<float>(thumbX), static_cast<float>(sbY), static_cast<float>(thumbW), static_cast<float>(sbW)
            };
            SDL_RenderFillRect(r, &thumb);
        }
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// ─── ToolBar ─────────────────────────────────────────────────────────────────
// ─────────────────────────────────────────────────────────────────────────────
class Toolbar : public Container 
{
public:
    int itemSpacing = 4;            // Gap between toolbar buttons
    int itemH       = 32;           // Fixed height for toolbar items

    /* Constructor that takes the relative rectangle, which is the widget's position and size relative to its parent container */
    Toolbar(int x, int y, int w, int h) { relRect = { x, y, w, h }; }

    /* Like Container::add but auto-positions items horizontally */
    template<typename T, typename... Args>
    T* addTool(Args&&... args) 
    {
        int nextX       = rect.x + itemSpacing;
        for (auto& c : children) 
        {
            nextX = c->rect.x + c->rect.w + itemSpacing;
        }
        T* raw          = new T(std::forward<Args>(args)...);
        raw->relRect    = { nextX - rect.x, 2, raw->relRect.w, itemH };
        children.push_back(std::unique_ptr<Widget>(raw));
        zOrder.push_back(raw);
        return raw;
    }

    /* Draws a dark background + bottom border, then calls Container::render */
    void render(SDL_Renderer* r, TTF_Font* font, const Theme& theme) override 
    {
        if (!visible) return;

        const SDL_Color bg = { 40, 40, 50, 255 };
        SDL_SetRenderDrawColor(r, bg.r, bg.g, bg.b, bg.a);
        const SDL_FRect fr{   static_cast<float>(rect.x), static_cast<float>(rect.y), 
                        static_cast<float>(rect.w), static_cast<float>(rect.h) };
        SDL_RenderFillRect(r, &fr);
        SDL_SetRenderDrawColor(r, theme.border.r, theme.border.g, theme.border.b, theme.border.a);
        SDL_RenderLine(r, rect.x, rect.y + rect.h - 1, rect.x + rect.w, rect.y + rect.h - 1);
        Container::render(r, font, theme);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// ─── Tab Panel ───────────────────────────────────────────────────────────────
// ─────────────────────────────────────────────────────────────────────────────
class TabPanel : public Widget 
{
public:
    
    /* The name and content data for a tab */
    struct Tab 
    {
        std::string name;
        std::unique_ptr<Container> content;
    };
    std::vector<Tab> tabs;                 // Vector of{ name, content } pairs
    int activeTab   = 0;                   // Index of the currently visible tab
    int tabH        = 28;                  // Height of the tab header row

    /* Constructor that sets up the relRect */
    TabPanel(int x, int y, int w, int h) { relRect = { x, y, w, h }; }

    /* Creates a new Container for that tab's content, returns it */
    Container* addTab(const std::string& name)
    {
        Tab tab;
        tab.name    = name;
        tab.content = std::make_unique<Container>();
        tab.content->relRect = { 2, tabH, rect.w - 4, rect.h - tabH - 2 };
        tabs.push_back(std::move(tab));
        return tabs.back().content.get();
    }
    
    /* Checks if click is on a tab header (switches tab), otherwise dispatches to active tab's content */
    bool handleEvent(const SDL_Event& e, GUIApp* app) override {
        if (!visible) return false;
        if (e.type == SDL_EVENT_MOUSE_BUTTON_UP && e.button.button == SDL_BUTTON_LEFT) {
            int tx = rect.x;
            for (int i = 0; i < static_cast<int>(tabs.size()); ++i) {
                int tw = 60 + static_cast<int>(tabs[i].name.size()) * 7;
                SDL_Rect tabRect{ tx, rect.y, tw, tabH };
                SDL_Point pt{ static_cast<int>(e.button.x), static_cast<int>(e.button.y) };
                if (SDL_PointInRect(&pt, &tabRect)) {
                    activeTab = i;
                    return true;
                }
                tx += tw;
            }
        }
        if (activeTab < static_cast<int>(tabs.size())) {
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

    /* Draws tab headers (active = highlighted), then renders the active tab's content with correct absolute position */
    void render(SDL_Renderer* r, TTF_Font* font, const Theme& theme) override 
    {
        if (!visible) return;

        int tx = rect.x;
        for (int i = 0; i < static_cast<int>(tabs.size()); ++i) 
        {
            int tw          = 60 + static_cast<int>(tabs[i].name.size()) * 7;
            SDL_Color col   = (i == activeTab) ? theme.tabActive : theme.tabInactive;
            SDL_SetRenderDrawColor(r, col.r, col.g, col.b, col.a);
            SDL_FRect tr{ static_cast<float>(tx), static_cast<float>(rect.y), static_cast<float>(tw), static_cast<float>(tabH) };
            SDL_RenderFillRect(r, &tr);

            SDL_Surface* surf = TTF_RenderText_Blended(font, tabs[i].name.c_str(), 0, theme.fg);
            if (surf) 
            {
                SDL_Texture* tex = SDL_CreateTextureFromSurface(r, surf);
                SDL_FRect dst{ static_cast<float>(tx + 8), static_cast<float>(rect.y + 5), static_cast<float>(surf->w), static_cast<float>(surf->h) };
                SDL_RenderTexture(r, tex, nullptr, &dst);
                SDL_DestroyTexture(tex);
                SDL_DestroySurface(surf);
            }
            tx += tw;
        }

        if (activeTab < static_cast<int>(tabs.size())) 
        {
            auto* content = tabs[activeTab].content.get();
            content->rect.x = rect.x + content->relRect.x;
            content->rect.y = rect.y + content->relRect.y;
            content->rect.w = rect.w - content->relRect.x;
            content->rect.h = rect.h - content->relRect.y;
            content->layout();
            content->render(r, font, theme);
        }
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// ─── Menu ────────────────────────────────────────────────────────────────────
// ─────────────────────────────────────────────────────────────────────────────
class Menu : public Widget
{
public:
    /* Text and action function data for a Menu Item */
    struct MenuItem 
    {
        std::string text;
        std::function<void()> action;
    };
    std::string title;                      // Text shown on the menu button (e.g. "File")
    std::vector<MenuItem> items;            // Vector of {text, action} pairs for the dropdown
    bool open   = false;                    // Whether the dropdown is currently visible
    int itemH   = 24;                       // Height of each dropdown row

    /* Constructore taking in the title and the relrect rectangle dimensions */
    Menu(const std::string& title, int x, int y, int w, int h)
        : title(title) 
    {
        relRect = { x, y, w, h };
    }

    /* Adds a dropdown entry */
    void addItem(const std::string& text, std::function<void()> action = nullptr) 
    {
        items.push_back({ text, std::move(action) });
    }

    /* Computes width from the longest item text */
    int menuWidth() const 
    {
        int w = rect.w;
        for (auto& item : items)
            if (static_cast<int>(item.text.size()) * 8 > w) w = static_cast<int>(item.text.size()) * 8;
        return w + 16;
    }

    /* Total dropdown height */
    int menuHeight() const
    {
        return itemH * static_cast<int>(items.size()) + 12;
    }

    /* Click on button toggles open. Click on a dropdown item fires its action and closes. Click outside closes */
    bool handleEvent(const SDL_Event& e, GUIApp* app) override
    {
        if (!visible) return false;
        if (e.type == SDL_EVENT_MOUSE_BUTTON_UP && e.button.button == SDL_BUTTON_LEFT) 
        {
            int mx = static_cast<int>(e.button.x), my = static_cast<int>(e.button.y);

            // Click on button toggles
            if (contains(mx, my)) 
            {
                open = !open;
                return true;
            }

            // Click on dropdown item
            if (open) 
            {
                int mw = menuWidth();
                int dropY = rect.y + rect.h;
                if (mx >= rect.x && mx < rect.x + mw && my >= dropY && my < dropY + menuHeight()) 
                {
                    int idx = (my - dropY - 2) / itemH;
                    if (idx >= 0 && idx < static_cast<int>(items.size())) 
                    {
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

    /* Draws the button. If open, draws the dropdown overlay with hover highlighting on items */
    void render(SDL_Renderer* r, TTF_Font* font, const Theme& theme) override
    {
        if (!visible) return;

        // Button
        SDL_Color col = open ? theme.accentHover : theme.accent;
        SDL_SetRenderDrawColor(r, col.r, col.g, col.b, col.a);
        SDL_FRect fr{ static_cast<float>(rect.x), static_cast<float>(rect.y), static_cast<float>(rect.w), static_cast<float>(rect.h) };
        SDL_RenderFillRect(r, &fr);

        SDL_Surface* surf = TTF_RenderText_Blended(font, title.c_str(), 0, theme.fg);
        if (surf)
        {
            SDL_Texture* tex = SDL_CreateTextureFromSurface(r, surf);
            SDL_FRect dst{ static_cast<float>(rect.x + 8), static_cast<float>(rect.y + 4), static_cast<float>(surf->w), static_cast<float>(surf->h) };
            SDL_RenderTexture(r, tex, nullptr, &dst);
            SDL_DestroyTexture(tex);
            SDL_DestroySurface(surf);
        }

        // Dropdown (rendered as overlay on main window)
        if (open)
        {
            int mw = menuWidth();
            int mh = menuHeight();
            int mx = rect.x;
            int my = rect.y + rect.h;

            // Background
            SDL_SetRenderDrawColor(r, theme.menuBg.r, theme.menuBg.g, theme.menuBg.b, 255);
            SDL_FRect drop{ static_cast<float>(mx), static_cast<float>(my), static_cast<float>(mw), static_cast<float>(mh) };
            SDL_RenderFillRect(r, &drop);




            // Border (no top edge — flows from the button)
            SDL_SetRenderDrawColor(r, theme.border.r, theme.border.g, theme.border.b, 255);
            SDL_RenderLine(r, mx, my, mx, my + mh);
            SDL_RenderLine(r, mx + mw, my, mx + mw, my + mh);
            SDL_RenderLine(r, mx, my + mh, mx + mw, my + mh);

            // Items
            for (int i = 0; i < static_cast<int>(items.size()); ++i)
            {
                int iy = my + 6 + i * itemH;

                // Hover highlight
                float pmx, pmy;
                SDL_GetMouseState(&pmx, &pmy);
                if (static_cast<int>(pmx) >= mx && static_cast<int>(pmx) < mx + mw &&
                    static_cast<int>(pmy) >= iy && static_cast<int>(pmy) < iy + itemH)
                {
                    SDL_SetRenderDrawColor(r, theme.accent.r, theme.accent.g, theme.accent.b, 255);
                    SDL_FRect hl{ static_cast<float>(mx), static_cast<float>(iy), static_cast<float>(mw), static_cast<float>(itemH) };
                    SDL_RenderFillRect(r, &hl);
                }

                SDL_Surface* isurf = TTF_RenderText_Blended(font, items[i].text.c_str(), 0, theme.fg);
                if (isurf)
                {
                    SDL_Texture* itex = SDL_CreateTextureFromSurface(r, isurf);
                    SDL_FRect idst{ 10.0f + static_cast<float>(mx), static_cast<float>(iy + 4), static_cast<float>(isurf->w), static_cast<float>(isurf->h) };
                    SDL_RenderTexture(r, itex, nullptr, &idst);
                    SDL_DestroyTexture(itex);
                    SDL_DestroySurface(isurf);
                }
            }
        }
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// ─── Dialog ──────────────────────────────────────────────────────────────────
// ─────────────────────────────────────────────────────────────────────────────
class Dialog : public Container {
public:
    std::string title{};                    // Window title
    bool modal              = true;         // Non-modal (modeless) means the dialog stays open, but you can still interact with the parent window behind it.    
    SDL_Window* window      = nullptr;      // The SDL window to draw one
    SDL_Renderer* renderer  = nullptr;      // Renderer used to draw
    bool open               = false;        // Whether the dialog is currently visible

    /* Constructor that takes in the window title, the width and height, and the modal, which dialog blocks interaction with the parent window while it's open */
    Dialog(const std::string& title, int w, int h, bool modal = true)
        : title(title), modal(modal)
    {
        relRect = { 0, 0, w, h };
        rect    = { 0, 0, w, h };
    }

    /* Creates (once) or shows the window, centered on parent */
    void openDialog(SDL_Window* parent)
    {
        if (!window)
        {
            int px, py, pw, ph;
            SDL_GetWindowPosition(parent, &px, &py);
            SDL_GetWindowSize(parent, &pw, &ph);
            int dx = px + (pw - rect.w) / 2;
            int dy = py + (ph - rect.h) / 2;

            window = SDL_CreateWindow(title.c_str(), rect.w, rect.h,SDL_WINDOW_BORDERLESS | SDL_WINDOW_UTILITY | SDL_WINDOW_RESIZABLE);
            SDL_SetWindowPosition(window, dx, dy);
            if (modal) { SDL_SetWindowModal(window, parent); }
            renderer = SDL_CreateRenderer(window, nullptr);
        }
        open = true;
        SDL_ShowWindow(window);
    }

    /* Hides the window */
    void closeDialog()
    {
        open = false;
        if (window) SDL_HideWindow(window);
    }

    /* Catches close-requested, then dispatches to children */
    bool handleEvent(const SDL_Event& e)
    {
        if (!open) return false;
        if (e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && window && e.window.windowID == SDL_GetWindowID(window))
        {
            closeDialog();
            return true;
        }

        for (auto it = zOrder.rbegin(); it != zOrder.rend(); ++it)
        {
            if ((*it)->handleEvent(e, nullptr)) return true;
        }
        return false;
    }

    /* Calls layout(), draws title bar, then renders all children into the dialog's own renderer */
    void render()
    {
        if (!open || !renderer) return;

        layout(); // ← THIS LINE computes children's rect from relRect

        SDL_SetRenderDrawColor(renderer, 45, 45, 55, 255);
        SDL_RenderClear(renderer);

        TTF_Font* font = TTF_OpenFont("C:/Windows/Fonts/arial.ttf", 16.0f);
        if (font)
        {
            SDL_SetRenderDrawColor(renderer, 30, 30, 40, 255);
            SDL_FRect titleBar { 0, 0, static_cast<float>(rect.w), 30.0f };
            SDL_RenderFillRect(renderer, &titleBar);
            SDL_Surface* surf = TTF_RenderText_Blended(font, title.c_str(), 0,
                                                       SDL_Color { 220, 220, 225, 255 });
            if (surf)
            {
                SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
                SDL_FRect dst { 10.0f, 5.0f, static_cast<float>(surf->w), static_cast<float>(surf->h) };
                SDL_RenderTexture(renderer, tex, nullptr, &dst);
                SDL_DestroyTexture(tex);
                SDL_DestroySurface(surf);
            }

            Theme theme;
            for (auto* w : zOrder) { if (w->visible) w->render(renderer, font, theme); }
            TTF_CloseFont(font);
        }
        SDL_RenderPresent(renderer);
    }

    ~Dialog() override
    {
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// ─── GUIApp ──────────────────────────────────────────────────────────────────
// ─────────────────────────────────────────────────────────────────────────────
class GUIApp 
{
public:
    SDL_Window* window                  = nullptr;      // The main SDL window
    SDL_Renderer* renderer              = nullptr;
    TTF_Font* font                      = nullptr;      // Shared TTF font used by all widgets
    bool running                        = false;
    Widget* focusedWidget               = nullptr;      // Currently focused Widget* (usually a TextBox)
    Widget* hoveredWidget               = nullptr;      // Currently hovered widget (for tooltips)
    int hoverFrame                      = 0;            // Frame counter for tooltip delay
    static constexpr int TOOLTIP_DELAY  = 45;           // ~0.75s at 60fps
    Theme theme;                                        // Color/size palette passed to every render call

    std::unique_ptr<Container> root;                    // The top-level Container — all widgets are descendants of this
    std::vector<std::unique_ptr<Dialog>> dialogs;       // All Dialog instances (owned)

    /* Destructor to clean up SDL3 */
    ~GUIApp()
    {
        if (font) TTF_CloseFont(font);
        TTF_Quit();
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        SDL_Quit();
    }

    /* Initializes SDL, TTF, creates window/renderer/font, starts text input */
    bool init(const char* title, int w, int h)
    {
        if (!SDL_Init(SDL_INIT_VIDEO))
        {
            SDL_Log("SDL_Init failed: %s", SDL_GetError());
            return false;
        }
        if (!TTF_Init())
        {
            SDL_Log("TTF_Init failed: %s", SDL_GetError());
            return false;
        }
        window = SDL_CreateWindow(title, w, h, SDL_WINDOW_RESIZABLE);
        if (!window) return false;
        renderer = SDL_CreateRenderer(window, nullptr);
        if (!renderer) return false;

        SDL_StartTextInput(window);

        font = TTF_OpenFont("C:/Windows/Fonts/arial.ttf", static_cast<float>(theme.fontSize));
        if (!font)
        {
            SDL_Log("Failed to load font: %s", SDL_GetError());
            return false;
        }

        root          = std::make_unique<Container>();
        root->rect    = { 0, 0, w, h };
        root->relRect = { 0, 0, w, h };
        running       = true;
        return true;
    }

    /* Allocates a new Dialog, stores it, returns pointer */
    Dialog* createDialog(const std::string& title, int w, int h, bool modal = true)
    {
        dialogs.push_back(std::make_unique<Dialog>(title, w, h, modal));
        return dialogs.back().get();
    }

    /* The run loop of the GUI Program */
    void run()
    {
        while (running)
        {
            SDL_Event e;
            while (SDL_PollEvent(&e)) { handleEvent(e); }
            render();
            SDL_Delay(16);
        }
    }

    void clearAllFocus(Widget* w)
    {
        if (auto* tb = dynamic_cast<TextBox*>(w)) tb->focused = false;
        if (auto* c = dynamic_cast<Container*>(w))
            for (auto& child : c->children) clearAllFocus(child.get());
        if (auto* tp = dynamic_cast<TabPanel*>(w))
            for (auto& tab : tp->tabs) clearAllFocus(tab.content.get());
        if (auto* sb = dynamic_cast<ScrollBox*>(w))
            for (auto& item : sb->items) clearAllFocus(item.get());
    }

    /* Handles quit/escape, calls layout(), clears focus on mouse-down, dispatches to dialogs then root, finds new focus after dispatch */
    void handleEvent(const SDL_Event& e)
    {
        if (e.type == SDL_EVENT_QUIT)
        {
            running = false;
            return;
        }
        if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE)
        {
            running = false;
            return;
        }

        root->layout();

        if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
        {
            clearAllFocus(root.get()); // ← clear ALL textboxes
            focusedWidget = nullptr;
        }

        for (auto& d : dialogs) { if (d->open && d->handleEvent(e)) return; }
        if (root) root->handleEvent(e, this);

        // After dispatch, find which textbox is now focused
        if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) { findFocused(root.get()); }

        if (e.type == SDL_EVENT_WINDOW_RESIZED)
        {
            root->rect.w  = e.window.data1;
            root->rect.h  = e.window.data2;
            root->relRect = root->rect;
        }
    }

    /* Recursively walks the tree to find whichever TextBox has focused == true */
    void findFocused(Widget* w)
    {
        if (auto* tb = dynamic_cast<TextBox*>(w))
        {
            if (tb->focused)
            {
                focusedWidget = tb;
                return;
            }
        }
        if (auto* c = dynamic_cast<Container*>(w))
            for (auto& child : c->children) findFocused(child.get());
        if (auto* tp = dynamic_cast<TabPanel*>(w))
            for (auto& tab : tp->tabs) findFocused(tab.content.get());
        if (auto* sb = dynamic_cast<ScrollBox*>(w))
            for (auto& item : sb->items) findFocused(item.get());
    }

    /* Recursively walks the tree to find the topmost widget under the mouse that has a tooltip */
    Widget* findHovered(Widget* w)
    {
        float mx, my;
        SDL_GetMouseState(&mx, &my);
        if (!w->visible) return nullptr;
        if (w->contains(static_cast<int>(mx), static_cast<int>(my)) && !w->tooltip.empty()) return w;
        if (auto* c = dynamic_cast<Container*>(w))
        {
            for (auto& child : c->children)
            {
                auto* r = findHovered(child.get());
                if (r) return r;
            }
        }
        if (auto* tp = dynamic_cast<TabPanel*>(w))
        {
            for (auto& tab : tp->tabs)
            {
                auto* r = findHovered(tab.content.get());
                if (r) return r;
            }
        }
        if (auto* sb = dynamic_cast<ScrollBox*>(w))
        {
            for (auto& item : sb->items)
            {
                auto* r = findHovered(item.get());
                if (r) return r;
            }
        }
        return nullptr;
    }

    /* Clears screen, calls root->layout() + root->render(), draws tooltip, presents, then renders open dialogs */
    void render()
    {
        SDL_SetRenderDrawColor(renderer, theme.bg.r, theme.bg.g, theme.bg.b, theme.bg.a);
        SDL_RenderClear(renderer);
        if (root)
        {
            root->layout();
            root->render(renderer, font, theme);
        }

        // ─── Tooltip ───
        hoveredWidget = findHovered(root.get());
        if (hoveredWidget) { hoverFrame++; }
        else { hoverFrame = 0; }

        if (hoveredWidget && hoverFrame >= TOOLTIP_DELAY)
        {
            float mx, my;
            SDL_GetMouseState(&mx, &my);
            int textW = 0, textH = 0;
            TTF_GetStringSize(font, hoveredWidget->tooltip.c_str(), 0, &textW, &textH);
            int pad = 6;
            int tw  = textW + pad * 2;
            int th  = textH + pad * 2;
            int tx  = static_cast<int>(mx) + 12;
            int ty  = static_cast<int>(my) + 12;
            // Keep on screen
            int winW, winH;
            SDL_GetWindowSize(window, &winW, &winH);
            if (tx + tw > winW) tx = static_cast<int>(mx) - tw - 4;
            if (ty + th > winH) ty = static_cast<int>(my) - th - 4;

            // Background
            SDL_SetRenderDrawColor(renderer, 45, 45, 55, 230);
            SDL_FRect bg { static_cast<float>(tx), static_cast<float>(ty), static_cast<float>(tw), static_cast<float>(th) };
            SDL_RenderFillRect(renderer, &bg);
            SDL_SetRenderDrawColor(renderer, 100, 100, 120, 255);
            SDL_RenderRect(renderer, &bg);

            // Text
            SDL_Surface* surf = TTF_RenderText_Blended(font, hoveredWidget->tooltip.c_str(), 0, theme.fg);
            if (surf)
            {
                SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
                SDL_FRect dst {
                    static_cast<float>(tx + pad), static_cast<float>(ty + pad), static_cast<float>(surf->w),
                    static_cast<float>(surf->h)
                };
                SDL_RenderTexture(renderer, tex, nullptr, &dst);
                SDL_DestroyTexture(tex);
                SDL_DestroySurface(surf);
            }
        }

        SDL_RenderPresent(renderer);

        for (auto& d : dialogs) { if (d->open) d->render(); }
    }

    /* 	Sets running = false */
    void quit() { running = false; }
};