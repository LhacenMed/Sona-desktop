#include "widgets/Popover.h"

#include "graphics/Canvas.h"
#include "style/Surface.h"

void Popover::paint(Canvas &canvas)
{
    canvas.fillSurface(bounds(), Surfaces::panel);
}
