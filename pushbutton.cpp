#include "pushbutton.h"
PushButton::press PushButton::update() {
if (Bounce::update()) {
        if (fell()) return press::down;
        if (rose()) {
            if (!longpressActive) {
                return press::shortpress;
                }
            else
                {
                    longpressActive=false;
                    return press::up;
                }
        }
            
    };
    if (read()==LOW && duration()>=longPresInterval_ && !longpressActive) {
        longpressActive=true;
        return press::longpress;
    }
    return press::none;
}