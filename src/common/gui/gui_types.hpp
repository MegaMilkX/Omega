#pragma once

enum GUI_UNIT {
    GUI_PX,
    GUI_EM,
    GUI_PERC
};

class gui_unit {
    int value : 29;
    int type : 3;
};

class gui_unit2 {

};


gui_unit operator "" _px(unsigned long long);
gui_unit operator "" _em(unsigned long long);
gui_unit operator "" _prc(unsigned long long);