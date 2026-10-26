#include "player/ControlBar.h"

#include "graphics/Canvas.h"
#include "player/BarItems.h"
#include "style/Theme.h"

ControlBar::ControlBar(BarContext &context)
    : Row(20)
{
    setPadding({.left = 20, .right = 20});
    addBarGroups(*this, defaultBarLayout.controls, context);
}

void ControlBar::paint(Canvas &canvas)
{
    canvas.fillRect(bounds(), canvas.palette().bgA3);
}
