#include <libwidget/Button.h>
#include <libwidget/Container.h>

#include "demo/DemoWidget.h"

static Demo _demos[] = {
    {"Colors", colors_draw},
    {"Cube", cube_draw},
    {"Graphics", graphics_draw},
    {"Lines", lines_draw},
    {NULL, NULL},
};

Widget *_demo_widget = NULL;

void set_current_demo_callback(Demo *demo, Widget *sender, Event *event)
{
    __unused(sender);
    __unused(event);

    demo_widget_set_demo(_demo_widget, demo);
}

int main(int argc, char **argv)
{
    Result result = application_initialize(argc, argv);

    if (result != SUCCESS)
    {
        return -1;
    }

    Window *main_window = window_create("Demos", 500, 400);

    window_root(main_window)->layout = (Layout){LAYOUT_VFLOW, 0, 0};

    Widget *side_bar = container_create(window_root(main_window));
    side_bar->insets = INSETS(4, 0);
    side_bar->layout = (Layout){LAYOUT_HGRID, 4, 0};

    for (size_t i = 0; _demos[i].name; i++)
    {
        Widget *demo_button = button_create(side_bar, _demos[i].name);
        widget_set_event_handler(demo_button, EVENT_MOUSE_BUTTON_PRESS, &_demos[i], (WidgetEventHandlerCallback)set_current_demo_callback);
    }

    side_bar->bound.width = 128;

    _demo_widget = demo_widget_create(window_root(main_window));
    _demo_widget->layout_attributes = LAYOUT_FILL;

    demo_widget_set_demo(_demo_widget, &_demos[0]);

    return application_run();
}
