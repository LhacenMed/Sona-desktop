#include "widgets/DisabledArea.h"

#include "graphics/Canvas.h"

void DisabledArea::paintTree(Canvas &canvas)
{
    // Half opacity, as the toolkit's disabled buttons.
    canvas.pushOpacity(0.5f);
    View::paintTree(canvas);
    canvas.popOpacity();
}
