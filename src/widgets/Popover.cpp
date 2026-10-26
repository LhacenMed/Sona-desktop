#include "widgets/Popover.h"

#include "graphics/Canvas.h"
#include "style/Theme.h"

void Popover::paint(Canvas &canvas)
{
    canvas.fillRoundedRect(bounds(), 10, canvas.palette().bgB1Transparent);
}
