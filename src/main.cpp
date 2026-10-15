// main.cpp — Demo of the SDL3 GUI system
#include "sdl3_gui.h"
#include <string>

int main() {
    GUIApp app;
    if (!app.init("SDL3 GUI Demo", 800, 600)) return 1;

    // ─── Dialog (created early so lambdas can capture it) ────────────────────
    Dialog* aboutDlg = app.createDialog("About", 300, 200, true);
    aboutDlg->add<Label>("SDL3 GUI System", 20, 40, 260, 30);
    aboutDlg->add<Label>("Version 1.0", 20, 80, 200, 24);
    aboutDlg->add<Button>("OK", 110, 140, 80, 30, [aboutDlg]() { aboutDlg->closeDialog(); });

    // ─── Menu Bar (background only) ──────────────────────────────────────────
    Toolbar* menuBar = app.root->add<Toolbar>(0, 0, 800, 30);
    menuBar->setStretch(true, false);

    // ─── Toolbar ─────────────────────────────────────────────────────────────
    Toolbar* toolbar = app.root->add<Toolbar>(0, 30, 800, 36);
    toolbar->setStretch(true, false);
    auto* toolNew = toolbar->addTool<Button>("New", 4, 32, 60, 32, []() { SDL_Log("Tool: New"); });
    auto* toolOpen = toolbar->addTool<Button>("Open", 70, 32, 60, 32, []() { SDL_Log("Tool: Open"); });
    auto* toolSave = toolbar->addTool<Button>("Save", 136, 32, 60, 32, []() { SDL_Log("Tool: Save"); });
    auto* toolRun = toolbar->addTool<Button>("Run", 202, 32, 60, 32, []() { SDL_Log("Tool: Run"); });
    toolNew->setTooltip("Create a new document");
    toolOpen->setTooltip("Open an existing file");
    toolSave->setTooltip("Save current changes");
    toolRun->setTooltip("Run the application");

    // ─── Tab Panel ───────────────────────────────────────────────────────────
    TabPanel* tabs = app.root->add<TabPanel>(4, 70, 520, 400);
    tabs->setStretch(true, true);

    // Tab 1: General
    Container* tab1 = tabs->addTab("General");
    tab1->add<Label>("Application Settings:", 10, 10, 200, 24);
    tab1->add<Label>("Name:", 10, 40, 80, 24);
    auto* nameBox = tab1->add<TextBox>(100, 40, 200, 26);
    nameBox->setTooltip("Enter the application name");
    tab1->add<Label>("Version:", 10, 74, 80, 24);
    auto* versionBox = tab1->add<TextBox>(100, 74, 200, 26);
    versionBox->setTooltip("Enter the version number");
    auto* applyBtn = tab1->add<Button>("Apply", 100, 110, 80, 28, []() { SDL_Log("Settings applied"); });
    applyBtn->setTooltip("Save your changes");

    // Tab 2: Advanced
    Container* tab2 = tabs->addTab("Advanced");
    tab2->add<Label>("Debug Level:", 10, 10, 120, 24);
    tab2->add<TextBox>(140, 10, 150, 26);
    tab2->add<Label>("Log Path:", 10, 44, 100, 24);
    tab2->add<TextBox>(120, 44, 220, 26);

    // Tab 3: Network
    Container* tab3 = tabs->addTab("Network");
    tab3->add<Label>("Server:", 10, 10, 80, 24);
    tab3->add<TextBox>(100, 10, 200, 26);
    tab3->add<Label>("Port:", 10, 44, 60, 24);
    tab3->add<TextBox>(80, 44, 80, 26);

    // ─── ScrollBox with selection + add ──────────────────────────────────────
    ScrollBox* scroll = app.root->add<ScrollBox>(530, 70, 266, 340);
    scroll->setStretch(true, true);
    scroll->setStretchOffsets(90, 0);  // ← stops 90px from bottom (room for label + input row)   
    for (int i = 0; i < 20; ++i) {
        scroll->addItem<Label>("Item " + std::to_string(i + 1), 4, i * 30, 250, 28);
    }
    scroll->update();

    Label* selectedLabel = app.root->add<Label>("(none selected)", 530, 30, 266, 20);
    selectedLabel->setAnchor(true, false);
    scroll->onSelect = [selectedLabel](const std::string& txt) {
        selectedLabel->text = "Selected: " + txt;
        };

    TextBox* newEntry = app.root->add<TextBox>(530, 60, 140, 26);
    newEntry->setAnchor(true, false);
    newEntry->setTooltip("Type a name, then click Add");

    auto* addBtn = app.root->add<Button>("Add", 674, 60, 50, 26, [scroll, newEntry]() {
        if (!newEntry->text.empty()) {
            scroll->addNewItem(newEntry->text);
            newEntry->text.clear();
        }
        });
    addBtn->setAnchor(true, false);
    addBtn->setTooltip("Add a new item to the list");

    auto* aboutBtn = app.root->add<Button>("About", 10, 10, 66, 26, [aboutDlg, &app]() {
        aboutDlg->openDialog(app.window);
        });
    aboutBtn->setAnchor(true, true);
    aboutBtn->setTooltip("About this application");

    // ─── Status Bar ──────────────────────────────────────────────────────────
    app.root->add<Label>("Ready", 4, 20, 400, 20)->setAnchor(true, false);
    app.root->add<Label>("SDL3 GUI Demo", 500, 20, 296, 20)->setAnchor(true, true);

    // ─── Menus (added LAST so they're topmost in z-order) ────────────────────
    Menu* fileMenu = app.root->add<Menu>("File", 4, 2, 50, 26);
    fileMenu->addItem("New", []() { SDL_Log("New"); });
    fileMenu->addItem("Open", []() { SDL_Log("Open"); });
    fileMenu->addItem("Save", []() { SDL_Log("Save"); });
    fileMenu->addItem("Exit", [&app]() { app.quit(); });

    Menu* editMenu = app.root->add<Menu>("Edit", 60, 2, 50, 26);
    editMenu->addItem("Undo", []() { SDL_Log("Undo"); });
    editMenu->addItem("Redo", []() { SDL_Log("Redo"); });
    editMenu->addItem("Cut", []() { SDL_Log("Cut"); });

    Menu* helpMenu = app.root->add<Menu>("Help", 116, 2, 50, 26);
    helpMenu->addItem("About", [aboutDlg, &app]() {
        aboutDlg->openDialog(app.window);
        });

    // ─── Run ─────────────────────────────────────────────────────────────────
    app.run();
    return 0;
}