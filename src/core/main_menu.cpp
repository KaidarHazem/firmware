#include "main_menu.h"
#include "display.h"
#include "utils.h"
#include <globals.h>

static void launchCyberMusic() {
    tft.fillScreen(bruceConfig.bgColor);
    tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
    tft.drawString("Cyber Music", 10, 20);
    while (!check(EscPress)) delay(20);
}

MainMenu::MainMenu() {
    _menuItems = {
        &wifiMenu,
        &bleMenu,
        &rfMenu,
        &nrf24Menu,
#if !defined(LITE_VERSION)
        &loraMenu,
#endif
#if !defined(LITE_VERSION)
        &fmMenu,
#endif
        &irMenu,
#if !defined(LITE_VERSION)
        &ethernetMenu,
#endif
        &gpsMenu,
        &rfidMenu,
        &fileMenu,
#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
        &scriptsMenu,
#endif
        &clockMenu,
        &othersMenu,
#if !defined(LITE_VERSION)
        &wireToolsMenu,
#endif
        &configMenu,
    };

    _totalItems = _menuItems.size();

#if defined(HAS_TOUCH)
    gridPageTapHandler = [](int x, int y, int currentIndex, int &newIndex) -> bool {
        return mainMenu.handleGridPageTap(x, y, currentIndex, newIndex);
    };
#endif
}

MainMenu::~MainMenu() {}

void MainMenu::begin(void) {
    returnToMenu = false;
    options = {};

    bool gridStyle = bruceConfig.mainMenuStyle == MAIN_MENU_GRID;

    bool (*renderItem)(void *menuItem, bool shouldRender) = [](void *menuItem, bool shouldRender) {
        if (!shouldRender) return false;
        drawMainBorder(false);

        MenuItemInterface *obj = static_cast<MenuItemInterface *>(menuItem);
        float scale = float((float)tftWidth / (float)240);
        if (bruceConfigPins.rotation & 0b01) scale = float((float)tftHeight / (float)135);
        obj->draw(scale);
#if defined(HAS_TOUCH)
        TouchFooter();
#endif
        return true;
    };

    if (gridStyle) {
        renderItem = [](void *menuItem, bool shouldRender) {
            if (!shouldRender) return false;
            mainMenu.drawGrid(mainMenu.gridIndexOf(static_cast<MenuItemInterface *>(menuItem)));
            return true;
        };
    }

    std::vector<String> l = bruceConfig.disabledMenus;
    for (int i = 0; i < _totalItems; i++) {
        String itemName = _menuItems[i]->getName();
        if (find(l.begin(), l.end(), itemName) == l.end()) {
            options.push_back(
                {itemName,
                 [this, i]() { _menuItems[i]->optionsMenu(); },
                 false,
                 renderItem,
                 _menuItems[i]}
            );
        }
    }

    options.push_back({"Cyber Music", launchCyberMusic, false, renderItem, &othersMenu});

    if (gridStyle) {
        buildGridLayout(options.size());
        _gridScroll = 0;
        _gridLastIndex = -1;
        _gridRedrawAll = true;
        mainMenuGridColumns = _grid.cols;
    } else {
        mainMenuGridColumns = 0;
    }

    _currentIndex = loopOptions(options, MENU_TYPE_MAIN, "Main Menu", _currentIndex);
}

int MainMenu::gridIndexOf(MenuItemInterface *item) {
    for (size_t i = 0; i < options.size(); i++) {
        if (options[i].hoverPointer == item) return (int)i;
    }
    return 0;
}

void MainMenu::buildGridLayout(int itemCount) {
    const int MIN_CELL_W = 54 * FP;
    const int MIN_CELL_H = 40 * FP;
    const int PAGE_TAP_W = 4 * BORDER_OFFSET_FROM_SCREEN_EDGE;

    if (itemCount < 1) itemCount = 1;

    int areaX = BORDER_OFFSET_FROM_SCREEN_EDGE + 2;
    int areaY = STATUS_BAR_HEIGHT - 2;
    int areaW = tftWidth - 2 * areaX - PAGE_TAP_W;
    int areaH = tftHeight - areaY - areaX;

    int maxCols = max(1, min(itemCount, areaW / MIN_CELL_W));
    int bestCols = 1;
    int bestScore = -1;
    for (int cols = 1; cols <= maxCols; cols++) {
        int rows = (itemCount + cols - 1) / cols;
        int score = min(areaW / cols, areaH / rows);
        if (score > bestScore) {
            bestScore = score;
            bestCols = cols;
        }
    }

    _grid.cols = bestCols;
    _grid.cellW = areaW / _grid.cols;
    _grid.rows = (itemCount + _grid.cols - 1) / _grid.cols;

    _grid.visibleRows = max(1, areaH / MIN_CELL_H);
    if (_grid.visibleRows > _grid.rows) _grid.visibleRows = _grid.rows;
    _grid.cellH = areaH / _grid.visibleRows;

    _grid.labelSize = _grid.cellW >= 8 * LW * FM ? FM : FP;
    _grid.iconBox = _grid.cellH - LH * _grid.labelSize - 6;
    if (_grid.iconBox > _grid.cellW - 4) _grid.iconBox = _grid.cellW - 4;
    if (_grid.iconBox < 12) _grid.iconBox = 12;

    _grid.x = areaX + (areaW - _grid.cols * _grid.cellW) / 2;
    _grid.y = areaY + (areaH - _grid.visibleRows * _grid.cellH) / 2;

    _gridItemCount = itemCount;
    _gridRotation = bruceConfigPins.rotation;
}

