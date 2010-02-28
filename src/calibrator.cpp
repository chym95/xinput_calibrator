/*
 * Copyright (c) 2009 Tias Guns
 * Copyright (c) 2009 Soren Hauberg
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <algorithm>
#include "calibrator.hh"

Calibrator::Calibrator(const char* const device_name0, const XYinfo& axys0, const bool verbose0)
  : device_name(device_name0), old_axys(axys0), verbose(verbose0), num_clicks(0), threshold_doubleclick(7), threshold_misclick(15)
{
}

void Calibrator::set_threshold_doubleclick(int t)
{
    threshold_doubleclick = t;
}

void Calibrator::set_threshold_misclick(int t)
{
    threshold_misclick = t;
}

int Calibrator::get_numclicks()
{
    return num_clicks;
}

bool Calibrator::add_click(int x, int y)
{
    // Double-click check
    if (num_clicks > 0 && threshold_doubleclick > 0
     && abs (x - clicked_x[num_clicks-1]) < threshold_doubleclick
     && abs (y - clicked_y[num_clicks-1]) < threshold_doubleclick) {
        if (verbose) {
            printf("DEBUG: Not adding click %i (X=%i, Y=%i): within %i pixels of previous click\n",
                num_clicks, x, y, threshold_doubleclick);
        }
        return false;
    }

    // Mis-click detection, check second and third point with first point
    if (num_clicks == 1 || num_clicks == 2) {
        if (abs (x - clicked_x[0]) > threshold_misclick
         && abs (x - clicked_y[0]) > threshold_misclick
         && abs (y - clicked_x[0]) > threshold_misclick
         && abs (y - clicked_y[0]) > threshold_misclick) {
        printf("%i: misclick0, %i,%i to %i,%i\n", num_clicks, x, y, clicked_x[num_clicks-1], clicked_y[num_clicks-1]);
    } else {
        printf("%i: goodclick0, %i,%i to %i,%i\n", num_clicks, x, y, clicked_x[num_clicks-1], clicked_y[num_clicks-1]);
    }
    }

    // Mis-click check, check last point with second and third
    if (num_clicks == 3) {
        if ((abs (x - clicked_x[1]) > threshold_misclick
         && abs (x - clicked_y[1]) > threshold_misclick
         && abs (y - clicked_x[1]) > threshold_misclick
         && abs (y - clicked_y[1]) > threshold_misclick) ||
            (abs (x - clicked_x[2]) > threshold_misclick
         && abs (x - clicked_y[2]) > threshold_misclick
         && abs (y - clicked_x[2]) > threshold_misclick
         && abs (y - clicked_y[2]) > threshold_misclick)) {
        printf("%i: misclick3, %i,%i to %i,%i\n", num_clicks, x, y, clicked_x[num_clicks-2], clicked_y[num_clicks-1]);
    } else {
        printf("%i: goodclick3, %i,%i to %i,%i\n", num_clicks, x, y, clicked_x[num_clicks-2], clicked_y[num_clicks-1]);
    }
    }

    clicked_x[num_clicks] = x;
    clicked_y[num_clicks] = y;
    num_clicks ++;

    if (verbose)
        printf("DEBUG: Adding click %i (X=%i, Y=%i)\n", num_clicks-1, x, y);

    return true;
}

bool Calibrator::finish(int width, int height)
{
    if (get_numclicks() != 4) {
        return false;
    }

    // Should x and y be swapped?
    const bool swap_xy = (abs (clicked_x [UL] - clicked_x [UR]) < abs (clicked_y [UL] - clicked_y [UR]));
    if (swap_xy) {
        std::swap(clicked_x[LL], clicked_x[UR]);
        std::swap(clicked_y[LL], clicked_y[UR]);
    }

    // Compute min/max coordinates.
    XYinfo axys;
    // These are scaled using the values of old_axys
    const float scale_x = (old_axys.x_max - old_axys.x_min)/(float)width;
    axys.x_min = ((clicked_x[UL] + clicked_x[LL]) * scale_x/2) + old_axys.x_min;
    axys.x_max = ((clicked_x[UR] + clicked_x[LR]) * scale_x/2) + old_axys.x_min;
    const float scale_y = (old_axys.y_max - old_axys.y_min)/(float)height;
    axys.y_min = ((clicked_y[UL] + clicked_y[UR]) * scale_y/2) + old_axys.y_min;
    axys.y_max = ((clicked_y[LL] + clicked_y[LR]) * scale_y/2) + old_axys.y_min;

    // Add/subtract the offset that comes from not having the points in the
    // corners (using the same coordinate system they are currently in)
    const int delta_x = (axys.x_max - axys.x_min) / (float)(num_blocks - 2);
    axys.x_min -= delta_x;
    axys.x_max += delta_x;
    const int delta_y = (axys.y_max - axys.y_min) / (float)(num_blocks - 2);
    axys.y_min -= delta_y;
    axys.y_max += delta_y;


    // If x and y has to be swapped we also have to swap the parameters
    if (swap_xy) {
        std::swap(axys.x_min, axys.y_max);
        std::swap(axys.y_min, axys.x_max);
    }

    // finish the data, driver/calibrator specific
    return finish_data(axys, swap_xy);
}