void MainMenu::drawGrid(int index) {
    int itemCount = options.size();
    if (itemCount < 1) return;
    if (index < 0 || index >= itemCount) index = 0;

    if (itemCount != _gridItemCount || _gridRotation != bruceConfigPins.rotation) {
        buildGridLayout(itemCount);
        _gridScroll = 0;
        _gridLastIndex = -1;
        _gridRedrawAll = true;
    }

    int scroll = _gridScroll;
    int row = index / _grid.cols;
    if (row < scroll) scroll = row;
    else if (row >= scroll + _grid.visibleRows) scroll = row - _grid.visibleRows + 1;

    bool redrawAll = _gridRedrawAll || scroll != _gridScroll;
    _gridScroll = scroll;

    if (redrawAll) {
        for (auto &opt : options) {
            opt.w = 0;
            opt.h = 0;
        }
        drawMainBorder(false);
        tft.fillRect(
            _grid.x, _grid.y, _grid.cols * _grid.cellW, _grid.visibleRows * _grid.cellH, bruceConfig.bgColor
        );
        int first = _gridScroll * _grid.cols;
        int last = min(itemCount, first + _grid.visibleRows * _grid.cols);
        for (int i = first; i < last; i++) drawGridCell(i, i == index);
        drawGridScrollBar();
#if defined(HAS_TOUCH)
        TouchFooter();
#endif
    } else {
        if (_gridLastIndex >= 0 && _gridLastIndex != index && _gridLastIndex < itemCount)
            drawGridCell(_gridLastIndex, false);
        drawGridCell(index, true);
    }

    _gridLastIndex = index;
    _gridRedrawAll = false;
}

void MainMenu::drawGridCell(int index, bool selected) {
    int row = index / _grid.cols - _gridScroll;
    int col = index % _grid.cols;
    if (row < 0 || row >= _grid.visibleRows) return;

    int x = _grid.x + col * _grid.cellW;
    int y = _grid.y + row * _grid.cellH;

    options[index].x = x;
    options[index].y = y;
    options[index].w = _grid.cellW;
    options[index].h = _grid.cellH;

    uint16_t bgColor = selected ? bruceConfig.priColor : bruceConfig.bgColor;
    uint16_t fgColor = selected ? bruceConfig.bgColor : bruceConfig.priColor;

    tft.fillRect(x, y, _grid.cellW, _grid.cellH, bruceConfig.bgColor);
    if (selected) tft.fillRoundRect(x + 1, y + 1, _grid.cellW - 2, _grid.cellH - 2, 3, bgColor);

    MenuItemInterface *item = static_cast<MenuItemInterface *>(options[index].hoverPointer);
    if (item)
        item->drawIconInBox(x + _grid.cellW / 2, y + 3 + _grid.iconBox / 2, _grid.iconBox, fgColor, bgColor);

    int maxChars = (_grid.cellW - 4) / (LW * _grid.labelSize);
    tft.setTextSize(_grid.labelSize);
    tft.setTextColor(fgColor, bgColor);
    tft.drawCentreString(
        options[index].label.substring(0, maxChars),
        x + _grid.cellW / 2,
        y + _grid.cellH - LH * _grid.labelSize - 3,
        1
    );
}

void MainMenu::drawGridScrollBar() {
    int barX = tftWidth - BORDER_OFFSET_FROM_SCREEN_EDGE - 4;
    int trackY = _grid.y;
    int trackH = _grid.visibleRows * _grid.cellH;

    tft.fillRect(barX, trackY, 3, trackH, bruceConfig.bgColor);
    if (_grid.rows <= _grid.visibleRows) return;

    int thumbH = max(6, trackH * _grid.visibleRows / _grid.rows);
    int thumbY = trackY + (trackH - thumbH) * _gridScroll / (_grid.rows - _grid.visibleRows);
    tft.fillRect(barX, thumbY, 3, thumbH, bruceConfig.priColor);
}

bool MainMenu::handleGridPageTap(int x, int y, int currentIndex, int &newIndex) {
    if (_grid.rows <= _grid.visibleRows) return false;

    int zoneX = _grid.x + _grid.cols * _grid.cellW;
    int zoneY = _grid.y;
    int zoneW = tftWidth - BORDER_OFFSET_FROM_SCREEN_EDGE - zoneX;
    int zoneH = _grid.visibleRows * _grid.cellH;
    if (x < zoneX || x >= zoneX + zoneW || y < zoneY || y >= zoneY + zoneH) return false;

    int maxScroll = _grid.rows - _grid.visibleRows;
    int targetScroll = (y < zoneY + zoneH / 2) ? max(0, _gridScroll - _grid.visibleRows)
                                               : min(maxScroll, _gridScroll + _grid.visibleRows);

    int col = currentIndex % _grid.cols;
    int itemCount = static_cast<int>(options.size());
    newIndex = min(targetScroll * _grid.cols + col, itemCount - 1);
    return true;
}

void MainMenu::hideAppsMenu() {
    auto items = this->getItems();
    int index = 0;
RESTART:
    options.clear();
    for (auto item : items) {
        String label = item->getName();
        std::vector<String> l = bruceConfig.disabledMenus;
        bool enabled = find(l.begin(), l.end(), label) == l.end();
        options.push_back(
            {label,
             [this, label, enabled]() {
                 if (enabled) bruceConfig.addDisabledMenu(label);
                 else bruceConfig.removeDisabledMenu(label);
             },
             enabled}
        );
    }
    options.push_back({"Show All", [=]() { bruceConfig.disabledMenus.clear(); }, true});
    addOptionToMainMenu();
    index = loopOptions(options, index);
    bruceConfig.saveFile();
    if (!returnToMenu) goto RESTART;
}
